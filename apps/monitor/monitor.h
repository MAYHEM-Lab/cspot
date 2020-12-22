#ifndef MONITOR_H
#define MONITOR_H

#include <stdint.h>
#define MONITOR_POOL_WOOF "monitor_pool.woof"
#define MONITOR_DONE_WOOF "monitor_done.woof"
#define MONITOR_HANDLER_WOOF "monitor_handler.woof"
#define MONITOR_INVOKER_WOOF "monitor_invoker.woof"
#define MONITOR_WOOF_NAME_LENGTH 256
#define MONITOR_HISTORY_LENGTH 1024
#define MONITOR_SPINLOCK_DELAY 20
#define MONITOR_WARNING_QUEUED_HANDLERS 0

char monitor_error_msg[256];

typedef struct monitor_pool_item {
    char woof_name[MONITOR_WOOF_NAME_LENGTH];
    char handler[MONITOR_WOOF_NAME_LENGTH];
    uint64_t seq_no;
    char monitor_name[MONITOR_WOOF_NAME_LENGTH];
    uint64_t queued_ts;
    int32_t idempotent;
} MONITOR_POOL_ITEM;

typedef struct monitor_done_item {

} MONITOR_DONE_ITEM;

typedef struct monitor_invoker_arg {
    char pool_woof[MONITOR_WOOF_NAME_LENGTH];
    char done_woof[MONITOR_WOOF_NAME_LENGTH];
    char handler_woof[MONITOR_WOOF_NAME_LENGTH];
    int32_t spinlock_delay;
    int32_t wasted_cycle;
} MONITOR_INVOKER_ARG;

int monitor_init();
int monitor_create(char* monitor_name);
unsigned long monitor_put(char* monitor_name, char* woof_name, char* handler, void* ptr, int idempotent);
unsigned long monitor_queue(char* monitor_name, char* woof_name, char* handler, unsigned long seq_no);
unsigned long monitor_remote_put(char* monitor_uri, char* woof_uri, char* handler, void* ptr, int idempotent);
unsigned long
monitor_remote_queue(char* monitor_uri, char* woof_uri, char* handler, unsigned long seq_no, int idempotent);
int monitor_cast(void* ptr, void* element, unsigned long size);
unsigned long monitor_seqno(void* ptr);
int monitor_exit(void* ptr);
int monitor_join();

#endif
