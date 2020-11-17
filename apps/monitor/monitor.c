#include "monitor.h"

#include "woofc-access.h"
#include "woofc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int monitor_create(char* monitor_name) {
    char pool_woof[MONITOR_WOOF_NAME_LENGTH];
    sprintf(pool_woof, "%s_%s", monitor_name, MONITOR_POOL_WOOF);
    if (WooFCreate(pool_woof, sizeof(MONITOR_POOL_ITEM), MONITOR_HISTORY_LENGTH) < 0) {
        fprintf(stderr, "failed to create %s\n", pool_woof);
        return -1;
    }
    char done_woof[MONITOR_WOOF_NAME_LENGTH];
    sprintf(done_woof, "%s_%s", monitor_name, MONITOR_DONE_WOOF);
    if (WooFCreate(done_woof, sizeof(MONITOR_DONE_ITEM), MONITOR_HISTORY_LENGTH) < 0) {
        fprintf(stderr, "failed to create %s\n", done_woof);
        return -1;
    }
    char handler_woof[MONITOR_WOOF_NAME_LENGTH];
    sprintf(handler_woof, "%s_%s", monitor_name, MONITOR_HANDLER_WOOF);
    if (WooFCreate(handler_woof, sizeof(MONITOR_POOL_ITEM), MONITOR_HISTORY_LENGTH) < 0) {
        fprintf(stderr, "failed to create %s\n", handler_woof);
        return -1;
    }
    char invoker_woof[MONITOR_WOOF_NAME_LENGTH];
    sprintf(invoker_woof, "%s_%s", monitor_name, MONITOR_INVOKER_WOOF);
    if (WooFCreate(invoker_woof, sizeof(MONITOR_INVOKER_ARG), MONITOR_HISTORY_LENGTH) < 0) {
        fprintf(stderr, "failed to create %s\n", invoker_woof);
        return -1;
    }

    MONITOR_INVOKER_ARG invoker_arg = {0};
    sprintf(invoker_arg.pool_woof, "%s_%s", monitor_name, MONITOR_POOL_WOOF);
    sprintf(invoker_arg.done_woof, "%s_%s", monitor_name, MONITOR_DONE_WOOF);
    sprintf(invoker_arg.handler_woof, "%s_%s", monitor_name, MONITOR_HANDLER_WOOF);
    invoker_arg.spinlock_delay = MONITOR_SPINLOCK_DELAY;
    unsigned long seq = WooFPut(invoker_woof, "monitor_invoker", &invoker_arg);
    if (WooFInvalid(seq)) {
        return -1;
    }
    return 0;
}

