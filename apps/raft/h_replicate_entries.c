#include "monitor.h"
#include "raft.h"
#include "raft_utils.h"
#include "woofc.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

pthread_t thread_id[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS];
REPLICATE_THREAD_ARG thread_arg[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS];

typedef struct replicate_thread_arg {
    int member_id;
    char member_woof[RAFT_NAME_LENGTH];
    int num_entries;
    RAFT_APPEND_ENTRIES_ARG arg;
} REPLICATE_THREAD_ARG;

void* replicate_thread(void* arg) {
    REPLICATE_THREAD_ARG* replicate_thread_arg = (REPLICATE_THREAD_ARG*)arg;
    char monitor_name[RAFT_NAME_LENGTH];
    char woof_name[RAFT_NAME_LENGTH];
    sprintf(monitor_name, "%s/%s", replicate_thread_arg->member_woof, RAFT_MONITOR_NAME);
    sprintf(woof_name, "%s/%s", replicate_thread_arg->member_woof, RAFT_APPEND_ENTRIES_ARG_WOOF);
    unsigned long seq = monitor_remote_put(monitor_name, woof_name, "h_append_entries", &replicate_thread_arg->arg, 1);
    if (WooFInvalid(seq)) {
        log_warn("failed to replicate the log entries to member %d", replicate_thread_arg->member_id);
    } else {
        if (replicate_thread_arg->num_entries > 0) {
            if (replicate_thread_arg->member_id < RAFT_MAX_MEMBERS) {
                log_debug("sent %d entries to member %d [%lu], prev_index: %lu, %s",
                          replicate_thread_arg->num_entries,
                          replicate_thread_arg->member_id,
                          seq,
                          replicate_thread_arg->arg.prev_log_index,
                          woof_name);
            } else {
                log_debug("sent %d entries to observer %d [%lu], prev_index: %lu, %s",
                          replicate_thread_arg->num_entries,
                          replicate_thread_arg->member_id,
                          seq,
                          replicate_thread_arg->arg.prev_log_index,
                          woof_name);
            }
        } else {
            // log_debug("sent a heartbeat to member %d [%lu]: %s", replicate_thread_arg->member_id, seq, woof_name);
        }
    }
    free(arg);
}

int comp_index(const void* a, const void* b) {
    return *(unsigned long*)b - *(unsigned long*)a;
}

