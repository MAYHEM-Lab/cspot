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

void* invoke_thread(void* arg) {
    RAFT_LOG_ENTRY* entry = (RAFT_LOG_ENTRY*)arg;
    RAFT_LOG_HANDLER_ENTRY* handler_entry = (RAFT_LOG_HANDLER_ENTRY*)&entry->data;
    unsigned long invoked_seq = -1;
    if (handler_entry->monitored) {
        invoked_seq = monitor_put(
            RAFT_MONITOR_NAME, RAFT_LOG_HANDLER_ENTRIES_WOOF, handler_entry->handler, handler_entry->ptr, 0);
    } else {
        invoked_seq = WooFPut(RAFT_LOG_HANDLER_ENTRIES_WOOF, handler_entry->handler, handler_entry->ptr);
    }
    if (WooFInvalid(invoked_seq)) {
        log_error("failed to invoke %s for appended handler entry", handler_entry->handler);
        return;
    }
    log_debug("appended a handler entry and invoked the handler %s", handler_entry->handler);
}

int h_commit_handler(WOOF* wf, unsigned long seq_no, void* ptr) {
    RAFT_COMMIT_HANDLER_ARG* arg = (RAFT_COMMIT_HANDLER_ARG*)ptr;
    log_set_tag("h_commit_handler");
    log_set_level(RAFT_LOG_INFO);
    // log_set_level(RAFT_LOG_DEBUG);
    log_set_output(stdout);
    zsys_init();
    monitor_init();
    WooFMsgCacheInit();

    uint64_t begin = get_milliseconds();

    // get the server's current term and cluster members
    RAFT_SERVER_STATE server_state = {0};
    if (get_server_state(&server_state) < 0) {
        log_error("failed to get the server state");
        WooFMsgCacheShutdown();
        exit(1);
    }

    int pending = server_state.commit_index - arg->last_index;
    RAFT_LOG_ENTRY* invoke_thread_arg = malloc(sizeof(RAFT_LOG_ENTRY) * pending);
    pthread_t* invoke_thread_id = malloc(sizeof(pthread_t) * pending);
    int count = 0;
    unsigned int i;
    for (i = arg->last_index + 1; i <= server_state.commit_index; ++i) {
        if (WooFGet(RAFT_LOG_ENTRIES_WOOF, &invoke_thread_arg[count], i) < 0) {
            log_error("failed to get log entry at %lu", i);
        }
        if (invoke_thread_arg[count].is_handler) {
            if (pthread_create(&invoke_thread_id[count], NULL, invoke_thread, &invoke_thread_arg[count]) < 0) {
                log_error("failed to create thread to invoke committed handler entry");
            }
            ++count;
        }
        arg->last_index = i;
    }
    if (count != 0) {
        log_debug("waiting for %d handler entries to be invoked", count);
    }
    threads_join(count, invoke_thread_id);
    free(invoke_thread_arg);
    free(invoke_thread_id);
    if (count != 0) {
        log_debug("invoked %d handler entries: %lu ms", count, get_milliseconds() - begin);
    }

    unsigned long seq = WooFPut(RAFT_COMMIT_HANDLER_WOOF, "h_commit_handler", arg);
    if (WooFInvalid(seq)) {
        log_error("failed to invoke the next h_commit_handler handler");
        WooFMsgCacheShutdown();
        exit(1);
    }

    monitor_join();
    WooFMsgCacheShutdown();
    return 1;
}
