#include "czmq.h"
#include "monitor.h"
#include "raft.h"
#include "raft_utils.h"
#include "woofc-access.h"
#include "woofc.h"

#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

typedef struct invoke_thread_arg {
    RAFT_CLIENT_PUT_REQUEST request;
    RAFT_CLIENT_PUT_RESULT result;
} INVOKE_THREAD_ARG;

void* invoke_thread(void* arg) {
    INVOKE_THREAD_ARG* invoke_thread_arg = (INVOKE_THREAD_ARG*)arg;
    unsigned long result_seq;
    if (invoke_thread_arg->request.callback_handler[0] != 0) {
        result_seq = WooFPut(invoke_thread_arg->request.callback_woof,
                             invoke_thread_arg->request.callback_handler,
                             &invoke_thread_arg->result);
    } else {
        result_seq = WooFPut(invoke_thread_arg->request.callback_woof, NULL, &invoke_thread_arg->result);
    }
    if (WooFInvalid(result_seq)) {
        log_error("failed to put client_put_result to %s", invoke_thread_arg->request.callback_woof);
        return;
    }
}

int h_invoke_committed(WOOF* wf, unsigned long seq_no, void* ptr) {
    RAFT_INVOKE_COMMITTED_ARG* arg = (RAFT_INVOKE_COMMITTED_ARG*)ptr;
    log_set_tag("h_invoke_committed");
    log_set_level(RAFT_LOG_INFO);
    // log_set_level(RAFT_LOG_DEBUG);
    log_set_output(stdout);
    WooFMsgCacheInit();
    zsys_init();

    uint64_t begin = get_milliseconds();

    // get the server's current term and cluster members
    RAFT_SERVER_STATE server_state = {0};
    if (WooFGet(RAFT_SERVER_STATE_WOOF, &server_state, 0) < 0) {
        log_error("failed to get the server state");
        WooFMsgCacheShutdown();
        exit(1);
    }

    if (server_state.current_term != arg->term || server_state.role != RAFT_LEADER) {
        log_debug("not a leader at term %" PRIu64 " anymore, current term: %" PRIu64 "",
                  arg->term,
                  server_state.current_term);
        WooFMsgCacheShutdown();
        return 1;
    }

    // check previous append_entries_result
    unsigned long latest_result_seqno = WooFGetLatestSeqno(RAFT_CLIENT_PUT_RESULT_WOOF);
    if (WooFInvalid(latest_result_seqno)) {
        log_error("failed to get the latest seqno of %s", RAFT_CLIENT_PUT_RESULT_WOOF);
        WooFMsgCacheShutdown();
        exit(1);
    }

    int pending = latest_result_seqno - arg->last_checked_client_put_result_seqno;
    INVOKE_THREAD_ARG* invoke_thread_arg = malloc(sizeof(INVOKE_THREAD_ARG) * pending);
    pthread_t* invoke_thread_id = malloc(sizeof(pthread_t) * pending);
    int count = 0;
    unsigned int i;
    for (i = arg->last_checked_client_put_result_seqno + 1; i <= latest_result_seqno; ++i) {
        RAFT_CLIENT_PUT_REQUEST* request = &(invoke_thread_arg[count].request);
        if (WooFGet(RAFT_CLIENT_PUT_REQUEST_WOOF, request, i) < 0) {
            log_error("failed to get the client_put request at %lu", i);
            threads_join(count, invoke_thread_id);
            free(invoke_thread_arg);
            free(invoke_thread_id);
            WooFMsgCacheShutdown();
            exit(1);
        }
        // log_debug("checking result %lu", i);
        RAFT_CLIENT_PUT_RESULT* result = &(invoke_thread_arg[count].result);
        if (WooFGet(RAFT_CLIENT_PUT_RESULT_WOOF, result, i) < 0) {
            log_error("failed to get the client_put result at %lu", i);
            threads_join(count, invoke_thread_id);
            free(invoke_thread_arg);
            free(invoke_thread_id);
            WooFMsgCacheShutdown();
            exit(1);
        }
        if (result->index == 0 || request->callback_woof[0] == 0) {
            arg->last_checked_client_put_result_seqno = i;
            continue;
        } else if (result->index <= server_state.commit_index) {
            // log entry has been committed, invoke the handler
            if (pthread_create(&invoke_thread_id[count], NULL, invoke_thread, (void*)&invoke_thread_arg[count]) < 0) {
                log_error("failed to create thread to invoke callback of client_put request");
                threads_join(count, invoke_thread_id);
                free(invoke_thread_arg);
                free(invoke_thread_id);
                return -1;
            }
            ++count;
            arg->last_checked_client_put_result_seqno = i;
        } else {
            break;
        }
    }

    threads_join(count, invoke_thread_id);
    free(invoke_thread_arg);
    free(invoke_thread_id);
    if (count != 0) {
        log_debug("sent %d client_put: %lu ms", count, get_milliseconds() - begin);
    }

    unsigned long seq = WooFPut(RAFT_INVOKE_COMMITTED_WOOF, "h_invoke_committed", arg);
    if (WooFInvalid(seq)) {
        log_error("failed to invoke the next h_invoke_committed handler");
        WooFMsgCacheShutdown();
        exit(1);
    }

    WooFMsgCacheShutdown();
    return 1;
}