int h_replicate_entries(WOOF* wf, unsigned long seq_no, void* ptr) {
    log_set_tag("replicate_entries");
    log_set_level(RAFT_LOG_INFO);
    // log_set_level(RAFT_LOG_DEBUG);
    log_set_output(stdout);

    RAFT_REPLICATE_ENTRIES_ARG arg = {0};
    if (monitor_cast(ptr, &arg) < 0) {
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

    if (server_state.current_term != arg.term || server_state.role != RAFT_LEADER) {
        log_debug("not a leader at term %lu anymore, current term: %lu", arg.term, server_state.current_term);
        monitor_exit(ptr);
        return 1;
    }

    // if (get_milliseconds() - arg.last_ts < RAFT_REPLICATE_ENTRIES_DELAY) {
    //     monitor_exit(ptr);
    // 	usleep(RAFT_REPLICATE_ENTRIES_DELAY * 1000);
    //     unsigned long seq = monitor_put(RAFT_MONITOR_NAME, RAFT_REPLICATE_ENTRIES_WOOF, "h_replicate_entries", &arg,
    //     1); if (WooFInvalid(seq)) {
    //         log_error("failed to queue the next h_replicate_entries handler");
    //         exit(1);
    //     }
    //     return 1;
    // }

    // check previous append_entries_result
    unsigned long last_append_result_seqno = WooFGetLatestSeqno(RAFT_APPEND_ENTRIES_RESULT_WOOF);
    if (WooFInvalid(last_append_result_seqno)) {
        log_error("failed to get the latest seqno from %s", RAFT_APPEND_ENTRIES_RESULT_WOOF);
        exit(1);
    }
    unsigned long result_seq;
    for (result_seq = arg.last_seen_result_seqno + 1; result_seq <= last_append_result_seqno; ++result_seq) {
        RAFT_APPEND_ENTRIES_RESULT result = {0};
        if (WooFGet(RAFT_APPEND_ENTRIES_RESULT_WOOF, &result, result_seq) < 0) {
            log_error("failed to get append_entries result at %lu", result_seq);
            exit(1);
        }
        // if (get_milliseconds() - result.request_created_ts > RAFT_TIMEOUT_MIN) {
        // 	log_warn("request %lu took %lums to receive response", result_seq, get_milliseconds() -
        // result.request_created_ts);
        // }
        if (RAFT_SAMPLING_RATE > 0 && (result_seq % RAFT_SAMPLING_RATE == 0)) {
            log_debug("request %lu took %lums to receive response",
                      result_seq,
                      get_milliseconds() - result.request_created_ts);
        }
        int result_member_id = member_id(result.server_woof, server_state.member_woofs);
        if (result_member_id == -1) {
            log_warn("received a result not in current config");
            arg.last_seen_result_seqno = result_seq;
            continue;
        }
        if (result_member_id < server_state.members && result.term > server_state.current_term) {
            // fall back to follower
            log_debug("request term %lu is higher, fall back to follower", result.term);
            server_state.current_term = result.term;
            server_state.role = RAFT_FOLLOWER;
            strcpy(server_state.current_leader, result.server_woof);
            memset(server_state.voted_for, 0, RAFT_NAME_LENGTH);
            unsigned long seq = WooFPut(RAFT_SERVER_STATE_WOOF, NULL, &server_state);
            if (WooFInvalid(seq)) {
                log_error("failed to fall back to follower at term %lu", result.term);
                exit(1);
            }
            log_info("state changed at term %lu: FOLLOWER", server_state.current_term);
            RAFT_HEARTBEAT heartbeat = {0};
            heartbeat.term = server_state.current_term;
            heartbeat.timestamp = get_milliseconds();
            seq = WooFPut(RAFT_HEARTBEAT_WOOF, NULL, &heartbeat);
            if (WooFInvalid(seq)) {
                log_error("failed to put a heartbeat when falling back to follower");
                exit(1);
            }
            RAFT_TIMEOUT_CHECKER_ARG timeout_checker_arg = {0};
            timeout_checker_arg.timeout_value = random_timeout(get_milliseconds());
            seq =
                monitor_put(RAFT_MONITOR_NAME, RAFT_TIMEOUT_CHECKER_WOOF, "h_timeout_checker", &timeout_checker_arg, 1);
            if (WooFInvalid(seq)) {
                log_error("failed to start the timeout checker");
                exit(1);
            }
            monitor_exit(ptr);
            return 1;
        }
        if (result.term == arg.term) {
            server_state.next_index[result_member_id] = result.last_entry_seq + 1;
            if (result.success) {
                server_state.match_index[result_member_id] = result.last_entry_seq;
                // log_debug("[%lu] success[%lu], server_state.next_index[%d]: %lu", seq_no, result.seqno,
                // result_member_id, server_state.next_index[result_member_id]);
            } else {
                // log_debug("[%lu] failed[%lu], server_state.next_index[%d]: %lu", seq_no, result.seqno,
                // result_member_id, server_state.next_index[result_member_id]);
            }
        }
        arg.last_seen_result_seqno = result_seq;
    }
    // update commit_index using qsort
    unsigned long sorted_match_index[server_state.members];
    memcpy(sorted_match_index, server_state.match_index, sizeof(unsigned long) * server_state.members);
    qsort(sorted_match_index, server_state.members, sizeof(unsigned long), comp_index);
    int i;
    for (i = server_state.members / 2; i < server_state.members; ++i) {
        if (sorted_match_index[i] <= server_state.commit_index) {
            break;
        }
        RAFT_LOG_ENTRY entry = {0};
        if (WooFGet(RAFT_LOG_ENTRIES_WOOF, &entry, sorted_match_index[i]) < 0) {
            log_error("failed to get the log_entry at %lu", sorted_match_index[i]);
            exit(1);
        }
        if (entry.term == server_state.current_term && sorted_match_index[i] > server_state.commit_index) {
            // update commit_index
            server_state.commit_index = sorted_match_index[i];
            unsigned long seq = WooFPut(RAFT_SERVER_STATE_WOOF, NULL, &server_state);
            if (WooFInvalid(seq)) {
                log_error("failed to update commit_index at term %lu", server_state.current_term);
                exit(1);
            }
            log_debug("updated commit_index to %lu at %lu", server_state.commit_index, get_milliseconds());

            // check if joint config is commited
            if (server_state.current_config == RAFT_CONFIG_STATUS_JOINT &&
                server_state.commit_index >= server_state.last_config_seqno) {
                log_info("joint config is committed, appending new config");
                // append the new config
                RAFT_LOG_ENTRY config_entry = {0};
                if (WooFGet(RAFT_LOG_ENTRIES_WOOF, &config_entry, server_state.last_config_seqno) < 0) {
                    log_error("failed to get the commited config %lu", server_state.last_config_seqno);
                    exit(1);
                }
                int new_members;
                char new_member_woofs[RAFT_MAX_MEMBERS][RAFT_NAME_LENGTH];
                int joint_config_len = decode_config(config_entry.data.val, &new_members, new_member_woofs);
                decode_config(config_entry.data.val + joint_config_len, &new_members, new_member_woofs);
                log_debug("there are %d members in the new config", new_members);

                // append a config entry to log
                RAFT_LOG_ENTRY new_config_entry = {0};
                new_config_entry.term = server_state.current_term;
                new_config_entry.is_config = RAFT_CONFIG_ENTRY_NEW;
                new_config_entry.is_handler = 0;
                if (encode_config(new_config_entry.data.val, new_members, new_member_woofs) < 0) {
                    log_error("failed to encode the new config to a log entry");
                    exit(1);
                }
                unsigned long entry_seq = WooFPut(RAFT_LOG_ENTRIES_WOOF, NULL, &new_config_entry);
                if (WooFInvalid(entry_seq)) {
                    log_error("failed to put the new config as log entry");
                    exit(1);
                }
                log_debug("appended the new config as a log entry");
                server_state.members = new_members;
                server_state.current_config = RAFT_CONFIG_STATUS_STABLE;
                server_state.last_config_seqno = entry_seq;
                memcpy(server_state.member_woofs, new_member_woofs, RAFT_MAX_MEMBERS * RAFT_NAME_LENGTH);
                int i;
                for (i = 0; i < RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS; ++i) {
                    server_state.next_index[i] = entry_seq + 1;
                    server_state.match_index[i] = 0;
                    server_state.last_sent_timestamp[i] = 0;
                    server_state.last_sent_index[i] = 0;
                }
                if (member_id(server_state.woof_name, server_state.member_woofs) < 0) {
                    // TODO shutdown
                    log_debug("not in new config, shutdown");
                    server_state.role = RAFT_SHUTDOWN;
                }
                unsigned long seq = WooFPut(RAFT_SERVER_STATE_WOOF, NULL, &server_state);
                if (WooFInvalid(seq)) {
                    log_error("failed to update server config at term %lu", server_state.current_term);
                    exit(1);
                }
                log_info("start using new config with %d members", server_state.members);
                if (server_state.role == RAFT_SHUTDOWN) {
                    log_info("server not in the leader config anymore: SHUTDOWN");
                }
            }
            break;
        }
    }
    // checking what entries need to be replicated
    unsigned long last_log_entry_seqno = WooFGetLatestSeqno(RAFT_LOG_ENTRIES_WOOF);
    if (WooFInvalid(last_log_entry_seqno)) {
        log_error("failed to get the latest seqno from %s", RAFT_LOG_ENTRIES_WOOF);
        exit(1);
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
        if ((num_entries > 0 && server_state.last_sent_index[m] != server_state.next_index[m]) ||
            (get_milliseconds() - server_state.last_sent_timestamp[m] > RAFT_HEARTBEAT_RATE)) {
            thread_arg[m].member_id = m;
            thread_arg[m].num_entries = num_entries;
            strcpy(thread_arg[m].member_woof, server_state.member_woofs[m]);
            thread_arg[m].arg.term = server_state.current_term;
            strcpy(thread_arg[m].arg.leader_woof, server_state.woof_name);
            thread_arg[m].arg.prev_log_index = server_state.next_index[m] - 1;
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
                    log_error("failed to get the log entries at %lu", server_state.next_index[m] + i);
                    exit(1);
                }
            }
            thread_arg[m].arg.leader_commit = server_state.commit_index;
            thread_arg[m].arg.created_ts = get_milliseconds();
            if (pthread_create(&thread_id[m], NULL, replicate_thread, (void*)thread_arg) < 0) {
                log_error("failed to create thread to send entries");
                exit(1);
            }
            server_state.last_sent_timestamp[m] = get_milliseconds();
            server_state.last_sent_index[m] = server_state.next_index[m];
        }
    }
    unsigned long seq = WooFPut(RAFT_SERVER_STATE_WOOF, NULL, &server_state);
    if (WooFInvalid(seq)) {
        log_error("failed to update server state");
        exit(1);
    }
    monitor_exit(ptr);

    // usleep(RAFT_REPLICATE_ENTRIES_DELAY * 1000);
    arg.last_ts = get_milliseconds();
    seq = monitor_put(RAFT_MONITOR_NAME, RAFT_REPLICATE_ENTRIES_WOOF, "h_replicate_entries", &arg, 1);
    if (WooFInvalid(seq)) {
        log_error("failed to queue the next h_replicate_entries handler");
        exit(1);
    }

    threads_join(RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS, thread_id);
    return 1;
}