unsigned long monitor_put(char* monitor_name, char* woof_name, char* handler, void* ptr, int idempotent) {
    WOOF* wf = WooFOpen(woof_name);
    if (wf == NULL) {
        return -1;
    }
    unsigned long woof_seq = WooFAppend(wf, NULL, ptr);
    if (WooFInvalid(woof_seq)) {
        WooFDrop(wf);
        return -1;
    }
    MONITOR_POOL_ITEM pool_item = {0};
    strcpy(pool_item.woof_name, woof_name);
    strcpy(pool_item.handler, handler);
    pool_item.seq_no = (uint64_t)woof_seq;
    pool_item.element_size = (uint64_t)wf->shared->element_size;
    strcpy(pool_item.monitor_name, monitor_name);
    pool_item.idempotent = (int32_t)idempotent;
    WooFDrop(wf);
#ifdef PROCESS_TIME
    struct timeval tv;
    gettimeofday(&tv, NULL);
    pool_item.queued_ts = (uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000;
#endif

    char pool_woof[MONITOR_WOOF_NAME_LENGTH];
    sprintf(pool_woof, "%s_%s", monitor_name, MONITOR_POOL_WOOF);
    unsigned long monitor_seq = WooFPut(pool_woof, NULL, &pool_item);
    if (WooFInvalid(monitor_seq)) {
        return -1;
    }
    return woof_seq;
}

unsigned long monitor_queue(char* monitor_name, char* woof_name, char* handler, unsigned long seq_no) {
    WOOF* wf = WooFOpen(woof_name);
    if (wf == NULL) {
        return -1;
    }

    MONITOR_POOL_ITEM pool_item;
    strcpy(pool_item.woof_name, woof_name);
    strcpy(pool_item.handler, handler);
    pool_item.seq_no = (uint64_t)seq_no;
    pool_item.element_size = (uint64_t)wf->shared->element_size;
    strcpy(pool_item.monitor_name, monitor_name);
    WooFDrop(wf);
#ifdef PROCESS_TIME
    struct timeval tv;
    gettimeofday(&tv, NULL);
    pool_item.queued_ts = (uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000;
#endif

    char pool_woof[MONITOR_WOOF_NAME_LENGTH];
    sprintf(pool_woof, "%s_%s", monitor_name, MONITOR_POOL_WOOF);
    unsigned long monitor_seq = WooFPut(pool_woof, NULL, &pool_item);
    return monitor_seq;
}

unsigned long monitor_remote_put(char* monitor_uri, char* woof_uri, char* handler, void* ptr, int idempotent) {
    char monitor_name[MONITOR_WOOF_NAME_LENGTH];
    char woof_name[MONITOR_WOOF_NAME_LENGTH];
    if (WooFNameFromURI(monitor_uri, monitor_name, MONITOR_WOOF_NAME_LENGTH) < 0) {
        fprintf(stderr, "failed to extract monitor name from URI: %s\n", monitor_uri);
        return -1;
    }
    if (WooFNameFromURI(woof_uri, woof_name, MONITOR_WOOF_NAME_LENGTH) < 0) {
        fprintf(stderr, "failed to extract woof name from URI: %s\n", woof_uri);
        return -1;
    }

    unsigned long element_size = WooFMsgGetElSize(woof_uri);
    if (WooFInvalid(element_size)) {
        fprintf(stderr, "failed to get element size from %s\n", woof_uri);
        return -1;
    }
    unsigned long woof_seq = WooFPut(woof_uri, NULL, ptr);
    if (WooFInvalid(woof_seq)) {
        fprintf(stderr, "failed to WooFPut to %s\n", woof_uri);
        return -1;
    }
    MONITOR_POOL_ITEM pool_item = {0};
    strcpy(pool_item.woof_name, woof_name);
    strcpy(pool_item.handler, handler);
    pool_item.seq_no = (uint64_t)woof_seq;
    pool_item.element_size = (uint64_t)element_size;
    strcpy(pool_item.monitor_name, monitor_name);
    pool_item.idempotent = idempotent;
#ifdef PROCESS_TIME
    struct timeval tv;
    gettimeofday(&tv, NULL);
    pool_item.queued_ts = (uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000;
#endif

    char pool_woof[MONITOR_WOOF_NAME_LENGTH];
    sprintf(pool_woof, "%s_%s", monitor_uri, MONITOR_POOL_WOOF);
    unsigned long monitor_seq = WooFPut(pool_woof, NULL, &pool_item);
    if (WooFInvalid(monitor_seq)) {
        fprintf(stderr, "failed to WooFPut to %s\n", pool_woof);
        return -1;
    }
    return woof_seq;
}

unsigned long
monitor_remote_queue(char* monitor_uri, char* woof_uri, char* handler, unsigned long seq_no, int idempotent) {
    char monitor_name[MONITOR_WOOF_NAME_LENGTH];
    char woof_name[MONITOR_WOOF_NAME_LENGTH];
    if (WooFNameFromURI(monitor_uri, monitor_name, MONITOR_WOOF_NAME_LENGTH) < 0) {
        fprintf(stderr, "failed to extract monitor name from URI: %s\n", monitor_uri);
        return -1;
    }
    if (WooFNameFromURI(woof_uri, woof_name, MONITOR_WOOF_NAME_LENGTH) < 0) {
        fprintf(stderr, "failed to extract woof name from URI: %s\n", woof_uri);
        return -1;
    }

    unsigned long element_size = WooFMsgGetElSize(woof_uri);
    if (WooFInvalid(element_size)) {
        return -1;
    }

    MONITOR_POOL_ITEM pool_item = {0};
    strcpy(pool_item.woof_name, woof_name);
    strcpy(pool_item.handler, handler);
    pool_item.seq_no = (uint64_t)seq_no;
    pool_item.element_size = (uint64_t)element_size;
    strcpy(pool_item.monitor_name, monitor_name);
    pool_item.idempotent = idempotent;
#ifdef PROCESS_TIME
    struct timeval tv;
    gettimeofday(&tv, NULL);
    pool_item.queued_ts = (uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000;
#endif

    char pool_woof[MONITOR_WOOF_NAME_LENGTH];
    sprintf(pool_woof, "%s_%s", monitor_uri, MONITOR_POOL_WOOF);
    unsigned long monitor_seq = WooFPut(pool_woof, NULL, &pool_item);
    return monitor_seq;
}

int monitor_cast(void* ptr, void* element, unsigned long size) {
    MONITOR_POOL_ITEM* pool_item = (MONITOR_POOL_ITEM*)ptr;
    WOOF* wf = WooFOpen(pool_item->woof_name);
    unsigned long pool_item_size = wf->shared->element_size;
    WooFDrop(wf);
    if (size < pool_item_size) {
        void* buf = malloc(pool_item_size);
        if (buf == NULL) {
            return -1;
        }
        if (WooFGet(pool_item->woof_name, buf, (unsigned long)pool_item->seq_no) < 0) {
            free(buf);
            return -1;
        }
        memcpy(element, buf, size);
        free(buf);
        return 0;
    }
    if (WooFGet(pool_item->woof_name, element, (unsigned long)pool_item->seq_no) < 0) {
        return -1;
    }
    return 0;
}

unsigned long monitor_seqno(void* ptr) {
    MONITOR_POOL_ITEM* pool_item = (MONITOR_POOL_ITEM*)ptr;
    return (unsigned long)pool_item->seq_no;
}

int monitor_exit(void* ptr) {
    MONITOR_POOL_ITEM* pool_item = (MONITOR_POOL_ITEM*)ptr;

    MONITOR_DONE_ITEM done_item;
    char done_woof[MONITOR_WOOF_NAME_LENGTH];
    sprintf(done_woof, "%s_%s", pool_item->monitor_name, MONITOR_DONE_WOOF);
    unsigned long seq = WooFPut(done_woof, NULL, &done_item);
    if (WooFInvalid(seq)) {
        return -1;
    }

    MONITOR_INVOKER_ARG invoker_arg = {0};
    sprintf(invoker_arg.pool_woof, "%s_%s", pool_item->monitor_name, MONITOR_POOL_WOOF);
    sprintf(invoker_arg.done_woof, "%s_%s", pool_item->monitor_name, MONITOR_DONE_WOOF);
    sprintf(invoker_arg.handler_woof, "%s_%s", pool_item->monitor_name, MONITOR_HANDLER_WOOF);
    invoker_arg.spinlock_delay = MONITOR_SPINLOCK_DELAY;
    char invoker_woof[MONITOR_WOOF_NAME_LENGTH];
    sprintf(invoker_woof, "%s_%s", pool_item->monitor_name, MONITOR_INVOKER_WOOF);
    // if (strcmp(pool_item->monitor_name, "raft") == 0) {
    //     printf("handler %s done\n", pool_item->handler);
    // }
    seq = WooFPut(invoker_woof, "monitor_invoker", &invoker_arg);
    if (WooFInvalid(seq)) {
        fprintf(stderr, "failed to spinlock on %s\n", invoker_woof);
        exit(1);
    }
}
