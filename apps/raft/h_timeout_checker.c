#include "czmq.h"
#include "monitor.h"
#include "raft.h"
#include "raft_utils.h"
#include "woofc-access.h"
#include "woofc.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

typedef struct request_vote_thread_arg {
    char member_woof[RAFT_NAME_LENGTH];
    RAFT_REQUEST_VOTE_ARG arg;
} REQUEST_VOTE_THREAD_ARG;

pthread_t thread_id[RAFT_MAX_MEMBERS];
REQUEST_VOTE_THREAD_ARG thread_arg[RAFT_MAX_MEMBERS];

void* request_vote(void* arg) {
    REQUEST_VOTE_THREAD_ARG* thread_arg = (REQUEST_VOTE_THREAD_ARG*)arg;
    char monitor_name[RAFT_NAME_LENGTH];
    char woof_name[RAFT_NAME_LENGTH];
    sprintf(monitor_name, "%s/%s", thread_arg->member_woof, RAFT_MONITOR_NAME);
    sprintf(woof_name, "%s/%s", thread_arg->member_woof, RAFT_REQUEST_VOTE_ARG_WOOF);
    unsigned long seq = monitor_remote_put(monitor_name, woof_name, "h_request_vote", &thread_arg->arg, 0);
    if (WooFInvalid(seq)) {
        log_warn("failed to request vote from %s", thread_arg->member_woof);
    } else {
        log_debug("requested vote [%lu] from %s", seq, thread_arg->member_woof);
    }
}

