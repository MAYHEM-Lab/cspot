#include "czmq.h"
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
    RAFT_APPEND_ENTRIES_ARG arg;                          // do not change the relative order
    RAFT_LOG_ENTRY entries[RAFT_MAX_ENTRIES_PER_REQUEST]; // of these two members
    uint64_t term;
    unsigned long seq_no;
} REPLICATE_THREAD_ARG;

void* replicate_thread(void* arg) {
    REPLICATE_THREAD_ARG* replicate_thread_arg = (REPLICATE_THREAD_ARG*)arg;
    replicate_thread_arg->arg.ts_replicated = get_milliseconds();
    log_debug("sending %d entries to member %d, prev_index: %" PRIu64 "",
              replicate_thread_arg->arg.num_entries,
              replicate_thread_arg->member_id,
              replicate_thread_arg->arg.prev_log_index);
    char woof_name[RAFT_NAME_LENGTH];
    sprintf(woof_name,
            "%s/%s_%dX",
            replicate_thread_arg->member_woof,
            RAFT_APPEND_ENTRIES_ARG_WOOF,
            replicate_thread_arg->arg.num_entries);

    uint64_t put_begin = get_milliseconds();
    unsigned long seq =
        WooFPut(woof_name, "h_append_entries", &replicate_thread_arg->arg); // will be followed by entries
    if (get_milliseconds() - put_begin > 5000) {
        log_warn("calling h_append_entries took %lu ms", get_milliseconds() - put_begin);
    }
    if (WooFInvalid(seq)) {
        log_warn("failed to replicate the log entries to member %d, delaying the next thread to next heartbeat",
                 replicate_thread_arg->member_id);
        unsigned long last_log_entry_seqno = WooFGetLatestSeqno(RAFT_LOG_ENTRIES_WOOF);
        if (WooFInvalid(last_log_entry_seqno)) {
            log_error("failed to get the latest seqno from %s", RAFT_LOG_ENTRIES_WOOF);
            return;
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
        if (replicate_thread_arg->arg.num_entries > 0) {
            log_debug("sent %d entries to member %d [%lu], prev_index: %" PRIu64 "",
                      replicate_thread_arg->arg.num_entries,
                      replicate_thread_arg->member_id,
                      seq,
                      replicate_thread_arg->arg.prev_log_index);
        } else {
            log_debug("sent a heartbeat to member %d [%lu]", replicate_thread_arg->member_id, seq);
        }
    }
}

int comp_index(const void* a, const void* b) {
    return *(unsigned long*)b - *(unsigned long*)a;
}

int update_commit_index(RAFT_SERVER_STATE* server_state) {
    // update commit_index using qsort
    unsigned long sorted_match_index[server_state->members];
    memcpy(sorted_match_index, server_state->match_index, sizeof(unsigned long) * server_state->members);
    qsort(sorted_match_index, server_state->members, sizeof(unsigned long), comp_index);
    int i;
    for (i = server_state->members / 2; i < server_state->members; ++i) {
        if (sorted_match_index[i] <= server_state->commit_index) {
            break;
        }
        RAFT_LOG_ENTRY entry = {0};
        if (WooFGet(RAFT_LOG_ENTRIES_WOOF, &entry, sorted_match_index[i]) < 0) {
            log_error("failed to get the log_entry at %lu", sorted_match_index[i]);
            return -1;
        }
        if (entry.term == server_state->current_term && sorted_match_index[i] > server_state->commit_index) {
            // update commit_index
            server_state->commit_index = sorted_match_index[i];
            log_debug("updated commit_index to %" PRIu64 " at term %" PRIu64 "",
                     server_state->commit_index,
                     server_state->current_term);

            // check if joint config is commited
            if (server_state->current_config == RAFT_CONFIG_STATUS_JOINT &&
                server_state->commit_index >= server_state->last_config_seqno) {
                log_info("joint config is committed, appending new config");
                // append the new config
                RAFT_LOG_ENTRY config_entry = {0};
                if (WooFGet(RAFT_LOG_ENTRIES_WOOF, &config_entry, server_state->last_config_seqno) < 0) {
                    log_error("failed to get the commited config %" PRIu64 "", server_state->last_config_seqno);
                    return -1;
                }
                int new_members;
                char new_member_woofs[RAFT_MAX_MEMBERS][RAFT_NAME_LENGTH];
                int joint_config_len = decode_config(config_entry.data.val, &new_members, new_member_woofs);
                decode_config(config_entry.data.val + joint_config_len, &new_members, new_member_woofs);
                log_debug("there are %d members in the new config", new_members);

                // append a config entry to log
                RAFT_LOG_ENTRY new_config_entry = {0};
                new_config_entry.term = server_state->current_term;
                new_config_entry.is_config = RAFT_CONFIG_ENTRY_NEW;
                new_config_entry.is_handler = 0;
                if (encode_config(new_config_entry.data.val, new_members, new_member_woofs) < 0) {
                    log_error("failed to encode the new config to a log entry");
                    return -1;
                }
                raft_lock(RAFT_LOCK_LOG);
                unsigned long entry_seq = WooFPut(RAFT_LOG_ENTRIES_WOOF, NULL, &new_config_entry);
                raft_unlock(RAFT_LOCK_LOG);
                if (WooFInvalid(entry_seq)) {
                    log_error("failed to put the new config as log entry");
                    return -1;
                }
                log_debug("appended the new config as a log entry");
                server_state->members = new_members;
                server_state->current_config = RAFT_CONFIG_STATUS_STABLE;
                server_state->last_config_seqno = entry_seq;
                memcpy(server_state->member_woofs, new_member_woofs, RAFT_MAX_MEMBERS * RAFT_NAME_LENGTH);
                int i;
                for (i = 0; i < RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS; ++i) {
                    server_state->next_index[i] = entry_seq + 1;
                    server_state->match_index[i] = 0;
                    server_state->last_sent_timestamp[i] = 0;
                }
                if (member_id(server_state->woof_name, server_state->member_woofs) < 0) {
                    // TODO shutdown
                    log_debug("not in new config, shutdown");
                    server_state->role = RAFT_SHUTDOWN;
                }
                unsigned long seq = WooFPut(RAFT_SERVER_STATE_WOOF, NULL, &server_state);
                if (WooFInvalid(seq)) {
                    log_error("failed to update server config at term %" PRIu64 "", server_state->current_term);
                    return -1;
                }
                log_info("start using new config with %d members", server_state->members);
                if (server_state->role == RAFT_SHUTDOWN) {
                    log_info("server not in the leader config anymore: SHUTDOWN");
                }
            }
            break;
        }
    }
    return 0;
}

int check_append_result(RAFT_SERVER_STATE* server_state, RAFT_REPLICATE_ENTRIES_ARG* arg) {
    // check previous append_entries_result
    unsigned long last_append_result_seqno = WooFGetLatestSeqno(RAFT_APPEND_ENTRIES_RESULT_WOOF);
    if (WooFInvalid(last_append_result_seqno)) {
        log_error("failed to get the latest seqno from %s", RAFT_APPEND_ENTRIES_RESULT_WOOF);
        return -1;
    }
    int count = last_append_result_seqno - arg->last_seen_result_seqno;
    if (count != 0) {
        log_debug(
            "checking %d results from %lu to %lu", count, arg->last_seen_result_seqno + 1, last_append_result_seqno);
    }
    unsigned long result_seq;
    for (result_seq = arg->last_seen_result_seqno + 1; result_seq <= last_append_result_seqno; ++result_seq) {
        RAFT_APPEND_ENTRIES_RESULT result = {0};
        if (WooFGet(RAFT_APPEND_ENTRIES_RESULT_WOOF, &result, result_seq) < 0) {
            log_error("failed to get append_entries result at %lu", result_seq);
            return -1;
        }

#ifdef PROFILING
        uint64_t ts_result = get_milliseconds();
        printf("RAFT_PROFILE received->result: %lu\n", ts_result - result.ts_received);
#endif

        int result_member_id = member_id(result.server_woof, server_state->member_woofs);
        if (result_member_id == -1) {
            log_warn("received a result not in current config: %s", result.server_woof);
            int k;
            for (k = 0; k < 3; ++k) {
                log_warn("server_state->member_woofs[%d]: %s", k, server_state->member_woofs[k]);
            }
            arg->last_seen_result_seqno = result_seq;
            continue;
        }
        if (result_member_id < server_state->members && result.term > server_state->current_term) {
            // fall back to follower
            log_debug("request term %" PRIu64 " is higher, fall back to follower", result.term);
            server_state->current_term = result.term;
            server_state->role = RAFT_FOLLOWER;
            strcpy(server_state->current_leader, result.server_woof);
            memset(server_state->voted_for, 0, RAFT_NAME_LENGTH);
            unsigned long seq = WooFPut(RAFT_SERVER_STATE_WOOF, NULL, server_state);
            if (WooFInvalid(seq)) {
                log_error("failed to fall back to follower at term %" PRIu64 "", result.term);
                return -1;
            }
            log_info("state changed at term %" PRIu64 ": FOLLOWER", server_state->current_term);
            RAFT_HEARTBEAT heartbeat = {0};
            heartbeat.term = server_state->current_term;
            heartbeat.timestamp = get_milliseconds();
            seq = WooFPut(RAFT_HEARTBEAT_WOOF, NULL, &heartbeat);
            if (WooFInvalid(seq)) {
                log_error("failed to put a heartbeat when falling back to follower");
                return -1;
            }
            RAFT_TIMEOUT_CHECKER_ARG timeout_checker_arg = {0};
            timeout_checker_arg.timeout_value =
                random_timeout(get_milliseconds(), server_state->timeout_min, server_state->timeout_max);
            seq = WooFPut(RAFT_TIMEOUT_CHECKER_WOOF, "h_timeout_checker", &timeout_checker_arg);
            if (WooFInvalid(seq)) {
                log_error("failed to start the timeout checker");
                return -1;
            }
            return 1;
        }
        if (result.term == arg->term) {
            server_state->next_index[result_member_id] = result.last_entry_seq + 1;
            server_state->acked_request_seq[result_member_id] = result.ack_seq;
            if (result.success == 1) {
                // don't update the next index when append succeeds, let the leader do it so it can send more entries
                // without waiting acknowledgement
                server_state->match_index[result_member_id] = result.last_entry_seq;
            }
        }
        arg->last_seen_result_seqno = result_seq;
    }
    return 0;
}

int invoke_commit_handler_and_map_topic_index(RAFT_SERVER_STATE* server_state) {
    unsigned int i;
    for (i = server_state->last_checked_committed_entry + 1; i <= server_state->commit_index; ++i) {
        RAFT_LOG_ENTRY entry = {0};
        if (WooFGet(RAFT_LOG_ENTRIES_WOOF, &entry, i) < 0) {
            log_error("failed to get log entry at %lu", i);
        }

        // if the entry is a handler entry, invoke the handler
        if (entry.is_handler) {
            RAFT_LOG_HANDLER_ENTRY* handler_entry = (RAFT_LOG_HANDLER_ENTRY*)&entry.data;
            unsigned long seq = WooFPut(RAFT_LOG_HANDLER_ENTRIES_WOOF, handler_entry->handler, handler_entry->ptr);
            if (WooFInvalid(seq)) {
                log_error("failed to invoke %s for appended handler entry", handler_entry->handler);
            }
            log_debug("appended a handler entry and invoked the handler %s", handler_entry->handler);
        }

        // if the entry belongs to a topic, map the index
        if (entry.topic_name[0] != 0) {
            char index_map_woof[RAFT_NAME_LENGTH] = {0};
            sprintf(index_map_woof, "%s_%s", entry.topic_name, RAFT_INDEX_MAPPING_WOOF_SUFFIX);
            RAFT_INDEX_MAP index_map = {0};
            index_map.index = i;
            unsigned long seq = WooFPut(index_map_woof, NULL, &index_map);
            if (WooFInvalid(seq)) {
                log_error("failed to put to %s", index_map_woof);
                
                exit(1);
            }
            log_debug("mapped index %lu to topic %s[%lu]", i, entry.topic_name, seq);
        }
        server_state->last_checked_committed_entry = i;
    }
    return 0;
}

int h_replicate_entries(WOOF* wf, unsigned long seq_no, void* ptr) {
    RAFT_REPLICATE_ENTRIES_ARG* arg = (RAFT_REPLICATE_ENTRIES_ARG*)ptr;
    log_set_tag("replicate_entries");
    log_set_level(RAFT_LOG_INFO);
    // log_set_level(RAFT_LOG_DEBUG);
    log_set_output(stdout);
    zsys_init();
    uint64_t begin = get_milliseconds();

    // get the server's current term and cluster members
    raft_lock(RAFT_LOCK_SERVER);
    RAFT_SERVER_STATE server_state = {0};
    if (WooFGet(RAFT_SERVER_STATE_WOOF, &server_state, 0) < 0) {
        log_error("failed to get the server state");
        
        raft_unlock(RAFT_LOCK_SERVER);
        exit(1);
    }
    int heartbeat_rate = server_state.timeout_min / 2;

    if (server_state.current_term != arg->term || server_state.role != RAFT_LEADER) {
        log_debug("not a leader at term %" PRIu64 " anymore, current term: %" PRIu64 "",
                  arg->term,
                  server_state.current_term);
        
        raft_unlock(RAFT_LOCK_SERVER);
        return 1;
    }

    // checking what entries need to be replicated
    unsigned long last_log_entry_seqno = WooFGetLatestSeqno(RAFT_LOG_ENTRIES_WOOF);
    if (WooFInvalid(last_log_entry_seqno)) {
        log_error("failed to get the latest seqno from %s", RAFT_LOG_ENTRIES_WOOF);
        
        exit(1);
    }
#ifdef PROFILING
    if (last_log_entry_seqno - server_state.commit_index > 2) {
        log_warn("commit_lag: %lu entries %lu ", last_log_entry_seqno - server_state.commit_index, get_milliseconds());
    }
#endif

    // send entries to members and observers
    pthread_t* thread_id = malloc(sizeof(pthread_t) * (RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS));
    memset(thread_id, 0, sizeof(pthread_t) * (RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS));
    REPLICATE_THREAD_ARG* thread_arg = malloc(sizeof(REPLICATE_THREAD_ARG) * (RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS));
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
            thread_arg[m].arg.num_entries = num_entries;
            strcpy(thread_arg[m].member_woof, server_state.member_woofs[m]);
            thread_arg[m].arg.term = server_state.current_term;
            strcpy(thread_arg[m].arg.leader_woof, server_state.woof_name);
            thread_arg[m].arg.prev_log_index = server_state.next_index[m] - 1;
            thread_arg[m].term = arg->term;
            if (thread_arg[m].arg.prev_log_index == 0) {
                thread_arg[m].arg.prev_log_term = 0;
            } else {
                RAFT_LOG_ENTRY prev_entry = {0};
                if (WooFGet(RAFT_LOG_ENTRIES_WOOF, &prev_entry, thread_arg[m].arg.prev_log_index) < 0) {
                    log_error("failed to get the previous log entry");
                    threads_join(m, thread_id);
                    free(thread_id);
                    free(thread_arg);
                    
                    raft_unlock(RAFT_LOCK_SERVER);
                    exit(1);
                }
                thread_arg[m].arg.prev_log_term = prev_entry.term;
            }
            int i;
            uint64_t get_entries_begin = get_milliseconds();
            for (i = 0; i < num_entries; ++i) {
                if (WooFGet(RAFT_LOG_ENTRIES_WOOF, &thread_arg[m].entries[i], server_state.next_index[m] + i) < 0) {
                    log_error("failed to get the log entries at %" PRIu64 "", server_state.next_index[m] + i);
                    threads_join(m, thread_id);
                    free(thread_id);
                    free(thread_arg);
                    
                    raft_unlock(RAFT_LOCK_SERVER);
                    exit(1);
                }
                if (thread_arg[m].entries[i].is_handler) {
                    RAFT_LOG_HANDLER_ENTRY* handler_entry = (RAFT_LOG_HANDLER_ENTRY*)(&thread_arg[m].entries[i].data);
                }
                thread_arg[m].entries[i].ts_written = get_milliseconds();
#ifdef PROFILING
                printf("RAFT_PROFILE created->written: %lu\n",
                       thread_arg[m].entries[i].ts_written - thread_arg[m].entries[i].ts_created);
#endif
            }
            thread_arg[m].arg.leader_commit = server_state.commit_index;
            thread_arg[m].arg.ack_seq = server_state.last_sent_request_seq[m] + 1;
            thread_arg[m].seq_no = seq_no;
            if (pthread_create(&thread_id[m], NULL, replicate_thread, (void*)&thread_arg[m]) < 0) {
                log_error("failed to create thread to send entries");
                threads_join(m, thread_id);
                free(thread_id);
                free(thread_arg);
                
                raft_unlock(RAFT_LOCK_SERVER);
                exit(1);
            }
            server_state.last_sent_timestamp[m] = get_milliseconds();
            ++server_state.last_sent_request_seq[m];
        }
    }

    int err = check_append_result(&server_state, arg);
    if (err < 0) {
        
        raft_unlock(RAFT_LOCK_SERVER);
        exit(1);
    } else if (err == 1) {
        
        raft_unlock(RAFT_LOCK_SERVER);
        return 1;
    }

    err = update_commit_index(&server_state);
    if (err < 0) {
        
        raft_unlock(RAFT_LOCK_SERVER);
        exit(1);
    }

    err = invoke_commit_handler_and_map_topic_index(&server_state);
    if (err < 0) {
        
        raft_unlock(RAFT_LOCK_SERVER);
        exit(1);
    }

    unsigned long seq = WooFPut(RAFT_SERVER_STATE_WOOF, NULL, &server_state);
    if (WooFInvalid(seq)) {
        log_error("failed to update server state");
        
        raft_unlock(RAFT_LOCK_SERVER);
        exit(1);
    }
    raft_unlock(RAFT_LOCK_SERVER);

    arg->last_ts = get_milliseconds();
    seq = WooFPut(RAFT_REPLICATE_ENTRIES_WOOF, "h_replicate_entries", arg);
    if (WooFInvalid(seq)) {
        log_error("failed to queue the next h_replicate_entries handler");
        
        exit(1);
    }

    uint64_t join_begin = get_milliseconds();
    threads_join(RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS, thread_id);
    if (get_milliseconds() - join_begin > 5000) {
        log_warn("join tooks %lu ms", get_milliseconds() - join_begin);
    }
    free(thread_id);
    free(thread_arg);
    // printf("handler h_replicate_entries took %lu\n", get_milliseconds() - begin);
    
    return 1;
}
