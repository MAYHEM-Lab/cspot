#include "czmq.h"
#include "monitor.h"
#include "raft.h"
#include "raft_utils.h"
#include "woofc.h"

#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

typedef struct replicate_thread_arg {
    int member_id;
    char member_woof[RAFT_NAME_LENGTH];
    int num_entries;
    RAFT_APPEND_ENTRIES_ARG arg;
    uint64_t term;
    unsigned long seq_no;
} REPLICATE_THREAD_ARG;

pthread_t thread_id[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS];
REPLICATE_THREAD_ARG thread_arg[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS];

void* replicate_thread(void* arg) {
    REPLICATE_THREAD_ARG* replicate_thread_arg = (REPLICATE_THREAD_ARG*)arg;
    char monitor_name[RAFT_NAME_LENGTH];
    char woof_name[RAFT_NAME_LENGTH];
    sprintf(monitor_name, "%s/%s", replicate_thread_arg->member_woof, RAFT_MONITOR_NAME);
    sprintf(woof_name, "%s/%s", replicate_thread_arg->member_woof, RAFT_APPEND_ENTRIES_ARG_WOOF);
    unsigned long seq = monitor_remote_put(monitor_name, woof_name, "h_append_entries", &replicate_thread_arg->arg, 0);
    if (WooFInvalid(seq)) {
        log_warn("failed to replicate the log entries to member %d, delaying the next thread to next heartbeat",
                 replicate_thread_arg->member_id);
        unsigned long last_log_entry_seqno = WooFGetLatestSeqno(RAFT_LOG_ENTRIES_WOOF);
        if (WooFInvalid(last_log_entry_seqno)) {
            log_error("failed to get the latest seqno from %s", RAFT_LOG_ENTRIES_WOOF);
            exit(1);
        }
        RAFT_APPEND_ENTRIES_RESULT result = {0};
        memcpy(result.server_woof, replicate_thread_arg->member_woof, sizeof(result.server_woof));
        result.term = replicate_thread_arg->term;
        result.last_entry_seq = last_log_entry_seqno;
        result.success = 0;
        seq = WooFPut(RAFT_APPEND_ENTRIES_RESULT_WOOF, NULL, &result);
        if (WooFInvalid(seq)) {
            log_error("failed to put to result woof and cooldown failed thread");
        }
    } else {
        if (replicate_thread_arg->num_entries > 0) {
            log_debug("sent %d entries to member %d [%lu], prev_index: %" PRIu64 ", ack_seq: %lu",
                      replicate_thread_arg->num_entries,
                      replicate_thread_arg->member_id,
                      seq,
                      replicate_thread_arg->arg.prev_log_index,
                      replicate_thread_arg->arg.ack_seq);
            // log_debug("sent %d entries to member %d [%lu], prev_index: %" PRIu64 ", ack_seq: %lu",
            //           replicate_thread_arg->num_entries,
            //           replicate_thread_arg->member_id,
            //           seq,
            //           replicate_thread_arg->arg.prev_log_index,
            //           replicate_thread_arg->arg.ack_seq);
        } else {
            log_debug("sent a heartbeat to member %d [%lu]", replicate_thread_arg->member_id, seq);
        }
    }
}

