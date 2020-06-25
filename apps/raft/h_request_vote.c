#include "monitor.h"
#include "raft.h"
#include "raft_utils.h"
#include "woofc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

int h_request_vote(WOOF* wf, unsigned long seq_no, void* ptr) {
    log_set_tag("request_vote");
    log_set_level(RAFT_LOG_INFO);
    log_set_level(RAFT_LOG_DEBUG);
    log_set_output(stdout);

    RAFT_REQUEST_VOTE_ARG request = {0};
    if (monitor_cast(ptr, &request) < 0) {
        log_error("failed to monitor_cast");
        exit(1);
    }
    seq_no = monitor_seqno(ptr);

    // get the server's current term
    RAFT_SERVER_STATE server_state;
    if (get_server_state(&server_state) < 0) {
        log_error("failed to get the server state");
        exit(1);
    }

    if (server_state.role == RAFT_SHUTDOWN) {
        log_debug("server already shutdown");
        monitor_exit(ptr);
        return 1;
    }

    unsigned long i;
    RAFT_REQUEST_VOTE_RESULT result = {0};
    result.request_created_ts = request.created_ts;
    result.candidate_vote_pool_seqno = request.candidate_vote_pool_seqno;
    // deny the request if not a member
    int m_id = member_id(request.candidate_woof, server_state.member_woofs);
    if (m_id == -1 || m_id >= server_state.members) {
        result.term = 0; // result term 0 means shutdown
        result.granted = 0;
        log_debug("rejected a vote request [%lu] from a candidate not in the config", seq_no);
    } else if (request.term < server_state.current_term) { // current term is higher than the request
        result.term = server_state.current_term;           // server term will always be greater than reviewing term
        result.granted = 0;
        log_debug("rejected a vote request [%lu] from lower term %lu at term %lu",
                  seq_no,
                  request.term,
                  server_state.current_term);
    } else {
        if (request.term > server_state.current_term) {
            // fallback to follower
            log_debug("request [%lu] term %lu is higher, fall back to follower", seq_no, request.term);
            server_state.current_term = request.term;
            server_state.role = RAFT_FOLLOWER;
            strcpy(server_state.current_leader, request.candidate_woof);
            memset(server_state.voted_for, 0, RAFT_NAME_LENGTH);
            unsigned long seq = WooFPut(RAFT_SERVER_STATE_WOOF, NULL, &server_state);
            if (WooFInvalid(seq)) {
                log_error("failed to fall back to follower at term %lu", request.term);
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
        }

        result.term = server_state.current_term;
        if (server_state.voted_for[0] == 0 || strcmp(server_state.voted_for, request.candidate_woof) == 0) {
            // check if the log is up-to-date
            unsigned long latest_log_entry = WooFGetLatestSeqno(RAFT_LOG_ENTRIES_WOOF);
            if (WooFInvalid(latest_log_entry)) {
                log_error("failed to get the latest seqno from %s", RAFT_LOG_ENTRIES_WOOF);
                exit(1);
            }
            RAFT_LOG_ENTRY last_log_entry = {0};
            if (latest_log_entry > 0) {
                if (WooFGet(RAFT_LOG_ENTRIES_WOOF, &last_log_entry, latest_log_entry) < 0) {
                    log_error(
                        "failed to get the latest log entry %lu from %s", latest_log_entry, RAFT_LOG_ENTRIES_WOOF);
                    exit(1);
                }
            }
            if (latest_log_entry > 0 && last_log_entry.term > request.last_log_term) {
                // the server has more up-to-dated entries than the candidate
                result.granted = 0;
                log_debug("rejected vote [%lu] from server with outdated log (last entry at term %lu)",
                          seq_no,
                          request.last_log_term);
            } else if (latest_log_entry > 0 && last_log_entry.term == request.last_log_term &&
                       latest_log_entry > request.last_log_index) {
                // both have same term but the server has more entries
                result.granted = 0;
                log_debug("rejected vote [%lu] from server with outdated log (last entry at index %lu)",
                          seq_no,
                          request.last_log_index);
            } else {
                // the candidate has more up-to-dated log entries
                memcpy(server_state.voted_for, request.candidate_woof, RAFT_NAME_LENGTH);
                unsigned long seq = WooFPut(RAFT_SERVER_STATE_WOOF, NULL, &server_state);
                if (WooFInvalid(seq)) {
                    log_error("failed to update voted_for at term %lu", server_state.current_term);
                    exit(1);
                }
                result.granted = 1;
                strcpy(result.granter, server_state.woof_name);
                log_debug(
                    "granted vote [%lu] at term %lu to %s", seq_no, server_state.current_term, request.candidate_woof);
            }
        } else {
            result.granted = 0;
            log_debug("rejected vote [%lu] from since already voted at term %lu", seq_no, server_state.current_term);
        }
    }
    // return the request
    char candidate_monitor[RAFT_NAME_LENGTH];
    char candidate_result_woof[RAFT_NAME_LENGTH];
    sprintf(candidate_monitor, "%s/%s", request.candidate_woof, RAFT_MONITOR_NAME);
    sprintf(candidate_result_woof, "%s/%s", request.candidate_woof, RAFT_REQUEST_VOTE_RESULT_WOOF);
    unsigned long seq = monitor_remote_put(candidate_monitor, candidate_result_woof, "h_count_vote", &result, 0);
    if (WooFInvalid(seq)) {
        log_warn("failed to return the vote result to %s", candidate_result_woof);
    }

    monitor_exit(ptr);
    return 1;
}