int h_timeout_checker(WOOF* wf, unsigned long seq_no, void* ptr) {
    log_set_tag("h_timeout_checker");
    log_set_level(RAFT_LOG_INFO);
    // log_set_level(RAFT_LOG_DEBUG);
    log_set_output(stdout);
    WooFMsgCacheInit();

    uint64_t begin = get_milliseconds();
    RAFT_TIMEOUT_CHECKER_ARG arg = {0};
    if (monitor_cast(ptr, &arg, sizeof(RAFT_TIMEOUT_CHECKER_ARG)) < 0) {
        log_error("failed to monitor_cast: %s", monitor_error_msg);
        WooFMsgCacheShutdown();
        exit(1);
    }

    // zsys_init() is called automatically when a socket is created
    // not thread safe and can only be called in main thread
    // call it here to avoid being called concurrently in the threads
    zsys_init();

    // check if it's leader, if so no need to timeout
    RAFT_SERVER_STATE server_state = {0};
    if (get_server_state(&server_state) < 0) {
        log_error("failed to get the latest sever state");
        WooFMsgCacheShutdown();
        exit(1);
    }
    if (server_state.role == RAFT_LEADER || server_state.role == RAFT_OBSERVER || server_state.role == RAFT_SHUTDOWN) {
        monitor_exit(ptr);
        monitor_join();
        WooFMsgCacheShutdown();
        return 1;
    }
    int checker_delay = server_state.timeout_min / 5;

    // check if there's new heartbeat since went into sleep
    unsigned long latest_heartbeat_seqno = WooFGetLatestSeqno(RAFT_HEARTBEAT_WOOF);
    if (WooFInvalid(latest_heartbeat_seqno)) {
        log_error("failed to get the latest seqno from %s", RAFT_HEARTBEAT_WOOF);
        WooFMsgCacheShutdown();
        exit(1);
    }
    RAFT_HEARTBEAT heartbeat = {0};
    if (WooFGet(RAFT_HEARTBEAT_WOOF, &heartbeat, latest_heartbeat_seqno) < 0) {
        log_error("failed to get the latest heartbeat");
        WooFMsgCacheShutdown();
        exit(1);
    }

    // if timeout, start an election
    memset(thread_id, 0, sizeof(pthread_t) * RAFT_MAX_MEMBERS);
    memset(thread_arg, 0, sizeof(REQUEST_VOTE_THREAD_ARG) * RAFT_MAX_MEMBERS);
    if (get_milliseconds() - heartbeat.timestamp > arg.timeout_value) {
        arg.timeout_value = random_timeout(get_milliseconds(), server_state.timeout_min, server_state.timeout_max);
        log_warn("timeout after %" PRIu64 "ms at term %" PRIu64 "",
                 get_milliseconds() - heartbeat.timestamp,
                 server_state.current_term);

        // increment the term and become candidate
        server_state.current_term += 1;
        server_state.role = RAFT_CANDIDATE;
        memcpy(server_state.current_leader, server_state.woof_name, RAFT_NAME_LENGTH);
        memset(server_state.voted_for, 0, RAFT_NAME_LENGTH);
        unsigned long seq = WooFPut(RAFT_SERVER_STATE_WOOF, NULL, &server_state);
        if (WooFInvalid(seq)) {
            log_error("failed to increment the server's term to %" PRIu64 " and initialize an election",
                      server_state.current_term);
            WooFMsgCacheShutdown();
            exit(1);
        }
        log_info("state changed at term %" PRIu64 ": CANDIDATE", server_state.current_term);
        log_info("started an election for term %" PRIu64 "", server_state.current_term);

        // put a heartbeat to avoid another timeout
        RAFT_HEARTBEAT heartbeat = {0};
        heartbeat.term = server_state.current_term;
        heartbeat.timestamp = get_milliseconds();
        seq = WooFPut(RAFT_HEARTBEAT_WOOF, NULL, &heartbeat);
        if (WooFInvalid(seq)) {
            log_error("failed to put a heartbeat for the election");
            WooFMsgCacheShutdown();
            exit(1);
        }

        // initialize the vote progress
        // remember the latest seqno of request_vote_result before requesting votes
        unsigned long vote_pool_seqno = WooFGetLatestSeqno(RAFT_REQUEST_VOTE_RESULT_WOOF);
        if (WooFInvalid(vote_pool_seqno)) {
            log_error("failed to get the latest seqno from %s", RAFT_REQUEST_VOTE_RESULT_WOOF);
            WooFMsgCacheShutdown();
            exit(1);
        }

        // get last log entry info
        unsigned long latest_log_entry = WooFGetLatestSeqno(RAFT_LOG_ENTRIES_WOOF);
        if (WooFInvalid(latest_log_entry)) {
            log_error("failed to get the latest seqno from %s", RAFT_LOG_ENTRIES_WOOF);
            WooFMsgCacheShutdown();
            exit(1);
        }
        RAFT_LOG_ENTRY last_log_entry = {0};
        if (WooFGet(RAFT_LOG_ENTRIES_WOOF, &last_log_entry, latest_log_entry) < 0) {
            log_error("failed to get the latest log entry %lu from %s", latest_log_entry, RAFT_LOG_ENTRIES_WOOF);
            WooFMsgCacheShutdown();
            exit(1);
        }

        // requesting votes from members
        int i;
        for (i = 0; i < server_state.members; ++i) {
            strcpy(thread_arg[i].member_woof, server_state.member_woofs[i]);
            thread_arg[i].arg.term = server_state.current_term;
            strcpy(thread_arg[i].arg.candidate_woof, server_state.woof_name);
            thread_arg[i].arg.candidate_vote_pool_seqno = vote_pool_seqno;
            thread_arg[i].arg.last_log_index = latest_log_entry;
            thread_arg[i].arg.last_log_term = last_log_entry.term;
            if (pthread_create(&thread_id[i], NULL, request_vote, (void*)&thread_arg[i]) < 0) {
                log_error("failed to create thread to send entries");
                WooFMsgCacheShutdown();
                exit(1);
            }
        }
        log_debug("requested vote from %d members", server_state.members);
    }
    monitor_exit(ptr);

    usleep(checker_delay * 1000);
    unsigned long seq = monitor_put(RAFT_MONITOR_NAME, RAFT_TIMEOUT_CHECKER_WOOF, "h_timeout_checker", &arg, 1);
    if (WooFInvalid(seq)) {
        log_error("failed to queue the next h_timeout_checker handler");
        WooFMsgCacheShutdown();
        exit(1);
    }

    threads_join(server_state.members, thread_id);
    monitor_join();
    WooFMsgCacheShutdown();
    return 1;
}