int h_replicate_entries(WOOF* wf, unsigned long seq_no, void* ptr) {
    log_set_tag("replicate_entries");
    log_set_level(RAFT_LOG_INFO);
    log_set_level(RAFT_LOG_DEBUG);
    log_set_output(stdout);
uint64_t begin = get_milliseconds();
    RAFT_REPLICATE_ENTRIES_ARG arg = {0};
    if (monitor_cast(ptr, &arg, sizeof(RAFT_REPLICATE_ENTRIES_ARG)) < 0) {
        log_error("failed to monitor_cast");
        exit(1);
    }
    seq_no = monitor_seqno(ptr);

    // zsys_init() is called automatically when a socket is created
    // not thread safe and can only be called in main thread
    // call it here to avoid being called concurrently in the threads
    zsys_init();

    // get the server's current term and cluster members
    RAFT_SERVER_STATE server_state = {0};
    if (get_server_state(&server_state) < 0) {
        log_error("failed to get the server state");
        exit(1);
    }
    int heartbeat_rate = server_state.timeout_min / 2;
    int replicate_delay = server_state.replicate_delay;

    if (server_state.current_term != arg.term || server_state.role != RAFT_LEADER) {
        log_debug(
            "not a leader at term %" PRIu64 " anymore, current term: %" PRIu64 "", arg.term, server_state.current_term);
        monitor_exit(ptr);
        monitor_join();
        return 1;
    }

    // checking what entries need to be replicated
    unsigned long last_log_entry_seqno = WooFGetLatestSeqno(RAFT_LOG_ENTRIES_WOOF);
    if (WooFInvalid(last_log_entry_seqno)) {
        log_error("failed to get the latest seqno from %s", RAFT_LOG_ENTRIES_WOOF);
        exit(1);
    }
    if (last_log_entry_seqno - server_state.commit_index > 0) {
        log_warn("commit lag: %lu entries", last_log_entry_seqno - server_state.commit_index);
    }

    // send entries to members and observers
    memset(thread_id, 0, sizeof(pthread_t) * (RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS));
    memset(thread_arg, 0, sizeof(REPLICATE_THREAD_ARG) * (RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS));
    int m;
    for (m = 0; m < RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS; ++m) {
        if ((m >= server_state.members && m < RAFT_MAX_MEMBERS) || (m >= RAFT_MAX_MEMBERS + server_state.observers)) {
            continue;
        }
        if (strcmp(server_state.member_woofs[m], server_state.woof_name) == 0) {
            server_state.match_index[m] = last_log_entry_seqno;
            continue; // no need to replicate to itself
        }
        unsigned long num_entries = last_log_entry_seqno - server_state.next_index[m] + 1;
        if (num_entries > RAFT_MAX_ENTRIES_PER_REQUEST) {
            num_entries = RAFT_MAX_ENTRIES_PER_REQUEST;
        }
        if ((num_entries > 0 && server_state.acked_request_seq[m] == server_state.last_sent_request_seq[m]) ||
            (get_milliseconds() - server_state.last_sent_timestamp[m] > heartbeat_rate)) {
            thread_arg[m].member_id = m;
            thread_arg[m].num_entries = num_entries;
            strcpy(thread_arg[m].member_woof, server_state.member_woofs[m]);
            thread_arg[m].arg.term = server_state.current_term;
            strcpy(thread_arg[m].arg.leader_woof, server_state.woof_name);
            thread_arg[m].arg.prev_log_index = server_state.next_index[m] - 1;
            thread_arg[m].term = arg.term;
            if (thread_arg[m].arg.prev_log_index == 0) {
                thread_arg[m].arg.prev_log_term = 0;
            } else {
                RAFT_LOG_ENTRY prev_entry = {0};
                if (WooFGet(RAFT_LOG_ENTRIES_WOOF, &prev_entry, thread_arg[m].arg.prev_log_index) < 0) {
                    log_error("failed to get the previous log entry");
                    exit(1);
                }
                thread_arg[m].arg.prev_log_term = prev_entry.term;
            }
            int i;
            for (i = 0; i < num_entries; ++i) {
                if (WooFGet(RAFT_LOG_ENTRIES_WOOF, &thread_arg[m].arg.entries[i], server_state.next_index[m] + i) < 0) {
                    log_error("failed to get the log entries at %" PRIu64 "", server_state.next_index[m] + i);
                    exit(1);
                }
                if (thread_arg[m].arg.entries[i].is_handler) {
                    RAFT_LOG_HANDLER_ENTRY* handler_entry =
                        (RAFT_LOG_HANDLER_ENTRY*)(&thread_arg[m].arg.entries[i].data);
                }
            }
            thread_arg[m].arg.leader_commit = server_state.commit_index;
            thread_arg[m].arg.created_ts = get_milliseconds();
            thread_arg[m].arg.ack_seq = server_state.last_sent_request_seq[m] + 1;
            thread_arg[m].seq_no = seq_no;
            if (pthread_create(&thread_id[m], NULL, replicate_thread, (void*)&thread_arg[m]) < 0) {
                log_error("failed to create thread to send entries");
                exit(1);
            }
            server_state.last_sent_timestamp[m] = get_milliseconds();
            ++server_state.last_sent_request_seq[m];
        }
    }

    unsigned long seq = WooFPut(RAFT_SERVER_STATE_WOOF, NULL, &server_state);
    if (WooFInvalid(seq)) {
        log_error("failed to update server state");
        exit(1);
    }

    monitor_exit(ptr);

    // usleep(replicate_delay * 1000);
    arg.last_ts = get_milliseconds();
    seq = monitor_put(RAFT_MONITOR_NAME, RAFT_REPLICATE_ENTRIES_WOOF, "h_replicate_entries", &arg, 1);
    if (WooFInvalid(seq)) {
        log_error("failed to queue the next h_replicate_entries handler");
        exit(1);
    }

    threads_join(RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS, thread_id);
    monitor_join();
    // printf("handler h_replicate_entries took %lu\n", get_milliseconds() - begin);
    return 1;
}
