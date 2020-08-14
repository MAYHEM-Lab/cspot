#include "monitor.h"
#include "raft.h"
#include "raft_utils.h"
#include "woofc.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

int h_client_put(WOOF* wf, unsigned long seq_no, void* ptr) {
    log_set_tag("client_put");
    log_set_level(RAFT_LOG_INFO);
    // log_set_level(RAFT_LOG_DEBUG);
    log_set_output(stdout);

    RAFT_CLIENT_PUT_ARG arg = {0};
    if (monitor_cast(ptr, &arg) < 0) {
        log_error("failed to monitor_cast");
        exit(1);
    }
    seq_no = monitor_seqno(ptr);

    // get the server's current term
    RAFT_SERVER_STATE server_state = {0};
    if (get_server_state(&server_state) < 0) {
        log_error("failed to get the server state");
        exit(1);
    }
    int client_put_delay = server_state.timeout_min / 10;

    unsigned long last_request = WooFGetLatestSeqno(RAFT_CLIENT_PUT_REQUEST_WOOF);
    if (WooFInvalid(last_request)) {
        log_error("failed to get the latest seqno from %s", RAFT_CLIENT_PUT_RESULT_WOOF);
        exit(1);
    }

    unsigned long i;
    for (i = arg.last_seqno + 1; i <= last_request; ++i) {
        RAFT_CLIENT_PUT_REQUEST request = {0};
        if (WooFGet(RAFT_CLIENT_PUT_REQUEST_WOOF, &request, i) < 0) {
            log_error("failed to get client_put request at %lu", i);
            exit(1);
        }
        RAFT_CLIENT_PUT_RESULT result = {0};
        if (server_state.role != RAFT_LEADER) {
            result.redirected = 1;
            result.index = 0;
            result.term = server_state.current_term;
            memcpy(result.current_leader, server_state.current_leader, RAFT_NAME_LENGTH);
        } else {
            unsigned long entry_seqno;
            RAFT_LOG_ENTRY entry = {0};
            entry.term = server_state.current_term;
            memcpy(&entry.data, &request.data, sizeof(RAFT_DATA_TYPE));
            entry.is_config = RAFT_CONFIG_ENTRY_NOT;
            entry.is_handler = request.is_handler;
            entry_seqno = WooFPut(RAFT_LOG_ENTRIES_WOOF, NULL, &entry);
            if (WooFInvalid(entry_seqno)) {
                log_error("failed to append raft log");
                exit(1);
            }
            result.redirected = 0;
            result.index = entry_seqno;
            result.term = server_state.current_term;
            if (RAFT_SAMPLING_RATE > 0 && (entry_seqno % RAFT_SAMPLING_RATE == 0)) {
                log_debug("entry %lu was created at %" PRIu64 "", entry_seqno, request.created_ts);
            }
            // if it's a handler entry, invoke the handler
            if (request.is_handler) {
                RAFT_LOG_HANDLER_ENTRY* handler_entry = (RAFT_LOG_HANDLER_ENTRY*)(&request.data);
                unsigned long invoked_seq =
                    WooFPut(RAFT_LOG_HANDLER_ENTRIES_WOOF, handler_entry->handler, handler_entry->ptr);
                if (WooFInvalid(invoked_seq)) {
                    log_error("failed to invoke %s for appended handler entry", handler_entry->handler);
                    exit(1);
                }
                log_debug("appended a handler entry and invoked the handler %s", handler_entry->handler);
            }
        }
        // make sure the result's seq_no matches with request's seq_no
        unsigned long latest_result_seqno = WooFGetLatestSeqno(RAFT_CLIENT_PUT_RESULT_WOOF);
        if (latest_result_seqno < i - 1) {
            log_warn("client_put_result at %lu, client_put_request at %lu, padding %lu results",
                     latest_result_seqno,
                     i,
                     i - latest_result_seqno - 1);
            unsigned long j;
            for (j = latest_result_seqno + 1; j <= i - 1; ++j) {
                RAFT_CLIENT_PUT_RESULT result = {0};
                unsigned long result_seq = WooFPut(RAFT_CLIENT_PUT_RESULT_WOOF, NULL, &result);
                if (WooFInvalid(result_seq)) {
                    log_error("failed to put padded result");
                    exit(1);
                }
            }
        }

        unsigned long result_seq = WooFPut(RAFT_CLIENT_PUT_RESULT_WOOF, NULL, &result);
        while (WooFInvalid(result_seq)) {
            log_error("failed to write client_put_result");
            exit(1);
        }
        if (result_seq != i) {
            log_error("client_put_request seq_no %lu doesn't match result seq_no %lu", i, result_seq);
            exit(1);
        }

        arg.last_seqno = i;
    }
    monitor_exit(ptr);

    usleep(client_put_delay * 1000);
    unsigned long seq = monitor_put(RAFT_MONITOR_NAME, RAFT_CLIENT_PUT_ARG_WOOF, "h_client_put", &arg, 0);
    if (WooFInvalid(seq)) {
        log_error("failed to queue the next h_client_put handler");
        exit(1);
    }
    return 1;
}
