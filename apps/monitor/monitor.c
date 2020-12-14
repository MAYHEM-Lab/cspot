#include "monitor.h"

#include "woofc-access.h"
#include "woofc.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MONITOR_THREAD_POOL_SIZE 8

typedef struct put_pool_item_thread_arg {
    char woof_name[256];
    MONITOR_POOL_ITEM pool_item;
    int thread_id;
} PUT_POOL_ITEM_THREAD_ARG;

pthread_mutex_t lock;
PUT_POOL_ITEM_THREAD_ARG thread_arg[MONITOR_THREAD_POOL_SIZE];
pthread_t thread_id[MONITOR_THREAD_POOL_SIZE];
int next_available_thread;

time_t gm() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (time_t)tv.tv_sec * 1000 + (time_t)tv.tv_usec / 1000;
}

void* put_pool_item_thread(void* ptr) {
    PUT_POOL_ITEM_THREAD_ARG* arg = (PUT_POOL_ITEM_THREAD_ARG*)ptr;
    unsigned long seq = WooFPut(arg->woof_name, NULL, &arg->pool_item);
    if (WooFInvalid(seq)) {
        fprintf(stderr, "failed to WooFPut to %s", arg->woof_name);
    }
    pthread_mutex_lock(&lock);
    thread_id[arg->thread_id] = 0;
    next_available_thread = arg->thread_id;
    pthread_mutex_unlock(&lock);
}

int get_next_available_thread() {
    int id = next_available_thread % MONITOR_THREAD_POOL_SIZE;
    pthread_mutex_lock(&lock);
    while (thread_id[id] != 0) {
        pthread_mutex_unlock(&lock);
        id = (id + 1) % MONITOR_THREAD_POOL_SIZE;
        pthread_mutex_lock(&lock);
    }
    pthread_mutex_unlock(&lock);
    return id;
}

int monitor_create(char* monitor_name) {
    zsys_init();
    if (pthread_mutex_init(&lock, NULL) != 0) {
        sprintf(monitor_error_msg, "failed to initialize mutex");
        return -1;
    }
    memset(thread_arg, 0, sizeof(PUT_POOL_ITEM_THREAD_ARG) * MONITOR_THREAD_POOL_SIZE);
    memset(thread_id, 0, sizeof(pthread_t) * MONITOR_THREAD_POOL_SIZE);
    next_available_thread = 0;

    char pool_woof[MONITOR_WOOF_NAME_LENGTH];
    sprintf(pool_woof, "%s_%s", monitor_name, MONITOR_POOL_WOOF);
    if (WooFCreate(pool_woof, sizeof(MONITOR_POOL_ITEM), MONITOR_HISTORY_LENGTH) < 0) {
        sprintf(monitor_error_msg, "failed to create %s", pool_woof);
        return -1;
    }
    char done_woof[MONITOR_WOOF_NAME_LENGTH];
    sprintf(done_woof, "%s_%s", monitor_name, MONITOR_DONE_WOOF);
    if (WooFCreate(done_woof, sizeof(MONITOR_DONE_ITEM), MONITOR_HISTORY_LENGTH) < 0) {
        sprintf(monitor_error_msg, "failed to create %s", done_woof);
        return -1;
    }
    char handler_woof[MONITOR_WOOF_NAME_LENGTH];
    sprintf(handler_woof, "%s_%s", monitor_name, MONITOR_HANDLER_WOOF);
    if (WooFCreate(handler_woof, sizeof(MONITOR_POOL_ITEM), MONITOR_HISTORY_LENGTH) < 0) {
        sprintf(monitor_error_msg, "failed to create %s", handler_woof);
        return -1;
    }
    char invoker_woof[MONITOR_WOOF_NAME_LENGTH];
    sprintf(invoker_woof, "%s_%s", monitor_name, MONITOR_INVOKER_WOOF);
    if (WooFCreate(invoker_woof, sizeof(MONITOR_INVOKER_ARG), MONITOR_HISTORY_LENGTH) < 0) {
        sprintf(monitor_error_msg, "failed to create %s", invoker_woof);
        return -1;
    }

    MONITOR_INVOKER_ARG invoker_arg = {0};
    sprintf(invoker_arg.pool_woof, "%s_%s", monitor_name, MONITOR_POOL_WOOF);
    sprintf(invoker_arg.done_woof, "%s_%s", monitor_name, MONITOR_DONE_WOOF);
    sprintf(invoker_arg.handler_woof, "%s_%s", monitor_name, MONITOR_HANDLER_WOOF);
    invoker_arg.spinlock_delay = MONITOR_SPINLOCK_DELAY;
    unsigned long seq = WooFPut(invoker_woof, "monitor_invoker", &invoker_arg);
    if (WooFInvalid(seq)) {
        sprintf(monitor_error_msg, "failed to invoke next monitor_invoker");
        return -1;
    }
    return 0;
}

