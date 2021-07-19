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

typedef struct forward_result_thread_arg {
    RAFT_CLIENT_PUT_REQUEST request;
    RAFT_CLIENT_PUT_RESULT result;
} FORWARD_RESULT_THREAD_ARG;

void* forward_result_thread(void* arg) {
    FORWARD_RESULT_THREAD_ARG* forward_result_thread_arg = (FORWARD_RESULT_THREAD_ARG*)arg;
    unsigned long result_seq;
    if (forward_result_thread_arg->request.callback_handler[0] != 0) {
        result_seq = WooFPut(forward_result_thread_arg->request.callback_woof,
                             forward_result_thread_arg->request.callback_handler,
                             &forward_result_thread_arg->result);
    } else {
        result_seq =
            WooFPut(forward_result_thread_arg->request.callback_woof, NULL, &forward_result_thread_arg->result);
    }
    if (WooFInvalid(result_seq)) {
        log_error("failed to put client_put_result to %s", forward_result_thread_arg->request.callback_woof);
        return;
    }
}

int h_forward_put_result(WOOF* wf, unsigned long seq_no, void* ptr) {
    RAFT_FORWARD_PUT_RESULT_ARG* arg = (RAFT_FORWARD_PUT_RESULT_ARG*)ptr;
    log_set_tag("h_forward_put_result");
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

    if (server_state.current_term != arg->term || server_state.role != RAFT_LEADER) {
        log_debug("not a leader at term %" PRIu64 " anymore, current term: %" PRIu64 "",
                  arg->term,
                  server_state.current_term);
        
        raft_unlock(RAFT_LOCK_SERVER);
        return 1;
    }
    raft_unlock(RAFT_LOCK_SERVER);

    // check previous append_entries_result
    unsigned long latest_result_seqno = WooFGetLatestSeqno(RAFT_CLIENT_PUT_RESULT_WOOF);
    if (WooFInvalid(latest_result_seqno)) {
        log_error("failed to get the latest seqno of %s", RAFT_CLIENT_PUT_RESULT_WOOF);
        
        exit(1);
    }

    // forward client_put_result
    int pending = latest_result_seqno - arg->last_forwarded_put_result;
    FORWARD_RESULT_THREAD_ARG* forward_result_thread_arg = malloc(sizeof(FORWARD_RESULT_THREAD_ARG) * pending);
    pthread_t* forward_result_thread_id = malloc(sizeof(pthread_t) * pending);
    int result_count = 0;
    unsigned int i;
    for (i = arg->last_forwarded_put_result + 1; i <= latest_result_seqno; ++i) {
        RAFT_CLIENT_PUT_REQUEST* request = &(forward_result_thread_arg[result_count].request);
        if (WooFGet(RAFT_CLIENT_PUT_REQUEST_WOOF, request, i) < 0) {
            log_error("failed to get the client_put request at %lu", i);
            threads_join(result_count, forward_result_thread_id);
            free(forward_result_thread_arg);
            free(forward_result_thread_id);
            
            exit(1);
        }
        // log_debug("checking result %lu", i);
        RAFT_CLIENT_PUT_RESULT* result = &(forward_result_thread_arg[result_count].result);
        if (WooFGet(RAFT_CLIENT_PUT_RESULT_WOOF, result, i) < 0) {
            log_error("failed to get the client_put result at %lu", i);
            threads_join(result_count, forward_result_thread_id);
            free(forward_result_thread_arg);
            free(forward_result_thread_id);
            
            exit(1);
        }
        result->ts_forward = get_milliseconds();

        if (result->index == 0 || request->callback_woof[0] == 0) {
            arg->last_forwarded_put_result = i;
            continue;
        } else if (result->index <= server_state.commit_index) {
            if (result->term != server_state.current_term) {
                log_warn("result term %lu doesn't match server's term, entry got rewritten and wasn't committed",
                         result->term);
            } else {
                // log entry has been committed, invoke the handler
                if (pthread_create(&forward_result_thread_id[result_count],
                                   NULL,
                                   forward_result_thread,
                                   (void*)&forward_result_thread_arg[result_count]) < 0) {
                    log_error("failed to create thread to invoke callback of client_put request");
                    threads_join(result_count, forward_result_thread_id);
                    free(forward_result_thread_arg);
                    free(forward_result_thread_id);
                    return -1;
                }
                ++result_count;
            }
            arg->last_forwarded_put_result = i;
        } else {
            break;
        }
    }

    uint64_t join_begin = get_milliseconds();
    threads_join(result_count, forward_result_thread_id);
    if (get_milliseconds() - join_begin > 5000) {
        log_warn("join took %lu ms", get_milliseconds() - join_begin);
    }
    free(forward_result_thread_arg);
    free(forward_result_thread_id);
    if (result_count != 0) {
        log_debug("sent %d client_put: %lu ms", result_count, get_milliseconds() - begin);
    }

    unsigned long seq = WooFPut(RAFT_FORWARD_PUT_RESULT_WOOF, "h_forward_put_result", arg);
    if (WooFInvalid(seq)) {
        log_error("failed to invoke the next h_forward_put_result handler");
        
        exit(1);
    }

    
    return 1;
}
