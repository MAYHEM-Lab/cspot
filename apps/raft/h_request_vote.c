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

int h_request_vote(WOOF* wf, unsigned long seq_no, void* ptr) {
    RAFT_REQUEST_VOTE_ARG* request = (RAFT_REQUEST_VOTE_ARG*)ptr;
    log_set_tag("request_vote");
    log_set_level(RAFT_LOG_INFO);
    // log_set_level(RAFT_LOG_DEBUG);
    log_set_output(stdout);
    WooFMsgCacheInit();

    uint64_t begin = get_milliseconds();
    log_debug("request vote from %s", request->candidate_woof);

    // get the server's current term
    raft_lock(RAFT_LOCK_SERVER);
    RAFT_SERVER_STATE server_state;
    if (WooFGet(RAFT_SERVER_STATE_WOOF, &server_state, 0) < 0) {
        log_error("failed to get the server state");
        WooFMsgCacheShutdown();
        raft_unlock(RAFT_LOCK_SERVER);
        exit(1);
    }

    if (server_state.role == RAFT_SHUTDOWN) {
        log_debug("server already shutdown");
        WooFMsgCacheShutdown();
        raft_unlock(RAFT_LOCK_SERVER);
        return 1;
    }

    unsigned long i;
    RAFT_REQUEST_VOTE_RESULT result = {0};
    result.candidate_vote_pool_seqno = request->candidate_vote_pool_seqno;
    // deny the request if not a member
    int m_id = member_id(request->candidate_woof, server_state.member_woofs);
    if (m_id == -1 || m_id >= server_state.members) {
        result.term = 0; // result term 0 means shutdown
        result.granted = 0;
        log_debug("rejected a vote request [%lu] from a candidate not in the config", seq_no);
    } else if (request->term < server_state.current_term) { // current term is higher than the request
        result.term = server_state.current_term;            // server term will always be greater than reviewing term
        result.granted = 0;
        log_debug("rejected a vote request [%lu] from lower term %" PRIu64 " at term %" PRIu64 "",
                  seq_no,
                  request->term,
                  server_state.current_term);
    } else {
        if (request->term > server_state.current_term) {
            // fallback to follower
            log_debug("request [%lu] term %" PRIu64 " is higher, fall back to follower", seq_no, request->term);
            server_state.current_term = request->term;
            server_state.role = RAFT_FOLLOWER;
            strcpy(server_state.current_leader, request->candidate_woof);
            memset(server_state.voted_for, 0, RAFT_NAME_LENGTH);
            unsigned long seq = WooFPut(RAFT_SERVER_STATE_WOOF, NULL, &server_state);
            if (WooFInvalid(seq)) {
                log_error("failed to fall back to follower at term %" PRIu64 "", request->term);
                WooFMsgCacheShutdown();
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
                WooFMsgCacheShutdown();
                raft_unlock(RAFT_LOCK_SERVER);
                exit(1);
            }
            RAFT_TIMEOUT_CHECKER_ARG timeout_checker_arg = {0};
            timeout_checker_arg.timeout_value =
                random_timeout(get_milliseconds(), server_state.timeout_min, server_state.timeout_max);
            seq = WooFPut(RAFT_TIMEOUT_CHECKER_WOOF, "h_timeout_checker", &timeout_checker_arg);
            if (WooFInvalid(seq)) {
                log_error("failed to start the timeout checker");
                WooFMsgCacheShutdown();
                raft_unlock(RAFT_LOCK_SERVER);
                exit(1);
            }
        }

        result.term = server_state.current_term;
        if (server_state.voted_for[0] == 0 || strcmp(server_state.voted_for, request->candidate_woof) == 0) {
            // check if the log is up-to-date
            raft_lock(RAFT_LOCK_LOG);
            unsigned long latest_log_entry = WooFGetLatestSeqno(RAFT_LOG_ENTRIES_WOOF);
            raft_unlock(RAFT_LOCK_LOG);
            if (WooFInvalid(latest_log_entry)) {
                log_error("failed to get the latest seqno from %s", RAFT_LOG_ENTRIES_WOOF);
                WooFMsgCacheShutdown();
                raft_unlock(RAFT_LOCK_SERVER);
                exit(1);
            }
            RAFT_LOG_ENTRY last_log_entry = {0};
            if (latest_log_entry > 0) {
                if (WooFGet(RAFT_LOG_ENTRIES_WOOF, &last_log_entry, latest_log_entry) < 0) {
                    log_error(
                        "failed to get the latest log entry %lu from %s", latest_log_entry, RAFT_LOG_ENTRIES_WOOF);
                    WooFMsgCacheShutdown();
                    raft_unlock(RAFT_LOCK_SERVER);
                    exit(1);
                }
            }
            if (latest_log_entry > 0 && last_log_entry.term > request->last_log_term) {
                // the server has more up-to-dated entries than the candidate
                result.granted = 0;
                log_debug("rejected vote [%lu] from server with outdated log (last entry at term %" PRIu64 ")",
                          seq_no,
                          request->last_log_term);
            } else if (latest_log_entry > 0 && last_log_entry.term == request->last_log_term &&
                       latest_log_entry > request->last_log_index) {
                // both have same term but the server has more entries
                result.granted = 0;
                log_debug("rejected vote [%lu] from server with outdated log (last entry at index %" PRIu64 ")",
                          seq_no,
                          request->last_log_index);
            } else {
                // the candidate has more up-to-dated log entries
                memcpy(server_state.voted_for, request->candidate_woof, RAFT_NAME_LENGTH);
                unsigned long seq = WooFPut(RAFT_SERVER_STATE_WOOF, NULL, &server_state);
                if (WooFInvalid(seq)) {
                    log_error("failed to update voted_for at term %" PRIu64 "", server_state.current_term);
                    WooFMsgCacheShutdown();
                    raft_unlock(RAFT_LOCK_SERVER);
                    exit(1);
                }
                result.granted = 1;
                strcpy(result.granter, server_state.woof_name);
                log_debug("granted vote [%lu] at term %" PRIu64 " to %s",
                          seq_no,
                          server_state.current_term,
                          request->candidate_woof);
            }
        } else {
            result.granted = 0;
            log_debug(
                "rejected vote [%lu] from since already voted at term %" PRIu64 "", seq_no, server_state.current_term);
        }
    }
    raft_unlock(RAFT_LOCK_SERVER);

    // return the request
    char candidate_result_woof[RAFT_NAME_LENGTH];
    sprintf(candidate_result_woof, "%s/%s", request->candidate_woof, RAFT_REQUEST_VOTE_RESULT_WOOF);
    unsigned long seq = WooFPut(candidate_result_woof, "h_count_vote", &result);
    if (WooFInvalid(seq)) {
        log_warn("failed to return the vote result to %s", candidate_result_woof);
    }

    // printf("handler h_request_vote took %lu\n", get_milliseconds() - begin);
    WooFMsgCacheShutdown();
    return 1;
}
