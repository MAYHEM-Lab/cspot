#include "raft.h"
#include "raft_utils.h"
#include "woofc.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define RAFT_WARNING_LATENCY(x) x / 2

int invoke_handler_and_map_topic_index(RAFT_SERVER_STATE* server_state) {
    unsigned int i;
    for (i = server_state->last_checked_committed_entry + 1; i <= server_state->commit_index; ++i) {
        RAFT_LOG_ENTRY entry = {0};
        if (WooFGet(RAFT_LOG_ENTRIES_WOOF, &entry, i) < 0) {
            return -1;
        }

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
                return -1;
            }
            log_debug("mapped index %lu to topic %s[%lu]", i, entry.topic_name, seq);
        }
        server_state->last_checked_committed_entry = i;
    }
    return 0;
}

int h_append_entries(WOOF* wf, unsigned long seq_no, void* ptr) {
    RAFT_APPEND_ENTRIES_ARG* request = (RAFT_APPEND_ENTRIES_ARG*)ptr;
    RAFT_LOG_ENTRY* entries = (RAFT_LOG_ENTRY*)(ptr + sizeof(RAFT_APPEND_ENTRIES_ARG));
    log_set_tag("h_append_entries");
    log_set_level(RAFT_LOG_INFO);
    // log_set_level(RAFT_LOG_DEBUG);
    log_set_output(stdout);

    uint64_t ts_received = get_milliseconds();

    // get the server's current term
    raft_lock(RAFT_LOCK_SERVER);
    RAFT_SERVER_STATE server_state = {0};
    if (WooFGet(RAFT_SERVER_STATE_WOOF, &server_state, 0) < 0) {
        log_error("failed to get the server's latest state");
        raft_unlock(RAFT_LOCK_SERVER);
        exit(1);
    }
    int warning_latency = server_state.timeout_min / 2;

    RAFT_APPEND_ENTRIES_RESULT result = {0};
    result.seqno = seq_no;
    result.ack_seq = request->ack_seq;
    memcpy(result.server_woof, server_state.woof_name, RAFT_NAME_LENGTH);

    int m_id = member_id(request->leader_woof, server_state.member_woofs);
    if (m_id == -1 || m_id >= server_state.members) {
        log_debug("disregard a request from a server not in the config");
        log_debug("request->leader_woof: %s", request->leader_woof);
        raft_unlock(RAFT_LOCK_SERVER);
        return 1;
    } else if (request->term < server_state.current_term) {
        log_warn("received a previous request [%lu]", seq_no);
        // fail the request from lower term
        result.term = server_state.current_term;
        result.success = 0;
        unsigned long last_entry_seqno = WooFGetLatestSeqno(RAFT_LOG_ENTRIES_WOOF);
        if (WooFInvalid(last_entry_seqno)) {
            log_error("failed to get the latest seqno of %s", RAFT_LOG_ENTRIES_WOOF);
            raft_unlock(RAFT_LOCK_SERVER);
            exit(1);
        }
        result.last_entry_seq = last_entry_seqno;
    } else {
        if (request->term == server_state.current_term && server_state.role == RAFT_LEADER) {
            log_error("fatal error: receiving append_entries request at term %" PRIu64 " while being a leader",
                      server_state.current_term);
            raft_unlock(RAFT_LOCK_SERVER);
            exit(1);
        }
        if (server_state.role == RAFT_OBSERVER) {
            if (request->term > server_state.current_term) {
                log_debug("observer enters term %" PRIu64 "", request->term);
                server_state.current_term = request->term;
                strcpy(server_state.current_leader, request->leader_woof);
                unsigned long seq = WooFPut(RAFT_SERVER_STATE_WOOF, NULL, &server_state);
                if (WooFInvalid(seq)) {
                    log_error("failed to enter term %" PRIu64 "", request->term);
                    raft_unlock(RAFT_LOCK_SERVER);
                    exit(1);
                }
            }
        } else if (request->term > server_state.current_term ||
                   (request->term == server_state.current_term && server_state.role != RAFT_FOLLOWER)) {
            // fall back to follower
            log_debug("request term %" PRIu64 " is higher, fall back to follower", request->term);
            if (request->term > server_state.current_term) {
                server_state.current_term = request->term;
                memset(server_state.voted_for, 0, RAFT_NAME_LENGTH);
            }
            server_state.role = RAFT_FOLLOWER;
            strcpy(server_state.current_leader, request->leader_woof);
            unsigned long seq = WooFPut(RAFT_SERVER_STATE_WOOF, NULL, &server_state);
            if (WooFInvalid(seq)) {
                log_error("failed to fall back to follower at term %" PRIu64 "", request->term);
                raft_unlock(RAFT_LOCK_SERVER);
                exit(1);
            }
            log_info("state changed at term %" PRIu64 ": FOLLOWER", server_state.current_term);
            RAFT_HEARTBEAT heartbeat = {0};
            heartbeat.term = server_state.current_term;
            heartbeat.timestamp = get_milliseconds();
            seq = WooFPut(RAFT_HEARTBEAT_WOOF, NULL, &heartbeat);
            if (WooFInvalid(seq)) {
                log_error("failed to put a heartbeat when falling back to follower");
                raft_unlock(RAFT_LOCK_SERVER);
                exit(1);
            }
            RAFT_TIMEOUT_CHECKER_ARG timeout_checker_arg = {0};
            timeout_checker_arg.timeout_value =
                random_timeout(get_milliseconds(), server_state.timeout_min, server_state.timeout_max);
            seq = WooFPut(RAFT_TIMEOUT_CHECKER_WOOF, "h_timeout_checker", &timeout_checker_arg);
            if (WooFInvalid(seq)) {
                log_error("failed to start the timeout checker");
                raft_unlock(RAFT_LOCK_SERVER);
                exit(1);
            }
        }
        // it's a valid append_entries request, treat as a heartbeat from leader
        RAFT_HEARTBEAT heartbeat = {0};
        heartbeat.term = server_state.current_term;
        heartbeat.timestamp = get_milliseconds();
        unsigned long seq = WooFPut(RAFT_HEARTBEAT_WOOF, NULL, &heartbeat);
        if (WooFInvalid(seq)) {
            log_error("failed to put a new heartbeat from term %" PRIu64 "", request->term);
            raft_unlock(RAFT_LOCK_SERVER);
            exit(1);
        }
        log_debug("received a heartbeat [%lu]", seq_no);

        // check if the previous log matches with the leader
        unsigned long last_entry_seqno = WooFGetLatestSeqno(RAFT_LOG_ENTRIES_WOOF);
        if (last_entry_seqno < request->prev_log_index) {
            log_warn("no log entry exists at prev_log_index %" PRIu64 ", latest: %lu",
                     request->prev_log_index,
                     last_entry_seqno);
            result.term = request->term;
            result.success = 0;
            result.last_entry_seq = last_entry_seqno;
        } else {
            // read the previous log entry
            RAFT_LOG_ENTRY previous_entry = {0};
            if (request->prev_log_index > 0) {
                if (WooFGet(RAFT_LOG_ENTRIES_WOOF, &previous_entry, request->prev_log_index) < 0) {
                    log_error("failed to get log entry at prev_log_index %" PRIu64 "", request->prev_log_index);
                    raft_unlock(RAFT_LOCK_SERVER);
                    exit(1);
                }
            }
            // term doesn't match
            if (request->prev_log_index > 0 && previous_entry.term != request->prev_log_term) {
                log_warn("previous log entry at prev_log_index %lu doesn't match request->prev_log_term %" PRIu64
                         ": %" PRIu64 " [%" PRIu64 "]",
                         request->prev_log_index,
                         request->prev_log_term,
                         previous_entry.term,
                         seq_no);
                result.term = request->term;
                result.success = 0;
                result.last_entry_seq = request->prev_log_index - 1;
            } else {
                // if the server has more entries that conflict with the leader, delete them
                if (last_entry_seqno > request->prev_log_index) {
                    // TODO: check if the entries after prev_log_index match
                    // if so, we don't need to call WooFTruncate()
                    if (WooFTruncate(RAFT_LOG_ENTRIES_WOOF, request->prev_log_index) < 0) {
                        log_error("failed to truncate log entries woof");
                        raft_unlock(RAFT_LOCK_SERVER);
                        exit(1);
                    }
                    log_warn(
                        "log truncated from %" PRIu64 " to %" PRIu64 "", last_entry_seqno, request->prev_log_index);
                    if (server_state.commit_index > request->prev_log_index) {
                        server_state.commit_index = request->prev_log_index;
                    }
                }
                // appending entries
                raft_lock(RAFT_LOCK_LOG);
                int i;
                for (i = 0; i < request->num_entries; ++i) {
                    // log_debug("processing entry[%d]", i);

#ifdef PROFILING
                    printf("RAFT_PROFILE written->replicated: %lu replicated->received: %lu\n",
                           request->ts_replicated - entries[i].ts_written,
                           ts_received - request->ts_replicated);
#endif
                    unsigned long seq = WooFPut(RAFT_LOG_ENTRIES_WOOF, NULL, &entries[i]);
                    if (WooFInvalid(seq)) {
                        log_error("failed to append entries[%d]", i);
                        raft_unlock(RAFT_LOCK_SERVER);
                        raft_unlock(RAFT_LOCK_LOG);
                        exit(1);
                    }
                    // if this entry is a config entry, update server config
                    if (entries[i].is_config != RAFT_CONFIG_ENTRY_NOT) {
                        int new_members;
                        char new_member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH];
                        if (decode_config(entries[i].data.val, &new_members, new_member_woofs) < 0) {
                            log_error("failed to decode config from entry[%d]", i);
                            raft_unlock(RAFT_LOCK_SERVER);
                            raft_unlock(RAFT_LOCK_LOG);
                            exit(1);
                        }
                        server_state.members = new_members;
                        server_state.last_config_seqno = seq;
                        if (entries[i].is_config == RAFT_CONFIG_ENTRY_JOINT) {
                            server_state.current_config = RAFT_CONFIG_STATUS_JOINT;
                        } else if (entries[i].is_config == RAFT_CONFIG_ENTRY_NEW) {
                            server_state.current_config = RAFT_CONFIG_STATUS_STABLE;
                        }
                        memcpy(server_state.member_woofs,
                               new_member_woofs,
                               (RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS) * RAFT_NAME_LENGTH);
                        // if the observer is in the new config, start as follower
                        if (server_state.role == RAFT_OBSERVER &&
                            member_id(server_state.woof_name, server_state.member_woofs) < server_state.members) {
                            server_state.role = RAFT_FOLLOWER;
                            RAFT_HEARTBEAT heartbeat = {0};
                            heartbeat.term = server_state.current_term;
                            heartbeat.timestamp = get_milliseconds();
                            seq = WooFPut(RAFT_HEARTBEAT_WOOF, NULL, &heartbeat);
                            if (WooFInvalid(seq)) {
                                log_error("failed to put a heartbeat when falling back to follower");
                                raft_unlock(RAFT_LOCK_SERVER);
                                raft_unlock(RAFT_LOCK_LOG);
                                exit(1);
                            }
                            RAFT_TIMEOUT_CHECKER_ARG timeout_checker_arg = {0};
                            timeout_checker_arg.timeout_value =
                                random_timeout(get_milliseconds(), server_state.timeout_min, server_state.timeout_max);
                            seq = WooFPut(RAFT_TIMEOUT_CHECKER_WOOF, "h_timeout_checker", &timeout_checker_arg);
                            if (WooFInvalid(seq)) {
                                log_error("failed to start the timeout checker");
                                raft_unlock(RAFT_LOCK_SERVER);
                                raft_unlock(RAFT_LOCK_LOG);
                                exit(1);
                            }
                            log_info("the server was observing but now is in the new config, promoted to FOLLOWER");
                        }
                        unsigned long server_state_seq = WooFPut(RAFT_SERVER_STATE_WOOF, NULL, &server_state);
                        if (WooFInvalid(server_state_seq)) {
                            log_error("failed to update server config at term %" PRIu64 "", server_state.current_term);
                            raft_unlock(RAFT_LOCK_SERVER);
                            raft_unlock(RAFT_LOCK_LOG);
                            exit(1);
                        }
                        if (entries[i].is_config == RAFT_CONFIG_ENTRY_JOINT) {
                            log_info("start using joint config with %d members at term %" PRIu64 "",
                                     server_state.members,
                                     server_state.current_term);
                        } else if (entries[i].is_config == RAFT_CONFIG_ENTRY_NEW) {
                            log_info("start using new config with %d members at term %" PRIu64 "",
                                     server_state.members,
                                     server_state.current_term);
                        }
                    }
                }
                unsigned long last_entry_seq = WooFGetLatestSeqno(RAFT_LOG_ENTRIES_WOOF);
                raft_unlock(RAFT_LOCK_LOG);
                if (WooFInvalid(last_entry_seq)) {
                    log_error("failed to get the latest seqno from", RAFT_LOG_ENTRIES_WOOF);
                    raft_unlock(RAFT_LOCK_SERVER);
                    exit(1);
                }
                result.term = request->term;
                result.success = 1;
                result.last_entry_seq = last_entry_seq;
                if (i > 0) {
                    log_debug("appended %d entries for request [%lu], last_entry_seq: %lu", i, seq_no, last_entry_seq);
                }

                // check if there's new commit_index
                if (request->leader_commit > server_state.commit_index) {
                    // commit_index = min(leader_commit, index of last new entry)
                    if (invoke_handler_and_map_topic_index(&server_state) < 0) {
                        log_error("failed to map committed log index");
                        raft_unlock(RAFT_LOCK_SERVER);
                        exit(1);
                    }
                    server_state.commit_index = request->leader_commit;
                    if (last_entry_seq < server_state.commit_index) {
                        server_state.commit_index = last_entry_seq;
                    }
                    unsigned long server_state_seq = WooFPut(RAFT_SERVER_STATE_WOOF, NULL, &server_state);
                    if (WooFInvalid(server_state_seq)) {
                        log_error("failed to update commit_index to %" PRIu64 " at term %" PRIu64 "",
                                  server_state.commit_index,
                                  server_state.current_term);
                        raft_unlock(RAFT_LOCK_SERVER);
                        exit(1);
                    }
                    log_debug("updated commit_index to %" PRIu64 " at term %" PRIu64 "",
                              server_state.commit_index,
                              server_state.current_term);
                }
            }
        }
    }
    raft_unlock(RAFT_LOCK_SERVER);

    // return the request
    char leader_result_woof[RAFT_NAME_LENGTH];
    sprintf(leader_result_woof, "%s/%s", request->leader_woof, RAFT_APPEND_ENTRIES_RESULT_WOOF);
    result.ts_received = ts_received;
    uint64_t result_begin = get_milliseconds();
    unsigned long seq = WooFPut(leader_result_woof, NULL, &result);
    if (WooFInvalid(seq)) {
        log_warn("failed to return the h_append_entries result to %s", leader_result_woof);
    }
    log_debug("returned result to %s [%lu]", leader_result_woof, seq);
    if (get_milliseconds() - ts_received > 500) {
        log_warn("h_append_entries took %lu ms\n", get_milliseconds() - ts_received);
    }
    return 1;
}
