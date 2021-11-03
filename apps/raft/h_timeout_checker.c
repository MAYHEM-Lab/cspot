#include "czmq.h"
#include "raft.h"
#include "raft_utils.h"
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
    char woof_name[RAFT_NAME_LENGTH];
    sprintf(woof_name, "%s/%s", thread_arg->member_woof, RAFT_REQUEST_VOTE_ARG_WOOF);
    unsigned long seq = WooFPut(woof_name, "h_request_vote", &thread_arg->arg);
    if (WooFInvalid(seq)) {
        log_warn("failed to request vote from %s", thread_arg->member_woof);
    } else {
        log_debug("requested vote [%lu] from %s", seq, thread_arg->member_woof);
    }
}

int experiment_cheat(char* woof_name, unsigned long term) {
    if (term > 0) {
        return 1;
    }
    // except dht1
    // if (strstr(woof_name, "169.231.234.163") != NULL) {
    //     return 0;
    // }
    // except dht9, dht16, dht10, dht11
    // if (strstr(woof_name, "169.231.235.132") != NULL || strstr(woof_name, "169.231.235.200") != NULL ||
    //     strstr(woof_name, "169.231.235.148") != NULL || strstr(woof_name, "169.231.235.166") != NULL) {
    //     return 0;
    // }
    // except val9, val10
    if (strstr(woof_name, "128.111.45.140") != NULL || strstr(woof_name, "128.111.45.119") != NULL) {
        return 0;
    }
    // val1
    // if (strstr(woof_name, "128.111.45.112") != NULL) {
    //     return 1;
    // }
    // dht
    // if (strstr(woof_name, "169.231.23") != NULL) {
    //     return 1;
    // }
    // sed
    // if (strstr(woof_name, "128.111.39") != NULL) {
    //     return 1;
    // }
    // val
    if (strstr(woof_name, "128.111.45") != NULL) {
        return 1;
    }
    // sed1
    // if (strstr(woof_name, "128.111.39.229") != NULL) {
    //     return 1;
    // }
    log_debug("experiment_cheat not allowed");
    return 0;
}

int h_timeout_checker(WOOF* wf, unsigned long seq_no, void* ptr) {
    RAFT_TIMEOUT_CHECKER_ARG* arg = (RAFT_TIMEOUT_CHECKER_ARG*)ptr;
    log_set_tag("h_timeout_checker");
    log_set_level(RAFT_LOG_INFO);
    // log_set_level(RAFT_LOG_DEBUG);
    log_set_output(stdout);

    uint64_t begin = get_milliseconds();
    // zsys_init() is called automatically when a socket is created
    // not thread safe and can only be called in main thread
    // call it here to avoid being called concurrently in the threads
    zsys_init();

    // check if it's leader, if so no need to timeout
    raft_lock(RAFT_LOCK_SERVER);
    RAFT_SERVER_STATE server_state = {0};
    if (WooFGet(RAFT_SERVER_STATE_WOOF, &server_state, 0) < 0) {
        log_error("failed to get the latest sever state");
        
        raft_unlock(RAFT_LOCK_SERVER);
        exit(1);
    }
    if (server_state.role == RAFT_LEADER || server_state.role == RAFT_OBSERVER || server_state.role == RAFT_SHUTDOWN) {
        
        raft_unlock(RAFT_LOCK_SERVER);
        return 1;
    }

    // check if there's new heartbeat since went into sleep
    RAFT_HEARTBEAT heartbeat = {0};
    if (WooFGet(RAFT_HEARTBEAT_WOOF, &heartbeat, 0) < 0) {
        log_error("failed to get the latest heartbeat");
        
        raft_unlock(RAFT_LOCK_SERVER);
        exit(1);
    }

    // if timeout, start an election
    memset(thread_id, 0, sizeof(pthread_t) * RAFT_MAX_MEMBERS);
    memset(thread_arg, 0, sizeof(REQUEST_VOTE_THREAD_ARG) * RAFT_MAX_MEMBERS);
    if (get_milliseconds() - heartbeat.timestamp > arg->timeout_value &&
        experiment_cheat(server_state.woof_name, server_state.current_term)) {
        arg->timeout_value = random_timeout(get_milliseconds(), server_state.timeout_min, server_state.timeout_max);
        log_warn("timeout after %" PRIu64 "ms at term %" PRIu64 "",
                 get_milliseconds() - heartbeat.timestamp,
                 server_state.current_term);

        // increment the term and become candidate
        server_state.current_term += 1;
        server_state.role = RAFT_CANDIDATE;
        memcpy(server_state.current_leader, server_state.woof_name, RAFT_NAME_LENGTH);
        memset(server_state.voted_for, 0, RAFT_NAME_LENGTH);
        unsigned long seq = WooFPut(RAFT_SERVER_STATE_WOOF, NULL, &server_state);
        raft_unlock(RAFT_LOCK_SERVER);
        if (WooFInvalid(seq)) {
            log_error("failed to increment the server's term to %" PRIu64 " and initialize an election",
                      server_state.current_term);
            
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
            
            exit(1);
        }

        // initialize the vote progress
        // remember the latest seqno of request_vote_result before requesting votes
        unsigned long vote_pool_seqno = WooFGetLatestSeqno(RAFT_REQUEST_VOTE_RESULT_WOOF);
        if (WooFInvalid(vote_pool_seqno)) {
            log_error("failed to get the latest seqno from %s", RAFT_REQUEST_VOTE_RESULT_WOOF);
            
            exit(1);
        }

        // get last log entry info
        unsigned long latest_log_entry = WooFGetLatestSeqno(RAFT_LOG_ENTRIES_WOOF);
        if (WooFInvalid(latest_log_entry)) {
            log_error("failed to get the latest seqno from %s", RAFT_LOG_ENTRIES_WOOF);
            
            exit(1);
        }
        RAFT_LOG_ENTRY last_log_entry = {0};
        if (WooFGet(RAFT_LOG_ENTRIES_WOOF, &last_log_entry, latest_log_entry) < 0) {
            log_error("failed to get the latest log entry %lu from %s", latest_log_entry, RAFT_LOG_ENTRIES_WOOF);
            
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
                
                exit(1);
            }
        }
        log_debug("requested vote from %d members", server_state.members);
    } else {
        log_debug("last hearbeat was %lu ms ago", get_milliseconds() - heartbeat.timestamp);
        raft_unlock(RAFT_LOCK_SERVER);
    }

    usleep(1000 * 1000);
    unsigned long seq = WooFPut(RAFT_TIMEOUT_CHECKER_WOOF, "h_timeout_checker", arg);
    if (WooFInvalid(seq)) {
        log_error("failed to queue the next h_timeout_checker handler");
        
        exit(1);
    }

    uint64_t join_begin = get_milliseconds();
    threads_join(server_state.members, thread_id);
    if (get_milliseconds() - join_begin > 5000) {
        log_warn("join tooks %lu ms", get_milliseconds() - join_begin);
    }
    
    return 1;
}
