#include "monitor.h"
#include "raft.h"
#include "raft_utils.h"
#include "woofc.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int h_count_vote(WOOF* wf, unsigned long seq_no, void* ptr) {
    log_set_tag("count_vote");
    log_set_level(RAFT_LOG_INFO);
    // log_set_level(RAFT_LOG_DEBUG);
    log_set_output(stdout);

    RAFT_REQUEST_VOTE_RESULT result = {0};
    if (monitor_cast(ptr, &result, sizeof(RAFT_REQUEST_VOTE_RESULT)) < 0) {
        log_error("failed to monitor_cast");
        exit(1);
    }
    seq_no = monitor_seqno(ptr);

    RAFT_SERVER_STATE server_state = {0};
    if (get_server_state(&server_state) < 0) {
        log_error("failed to get the server state");
        exit(1);
    }

    // if result term is 0, it means the candidate is not in the leader config, shut it down
    if (result.term == 0) {
        server_state.role = RAFT_SHUTDOWN;
        unsigned long seq = WooFPut(RAFT_SERVER_STATE_WOOF, NULL, &server_state);
        if (WooFInvalid(seq)) {
            log_error("failed to shutdown");
            exit(1);
        }
        log_info("server not in the leader config anymore: SHUTDOWN");
        monitor_exit(ptr);
        return 1;
    }
    // server's term is higher than the vote's term, ignore it
    if (result.term < server_state.current_term) {
        log_debug("current term %" PRIu64 " is higher than vote's term %" PRIu64 ", ignore the election",
                  server_state.current_term,
                  result.term);
        monitor_exit(ptr);
        return 1;
    }
    // the server is already a leader at vote's term, ifnore the vote
    if (result.term == server_state.current_term && server_state.role == RAFT_LEADER) {
        log_debug("already a leader at term %" PRIu64 ", ignore the election", server_state.current_term);
        monitor_exit(ptr);
        return 1;
    }

    log_debug("counting from seqno %" PRIu64 " to %lu", result.candidate_vote_pool_seqno + 1, seq_no);
    // start counting the votes
    int granted_votes = 0;
    if (result.granted == 1) {
        log_debug("granted by %s at term %" PRIu64 "", result.granter, result.term);
        ++granted_votes;
    }
    RAFT_REQUEST_VOTE_RESULT vote = {0};
    unsigned long vote_seq;
    for (vote_seq = result.candidate_vote_pool_seqno + 1; vote_seq < seq_no; ++vote_seq) {
        if (WooFGet(RAFT_REQUEST_VOTE_RESULT_WOOF, &vote, vote_seq) < 0) {
            log_error("failed to get the vote result at seqno %lu", vote_seq);
            exit(1);
        }
        if (vote.granted == 1 && vote.term == result.term) {
            log_debug("granted by %s at term %" PRIu64 "", vote.granter, vote.term);
            ++granted_votes;
            if (granted_votes > server_state.members / 2) {
                break;
            }
        }
    }
    log_debug("counted %d granted votes for term %" PRIu64 "", granted_votes, result.term);

    // if the majority granted, promoted to leader
    if (granted_votes > server_state.members / 2) {
        unsigned long last_log_entry_seqno = WooFGetLatestSeqno(RAFT_LOG_ENTRIES_WOOF);
        if (WooFInvalid(last_log_entry_seqno)) {
            log_error("failed to get the latest seqno from %s", RAFT_LOG_ENTRIES_WOOF);
            exit(1);
        }
        server_state.current_term = result.term;
        server_state.role = RAFT_LEADER;
        memcpy(server_state.current_leader, server_state.woof_name, RAFT_NAME_LENGTH);
        memcpy(server_state.voted_for, server_state.woof_name, RAFT_NAME_LENGTH);
        int i;
        for (i = 0; i < RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS; ++i) {
            server_state.next_index[i] = last_log_entry_seqno + 1;
            server_state.match_index[i] = 0;
            server_state.last_sent_timestamp[i] = 0;
            server_state.last_sent_request_seq[i] = 0;
            server_state.acked_request_seq[i] = 0;
        }
        unsigned long seq = WooFPut(RAFT_SERVER_STATE_WOOF, NULL, &server_state);
        if (WooFInvalid(seq)) {
            log_error("failed to promote itself to leader");
            exit(1);
        }
        log_debug("promoted to leader for term %" PRIu64 "", result.term);
        log_info("state changed at term %" PRIu64 ": LEADER", server_state.current_term);

        // start replicate_entries handlers
        unsigned long last_append_result_seqno = WooFGetLatestSeqno(RAFT_APPEND_ENTRIES_RESULT_WOOF);
        if (WooFInvalid(last_append_result_seqno)) {
            log_error("failed to get the latest seqno from %s", RAFT_APPEND_ENTRIES_RESULT_WOOF);
            exit(1);
        }
        unsigned long last_client_put_result_seqno = WooFGetLatestSeqno(RAFT_CLIENT_PUT_RESULT_WOOF);
        if (WooFInvalid(last_client_put_result_seqno)) {
            log_error("failed to get the latest seqno from %s", RAFT_CLIENT_PUT_RESULT_WOOF);
            exit(1);
        }

        RAFT_REPLICATE_ENTRIES_ARG replicate_entries_arg = {0};
        replicate_entries_arg.term = server_state.current_term;
        replicate_entries_arg.last_ts = get_milliseconds();
        seq = monitor_put(
            RAFT_MONITOR_NAME, RAFT_REPLICATE_ENTRIES_WOOF, "h_replicate_entries", &replicate_entries_arg, 1);
        if (WooFInvalid(seq)) {
            log_error("failed to start h_replicate_entries handler");
            exit(1);
        }
        log_debug("started h_replicate_entries");
        RAFT_CHECK_APPEND_RESULT_ARG check_append_result_arg = {0};
        check_append_result_arg.term = server_state.current_term;
        check_append_result_arg.last_seen_result_seqno = last_append_result_seqno;
        seq = monitor_put(
            RAFT_MONITOR_NAME, RAFT_CHECK_APPEND_RESULT_WOOF, "h_check_append_result", &check_append_result_arg, 1);
        if (WooFInvalid(seq)) {
            log_error("failed to start h_check_append_result handler");
            exit(1);
        }
        log_debug("started h_check_append_result");
        RAFT_UPDATE_COMMIT_INDEX_ARG update_commit_index_arg = {0};
        update_commit_index_arg.term = server_state.current_term;
        seq = monitor_put(
            RAFT_MONITOR_NAME, RAFT_UPDATE_COMMIT_INDEX_WOOF, "h_update_commit_index", &update_commit_index_arg, 1);
        if (WooFInvalid(seq)) {
            log_error("failed to start h_update_commit_index handler");
            exit(1);
        }
        log_debug("started h_update_commit_index");
        RAFT_INVOKE_COMMITTED_ARG invoke_committed_arg = {0};
        invoke_committed_arg.term = server_state.current_term;
        invoke_committed_arg.last_checked_client_put_result_seqno = last_client_put_result_seqno;
        seq = monitor_put(
            RAFT_MONITOR_NAME, RAFT_INVOKE_COMMITTED_WOOF, "h_invoke_committed", &invoke_committed_arg, 1);
        if (WooFInvalid(seq)) {
            log_error("failed to start h_invoke_committed handler");
            exit(1);
        }
        log_debug("started h_invoke_committed");
    }

    log_debug(
        "request [%lu] took %" PRIu64 "ms to receive response", seq_no, get_milliseconds() - result.request_created_ts);
    monitor_exit(ptr);
    return 1;
}
