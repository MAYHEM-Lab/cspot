#ifndef MONITOR_H
#define MONITOR_H

#define MONITOR_POOL_WOOF "monitor_pool.woof"
#define MONITOR_DONE_WOOF "monitor_done.woof"
#define MONITOR_HANDLER_WOOF "monitor_handler.woof"
#define MONITOR_INVOKER_WOOF "monitor_invoker.woof"
#define MONITOR_WOOF_NAME_LENGTH 256
#define MONITOR_HISTORY_LENGTH 256

int monitor_spinlock_delay;

typedef struct monitor_pool_item {
	char woof_name[MONITOR_WOOF_NAME_LENGTH];
	char handler[MONITOR_WOOF_NAME_LENGTH];
	unsigned long seq_no;
	unsigned long element_size;
	char monitor_name[MONITOR_WOOF_NAME_LENGTH];
} MONITOR_POOL_ITEM;

typedef struct monitor_done_item {
	// unsigned long pool_seq_no;
} MONITOR_DONE_ITEM;

typedef struct monitor_invoker_arg {
	char pool_woof[MONITOR_WOOF_NAME_LENGTH];
	char done_woof[MONITOR_WOOF_NAME_LENGTH];
	char handler_woof[MONITOR_WOOF_NAME_LENGTH];
} MONITOR_INVOKER_ARG;

int monitor_create(char *monitor_name);
unsigned long monitor_put(char *monitor_name, char *woof_name, char *handler, void *ptr);
void *monitor_cast(void *ptr);
int monitor_exit(void *ptr);
void monitor_set_spinlock_delay(int milliseconds);

#endif

