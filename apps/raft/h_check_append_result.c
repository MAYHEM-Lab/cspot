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

int h_check_append_result(WOOF* wf, unsigned long seq_no, void* ptr) {
    log_set_tag("h_check_append_result");
    log_set_level(RAFT_LOG_INFO);
    // log_set_level(RAFT_LOG_DEBUG);
    log_set_output(stdout);
    uint64_t begin = get_milliseconds();
    RAFT_CHECK_APPEND_RESULT_ARG arg = {0};
    if (monitor_cast(ptr, &arg, sizeof(RAFT_CHECK_APPEND_RESULT_ARG)) < 0) {
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
        log_debug(
            "not a leader at term %" PRIu64 " anymore, current term: %" PRIu64 "", arg.term, server_state.current_term);
        monitor_exit(ptr);
        monitor_join();
        return 1;
    }

    // check previous append_entries_result
    unsigned long last_append_result_seqno = WooFGetLatestSeqno(RAFT_APPEND_ENTRIES_RESULT_WOOF);
    if (WooFInvalid(last_append_result_seqno)) {
        log_error("failed to get the latest seqno from %s", RAFT_APPEND_ENTRIES_RESULT_WOOF);
        exit(1);
    }
    int count = last_append_result_seqno - arg.last_seen_result_seqno;
    if (count != 0) {
        log_debug(
            "checking %d results from %lu to %lu", count, arg.last_seen_result_seqno + 1, last_append_result_seqno);
    }
    unsigned long result_seq;
    for (result_seq = arg.last_seen_result_seqno + 1; result_seq <= last_append_result_seqno; ++result_seq) {
        RAFT_APPEND_ENTRIES_RESULT result = {0};
        if (WooFGet(RAFT_APPEND_ENTRIES_RESULT_WOOF, &result, result_seq) < 0) {
            log_error("failed to get append_entries result at %lu", result_seq);
            exit(1);
        }
        if (RAFT_SAMPLING_RATE > 0 && (result_seq % RAFT_SAMPLING_RATE == 0)) {
            log_debug("request %lu took %" PRIu64 "ms to receive response",
                      result_seq,
                      get_milliseconds() - result.request_created_ts);
        }
        int result_member_id = member_id(result.server_woof, server_state.member_woofs);
        if (result_member_id == -1) {
            log_warn("received a result not in current config: %s", result.server_woof);
            int k;
            for (k = 0; k < 3; ++k) {
                log_warn("server_state.member_woofs[%d]: %s", k, server_state.member_woofs[k]);
            }
            arg.last_seen_result_seqno = result_seq;
            continue;
        }
        if (result_member_id < server_state.members && result.term > server_state.current_term) {
            // fall back to follower
            log_debug("request term %" PRIu64 " is higher, fall back to follower", result.term);
            server_state.current_term = result.term;
            server_state.role = RAFT_FOLLOWER;
            strcpy(server_state.current_leader, result.server_woof);
            memset(server_state.voted_for, 0, RAFT_NAME_LENGTH);
            unsigned long seq = WooFPut(RAFT_SERVER_STATE_WOOF, NULL, &server_state);
            if (WooFInvalid(seq)) {
                log_error("failed to fall back to follower at term %" PRIu64 "", result.term);
                exit(1);
            }
            log_info("state changed at term %" PRIu64 ": FOLLOWER", server_state.current_term);
            RAFT_HEARTBEAT heartbeat = {0};
            heartbeat.term = server_state.current_term;
            heartbeat.timestamp = get_milliseconds();
            seq = WooFPut(RAFT_HEARTBEAT_WOOF, NULL, &heartbeat);
            if (WooFInvalid(seq)) {
                log_error("failed to put a heartbeat when falling back to follower");
                exit(1);
            }
            RAFT_TIMEOUT_CHECKER_ARG timeout_checker_arg = {0};
            timeout_checker_arg.timeout_value =
                random_timeout(get_milliseconds(), server_state.timeout_min, server_state.timeout_max);
            seq =
                monitor_put(RAFT_MONITOR_NAME, RAFT_TIMEOUT_CHECKER_WOOF, "h_timeout_checker", &timeout_checker_arg, 1);
            if (WooFInvalid(seq)) {
                log_error("failed to start the timeout checker");
                exit(1);
            }
            monitor_exit(ptr);
            monitor_join();
            return 1;
        }
        if (result.term == arg.term) {
            server_state.next_index[result_member_id] = result.last_entry_seq + 1;
            server_state.acked_request_seq[result_member_id] = result.ack_seq;
            if (result.success) {
                server_state.match_index[result_member_id] = result.last_entry_seq;
            }
        }
        arg.last_seen_result_seqno = result_seq;
    }

    unsigned long seq = WooFPut(RAFT_SERVER_STATE_WOOF, NULL, &server_state);
    if (WooFInvalid(seq)) {
        log_error("failed to update server state");
        exit(1);
    }

    monitor_exit(ptr);
    seq = monitor_put(RAFT_MONITOR_NAME, RAFT_CHECK_APPEND_RESULT_WOOF, "h_check_append_result", &arg, 1);
    if (WooFInvalid(seq)) {
        log_error("failed to queue the next h_check_append_result handler");
        exit(1);
    }

    monitor_join();
    // printf("handler h_check_append_result took %lu\n", get_milliseconds() - begin);
    return 1;
}
