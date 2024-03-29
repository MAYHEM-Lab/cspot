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
    RAFT_CLIENT_PUT_ARG* arg = (RAFT_CLIENT_PUT_ARG*)ptr;
    log_set_tag("h_client_put");
    log_set_level(RAFT_LOG_INFO);
    // log_set_level(RAFT_LOG_DEBUG);
    log_set_output(stdout);
    uint64_t begin = get_milliseconds();

    // get the server's current term
    RAFT_SERVER_STATE server_state = {0};
    if (WooFGet(RAFT_SERVER_STATE_WOOF, &server_state, 0) < 0) {
        log_error("failed to get the server state");
        
        exit(1);
    }

    unsigned long latest_request = WooFGetLatestSeqno(RAFT_CLIENT_PUT_REQUEST_WOOF);
    if (WooFInvalid(latest_request)) {
        log_error("failed to get the latest seqno from %s", RAFT_CLIENT_PUT_REQUEST_WOOF);
        exit(1);
    }
    int count = latest_request - arg->last_seqno;
    if (count > 0) {
        log_debug("processing %lu requests, %lu to %lu", count, arg->last_seqno + 1, latest_request);
    }

    raft_lock(RAFT_LOCK_LOG);
    int redirected = 0;
    unsigned long i;
    for (i = arg->last_seqno + 1; i <= latest_request; ++i) {
        RAFT_CLIENT_PUT_REQUEST request = {0};
        if (WooFGet(RAFT_CLIENT_PUT_REQUEST_WOOF, &request, i) < 0) {
            log_error("failed to get client_put request at %lu", i);
            
            raft_unlock(RAFT_LOCK_LOG);
            exit(1);
        }
        RAFT_CLIENT_PUT_RESULT result = {0};
        memcpy(result.source, server_state.woof_name, sizeof(result.source));
        memcpy(result.extra_woof, request.extra_woof, sizeof(result.extra_woof));
        result.extra_seqno = request.extra_seqno;
        if (server_state.role != RAFT_LEADER) {
            if (strcmp(server_state.current_leader, server_state.woof_name) == 0) {
                // there is a leader election right now, hold the request
                break;
            }
            char redirected_woof[RAFT_NAME_LENGTH] = {0};
            sprintf(redirected_woof, "%s/%s", server_state.current_leader, RAFT_CLIENT_PUT_REQUEST_WOOF);
            strcpy(result.redirected_target, server_state.current_leader);
            result.redirected_seqno = WooFPut(redirected_woof, NULL, &request);
            if (WooFInvalid(result.redirected_seqno)) {
                log_error("failed to redirect client_put request to %s", redirected_woof);
            }
            log_debug("redirected request to leader %s[%lu]", result.redirected_target, result.redirected_seqno);
            ++redirected;

            // // let the caller know to invalidate cache
            // char caller_woof[RAFT_NAME_LENGTH] = {0};
            // strcpy(caller_woof, request.callback_woof);
            // int j;
            // for (j = strlen(caller_woof) - 1; j >= 0; --j) {
            //     if (caller_woof[j] == '/') {
            //         break;
            //     }
            // }
            // if (j > 0) {
            //     sprintf(caller_woof + j + 1, "dht_invalidate_cache.woof");
            //     struct invalidate_cache_arg {
            //         char topic_name[RAFT_NAME_LENGTH];
            //     } arg = {0};
            //     strcpy(arg.topic_name, request.topic_name);
            //     if (WooFPut(caller_woof, "h_invalidate_cache", &arg) < 0) {
            //         log_error("failed to ask %s to invalidate cache");
            //     }
            //     log_debug("invalidate %s on %s", arg.topic_name, caller_woof);
            // }
        } else {
            unsigned long entry_seqno;
            RAFT_LOG_ENTRY entry = {0};
            entry.term = server_state.current_term;
            strcpy(entry.topic_name, request.topic_name);
            memcpy(&entry.data, &request.data, sizeof(RAFT_DATA_TYPE));
            entry.is_config = RAFT_CONFIG_ENTRY_NOT;
            entry.is_handler = request.is_handler;
            entry.ts_created = request.ts_created;
            entry.ts_written = get_milliseconds();
            entry_seqno = WooFPut(RAFT_LOG_ENTRIES_WOOF, NULL, &entry);
            if (WooFInvalid(entry_seqno)) {
                log_error("failed to append raft log");
                
                raft_unlock(RAFT_LOCK_LOG);
                exit(1);
            }
            log_debug("appended entry[%lu] into log", entry_seqno);
            result.index = (uint64_t)entry_seqno;
            result.term = server_state.current_term;
        }
        // make sure the result's seq_no matches with request's seq_no
        unsigned long latest_result_seqno = WooFGetLatestSeqno(RAFT_CLIENT_PUT_RESULT_WOOF);
        while (latest_result_seqno + 1 != i) {
            log_warn("client_put_result seq_no %lu mismatches %lu", latest_result_seqno + 1, i);
            RAFT_CLIENT_PUT_RESULT padding_result = {0};
            if (WooFInvalid(WooFPut(RAFT_CLIENT_PUT_RESULT_WOOF, NULL, &padding_result))) {
                log_error("failed to pad client_put result for previous unresolved requests");
                
                raft_unlock(RAFT_LOCK_LOG);
                exit(1);
            }
            ++latest_result_seqno;
        }

        unsigned long result_seq = WooFPut(RAFT_CLIENT_PUT_RESULT_WOOF, NULL, &result);
        if (WooFInvalid(result_seq)) {
            log_error("failed to write client_put_result");
            
            raft_unlock(RAFT_LOCK_LOG);
            exit(1);
        }
        // log_error("result.index: %lu [%lu]", result.index, result_seq);
        arg->last_seqno = i;
    }
    raft_unlock(RAFT_LOCK_LOG);

    if (redirected > 0) {
        log_warn("redirected %d requests to the leader", redirected);
    }

    unsigned long seq = WooFPut(RAFT_CLIENT_PUT_ARG_WOOF, "h_client_put", arg);
    if (WooFInvalid(seq)) {
        log_error("failed to queue the next h_client_put handler");
        
        exit(1);
    }
    // printf("handler h_client_put took %lu\n", get_milliseconds() - begin);
    
    return 1;
}