unsigned long monitor_put(char* monitor_name, char* woof_name, char* handler, void* ptr, int idempotent) {
    WOOF* wf = WooFOpen(woof_name);
    if (wf == NULL) {
        sprintf(monitor_error_msg, "failed to open %s", woof_name);
        return -1;
    }
    unsigned long woof_seq = WooFAppend(wf, NULL, ptr);
    if (WooFInvalid(woof_seq)) {
        sprintf(monitor_error_msg, "failed to append to %s", woof_name);
        WooFDrop(wf);
        return -1;
    }

    int tid = get_next_available_thread();

    thread_arg[tid].thread_id = tid;
    strcpy(thread_arg[tid].pool_item.woof_name, woof_name);
    strcpy(thread_arg[tid].pool_item.handler, handler);
    thread_arg[tid].pool_item.seq_no = (uint64_t)woof_seq;
    strcpy(thread_arg[tid].pool_item.monitor_name, monitor_name);
    thread_arg[tid].pool_item.idempotent = (int32_t)idempotent;
    sprintf(thread_arg[tid].woof_name, "%s_%s", monitor_name, MONITOR_POOL_WOOF);
    WooFDrop(wf);

    if (pthread_create(&thread_id[tid], NULL, put_pool_item_thread, (void*)&thread_arg[tid]) < 0) {
        sprintf(monitor_error_msg, "failed to put to pool woof %s", thread_arg[tid].woof_name);
        return -1;
    }
    return woof_seq;
}

unsigned long monitor_queue(char* monitor_name, char* woof_name, char* handler, unsigned long seq_no) {
    WOOF* wf = WooFOpen(woof_name);
    if (wf == NULL) {
        sprintf(monitor_error_msg, "failed to open %s", woof_name);
        return -1;
    }

    MONITOR_POOL_ITEM pool_item;
    strcpy(pool_item.woof_name, woof_name);
    strcpy(pool_item.handler, handler);
    pool_item.seq_no = (uint64_t)seq_no;
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
        sprintf(monitor_error_msg, "failed to extract monitor name from URI: %s", monitor_uri);
        return -1;
    }
    if (WooFNameFromURI(woof_uri, woof_name, MONITOR_WOOF_NAME_LENGTH) < 0) {
        sprintf(monitor_error_msg, "failed to extract woof name from URI: %s", woof_uri);
        return -1;
    }

    unsigned long woof_seq = WooFPut(woof_uri, NULL, ptr);
    if (WooFInvalid(woof_seq)) {
        sprintf(monitor_error_msg, "failed to WooFPut to %s", woof_uri);
        return -1;
    }

    int tid = get_next_available_thread();
    thread_arg[tid].thread_id = tid;
    strcpy(thread_arg[tid].pool_item.woof_name, woof_name);
    strcpy(thread_arg[tid].pool_item.handler, handler);
    thread_arg[tid].pool_item.seq_no = (uint64_t)woof_seq;
    strcpy(thread_arg[tid].pool_item.monitor_name, monitor_name);
    thread_arg[tid].pool_item.idempotent = (int32_t)idempotent;
    sprintf(thread_arg[tid].woof_name, "%s_%s", monitor_uri, MONITOR_POOL_WOOF);

    if (pthread_create(&thread_id[tid], NULL, put_pool_item_thread, (void*)&thread_arg[tid]) < 0) {
        sprintf(monitor_error_msg, "failed to put to pool woof %s", thread_arg[tid].woof_name);
        return -1;
    }
    return woof_seq;
}

unsigned long
monitor_remote_queue(char* monitor_uri, char* woof_uri, char* handler, unsigned long seq_no, int idempotent) {
    char monitor_name[MONITOR_WOOF_NAME_LENGTH];
    char woof_name[MONITOR_WOOF_NAME_LENGTH];
    if (WooFNameFromURI(monitor_uri, monitor_name, MONITOR_WOOF_NAME_LENGTH) < 0) {
        sprintf(monitor_error_msg, "failed to extract monitor name from URI: %s", monitor_uri);
        return -1;
    }
    if (WooFNameFromURI(woof_uri, woof_name, MONITOR_WOOF_NAME_LENGTH) < 0) {
        sprintf(monitor_error_msg, "failed to extract woof name from URI: %s", woof_uri);
        return -1;
    }

    MONITOR_POOL_ITEM pool_item = {0};
    strcpy(pool_item.woof_name, woof_name);
    strcpy(pool_item.handler, handler);
    pool_item.seq_no = (uint64_t)seq_no;
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
            sprintf(monitor_error_msg,
                    "failed to malloc buffer with size %lu for %s",
                    pool_item_size,
                    pool_item->woof_name);
            return -1;
        }
        if (WooFGet(pool_item->woof_name, buf, (unsigned long)pool_item->seq_no) < 0) {
            free(buf);
            sprintf(monitor_error_msg, "failed to WooFGet %lu from %s", pool_item->seq_no, pool_item->woof_name);
            return -1;
        }
        memcpy(element, buf, size);
        free(buf);
        return 0;
    }
    if (WooFGet(pool_item->woof_name, element, (unsigned long)pool_item->seq_no) < 0) {
        sprintf(monitor_error_msg, "failed to WooFGet %lu from %s", pool_item->seq_no, pool_item->woof_name);
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
        sprintf(monitor_error_msg, "failed to WooFPut to done woof %s", done_woof);
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
    //     printf("handler %s done", pool_item->handler);
    // }
    seq = WooFPut(invoker_woof, "monitor_invoker", &invoker_arg);
    if (WooFInvalid(seq)) {
        sprintf(monitor_error_msg, "failed to spinlock on %s", invoker_woof);
        return -1;
    }
}

int monitor_join() {
    int i;
    for (i = 0; i < MONITOR_THREAD_POOL_SIZE; ++i) {
        if (thread_id[i] != 0) {
            pthread_join(thread_id[i], NULL);
        }
    }
}