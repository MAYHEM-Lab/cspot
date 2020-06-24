#include "monitor.h"
#include "raft.h"
#include "raft_utils.h"
#include "woofc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>


int h_append_entries(WOOF* wf, unsigned long seq_no, void* ptr) {
    RAFT_APPEND_ENTRIES_ARG* request = (RAFT_APPEND_ENTRIES_ARG*)monitor_cast(ptr);
    seq_no = monitor_seqno(ptr);

    log_set_tag("append_entries");
    log_set_level(RAFT_LOG_INFO);
    // log_set_level(RAFT_LOG_DEBUG);
    log_set_output(stdout);

    // get the server's current term
    RAFT_SERVER_STATE server_state = {0};
    if (get_server_state(&server_state) < 0) {
        log_error("failed to get the server's latest state");
        free(request);
        exit(1);
    }

    RAFT_APPEND_ENTRIES_RESULT result = {0};
    result.request_created_ts = request->created_ts;
    result.seqno = seq_no;
    memcpy(result.server_woof, server_state.woof_name, RAFT_NAME_LENGTH);
    if (RAFT_WARNING_LATENCY > 0 && get_milliseconds() - request->created_ts > RAFT_WARNING_LATENCY) {
        log_warn("request %lu took %lums to receive", seq_no, get_milliseconds() - request->created_ts);
    }
    int m_id = member_id(request->leader_woof, server_state.member_woofs);
    if (m_id == -1 || m_id >= server_state.members) {
        log_debug("disregard a request from a server not in the config");
        free(request);
        monitor_exit(ptr);
        return 1;
    } else if (request->term < server_state.current_term) {
        // fail the request from lower term
        result.term = server_state.current_term;
        result.success = 0;
        log_debug("received a previous request [%lu]", seq_no);
    } else {
        if (request->term == server_state.current_term && server_state.role == RAFT_LEADER) {
            log_error("fatal error: receiving append_entries request at term %lu while being a leader",
                      server_state.current_term);
            free(request);
            exit(1);
        }
        if (server_state.role == RAFT_OBSERVER) {
            if (request->term > server_state.current_term) {
                log_debug("observer enters term %lu", request->term);
                server_state.current_term = request->term;
                strcpy(server_state.current_leader, request->leader_woof);
                unsigned long seq = WooFPut(RAFT_SERVER_STATE_WOOF, NULL, &server_state);
                if (WooFInvalid(seq)) {
                    log_error("failed to enter term %lu", request->term);
                    free(request);
                    exit(1);
                }
            }
        } else if (request->term > server_state.current_term ||
                   (request->term == server_state.current_term && server_state.role != RAFT_FOLLOWER)) {
            // fall back to follower
            log_debug("request term %lu is higher, fall back to follower", request->term);
            if (request->term > server_state.current_term) {
                server_state.current_term = request->term;
                memset(server_state.voted_for, 0, RAFT_NAME_LENGTH);
            }
            server_state.role = RAFT_FOLLOWER;
            strcpy(server_state.current_leader, request->leader_woof);
            unsigned long seq = WooFPut(RAFT_SERVER_STATE_WOOF, NULL, &server_state);
            if (WooFInvalid(seq)) {
                log_error("failed to fall back to follower at term %lu", request->term);
                free(request);
                exit(1);
            }
            log_info("state changed at term %lu: FOLLOWER", server_state.current_term);
            RAFT_HEARTBEAT heartbeat = {0};
            heartbeat.term = server_state.current_term;
            heartbeat.timestamp = get_milliseconds();
            seq = WooFPut(RAFT_HEARTBEAT_WOOF, NULL, &heartbeat);
            if (WooFInvalid(seq)) {
                log_error("failed to put a heartbeat when falling back to follower");
                free(request);
                exit(1);
            }
            RAFT_TIMEOUT_CHECKER_ARG timeout_checker_arg = {0};
            timeout_checker_arg.timeout_value = random_timeout(get_milliseconds());
            seq = monitor_put(RAFT_MONITOR_NAME, RAFT_TIMEOUT_CHECKER_WOOF, "h_timeout_checker", &timeout_checker_arg, 1);
            if (WooFInvalid(seq)) {
                log_error("failed to start the timeout checker");
                free(request);
                exit(1);
            }
        }
        // it's a valid append_entries request, treat as a heartbeat from leader
        RAFT_HEARTBEAT heartbeat = {0};
        heartbeat.term = server_state.current_term;
        heartbeat.timestamp = get_milliseconds();
        unsigned long seq = WooFPut(RAFT_HEARTBEAT_WOOF, NULL, &heartbeat);
        if (WooFInvalid(seq)) {
            log_error("failed to put a new heartbeat from term %lu", request->term);
            free(request);
            exit(1);
        }
        // log_debug("received a heartbeat [%lu]", seq_no);

        // check if the previous log matches with the leader
        unsigned long last_entry_seqno = WooFGetLatestSeqno(RAFT_LOG_ENTRIES_WOOF);
        if (last_entry_seqno < request->prev_log_index) {
            log_debug(
                "no log entry exists at prev_log_index %lu, latest: %lu", request->prev_log_index, last_entry_seqno);
            result.term = request->term;
            result.success = 0;
            result.last_entry_seq = last_entry_seqno;
        } else {
            // read the previous log entry
            RAFT_LOG_ENTRY previous_entry = {0};
            if (request->prev_log_index > 0) {
                if (WooFGet(RAFT_LOG_ENTRIES_WOOF, &previous_entry, request->prev_log_index) < 0) {
                    log_error("failed to get log entry at prev_log_index %lu", request->prev_log_index);
                    free(request);
                    exit(1);
                }
            }
            // term doesn't match
            if (request->prev_log_index > 0 && previous_entry.term != request->prev_log_term) {
                log_debug(
                    "previous log entry at prev_log_index %lu doesn't match request->prev_log_term %lu: %lu [%lu]",
                    request->prev_log_index,
                    request->prev_log_term,
                    previous_entry.term,
                    seq_no);
                result.term = request->term;
                result.success = 0;
                result.last_entry_seq = last_entry_seqno;
            } else {
                // if the server has more entries that conflict with the leader, delete them
                if (last_entry_seqno > request->prev_log_index) {
                    // TODO: check if the entries after prev_log_index match
                    // if so, we don't need to call WooFTruncate()
                    if (WooFTruncate(RAFT_LOG_ENTRIES_WOOF, request->prev_log_index) < 0) {
                        log_error("failed to truncate log entries woof");
                        free(request);
                        exit(1);
                    }
                    log_debug("log truncated to %lu", request->prev_log_index);
                }
                // appending entries
                int i;
                for (i = 0; i < RAFT_MAX_ENTRIES_PER_REQUEST; ++i) {
                    if (request->entries[i].term == 0) {
                        break; // no entry, finish append
                    }
                    unsigned long seq = WooFPut(RAFT_LOG_ENTRIES_WOOF, NULL, &request->entries[i]);
                    if (WooFInvalid(seq)) {
                        log_error("failed to append entries[%d]", i);
                        free(request);
                        exit(1);
                    }
                    // if this entry is a config entry, update server config
                    if (request->entries[i].is_config != RAFT_CONFIG_ENTRY_NOT) {
                        int new_members;
                        char new_member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH];
                        if (decode_config(request->entries[i].data.val, &new_members, new_member_woofs) < 0) {
                            log_error("failed to decode config from entry[%lu]", i);
                            free(request);
                            exit(1);
                        }
                        server_state.members = new_members;
                        server_state.last_config_seqno = seq;
                        if (request->entries[i].is_config == RAFT_CONFIG_ENTRY_JOINT) {
                            server_state.current_config = RAFT_CONFIG_STATUS_JOINT;
                        } else if (request->entries[i].is_config == RAFT_CONFIG_ENTRY_NEW) {
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
                                free(request);
                                exit(1);
                            }
                            RAFT_TIMEOUT_CHECKER_ARG timeout_checker_arg = {0};
                            timeout_checker_arg.timeout_value = random_timeout(get_milliseconds());
                            seq = monitor_put(RAFT_MONITOR_NAME,
                                              RAFT_TIMEOUT_CHECKER_WOOF,
                                              "h_timeout_checker",
                                              &timeout_checker_arg, 1);
                            if (WooFInvalid(seq)) {
                                log_error("failed to start the timeout checker");
                                free(request);
                                exit(1);
                            }
                            log_info("the server was observing but now is in the new config, promoted to FOLLOWER");
                        }
                        unsigned long server_state_seq = WooFPut(RAFT_SERVER_STATE_WOOF, NULL, &server_state);
                        if (WooFInvalid(server_state_seq)) {
                            log_error("failed to update server config at term %lu", server_state.current_term);
                            free(request);
                            exit(1);
                        }
                        if (request->entries[i].is_config == RAFT_CONFIG_ENTRY_JOINT) {
                            log_info("start using joint config with %d members at term %lu",
                                     server_state.members,
                                     server_state.current_term);
                        } else if (request->entries[i].is_config == RAFT_CONFIG_ENTRY_NEW) {
                            log_info("start using new config with %d members at term %lu",
                                     server_state.members,
                                     server_state.current_term);
                        }
                    }
                    // if it's a handler entry, invoke the handler
                    if (request->entries[i].is_handler) {
                        RAFT_LOG_HANDLER_ENTRY* handler_entry = (RAFT_LOG_HANDLER_ENTRY*)(&request->entries[i].data);
                        unsigned long invoked_seq =
                            WooFPut(RAFT_LOG_HANDLER_ENTRIES_WOOF, handler_entry->handler, handler_entry->ptr);
                        if (WooFInvalid(invoked_seq)) {
                            log_error("failed to invoke %s for appended handler entry", handler_entry->handler);
                            free(request);
                            exit(1);
                        }
                        log_debug("appended a handler entry and invoked the handler %s", handler_entry->handler);
                    }
                }
                unsigned long last_entry_seq = WooFGetLatestSeqno(RAFT_LOG_ENTRIES_WOOF);
                if (WooFInvalid(last_entry_seq)) {
                    log_error("failed to get the latest seqno from", RAFT_LOG_ENTRIES_WOOF);
                    free(request);
                    exit(1);
                }
                result.term = request->term;
                result.success = 1;
                result.last_entry_seq = last_entry_seq;
                if (i > 0) {
                    log_debug("appended %d entries for request [%lu]", i, seq_no);
                }

                // check if there's new commit_index
                if (request->leader_commit > server_state.commit_index) {
                    // commit_index = min(leader_commit, index of last new entry)
                    server_state.commit_index = request->leader_commit;
                    if (last_entry_seq < server_state.commit_index) {
                        server_state.commit_index = last_entry_seq;
                    }
                    unsigned long server_state_seq = WooFPut(RAFT_SERVER_STATE_WOOF, NULL, &server_state);
                    if (WooFInvalid(server_state_seq)) {
                        log_error("failed to update commit_index to %lu at term %lu",
                                  server_state.commit_index,
                                  server_state.current_term);
                        free(request);
                        exit(1);
                    }
                    log_debug("updated commit_index to %lu at term %lu",
                              server_state.commit_index,
                              server_state.current_term);
                }
            }
        }
    }

    if (RAFT_WARNING_LATENCY > 0 && get_milliseconds() - request->created_ts > RAFT_WARNING_LATENCY) {
        log_warn("request %lu took %lums to process", seq_no, get_milliseconds() - request->created_ts);
    }

    monitor_exit(ptr);
    // return the request
    char leader_result_woof[RAFT_NAME_LENGTH];
    sprintf(leader_result_woof, "%s/%s", request->leader_woof, RAFT_APPEND_ENTRIES_RESULT_WOOF);
    unsigned long seq = WooFPut(leader_result_woof, NULL, &result);
    if (WooFInvalid(seq)) {
        log_warn("failed to return the append_entries result to %s", leader_result_woof);
    }

    free(request);
    return 1;
}
