#ifndef MONITOR_H
#define MONITOR_H

#define MONITOR_INVOKER_WOOF "monitor_invoker.woof"
#define MONITOR_POOL_WOOF "monitor_pool.woof"
#define MONITOR_DONE_WOOF "monitor_done.woof"
#define MONITOR_WOOF_NAME_LENGTH 256
#define MONITOR_HISTORY_LENGTH 256

int monitor_spinlock_delay;

typedef struct monitor_invoker_arg {
	char monitor_name[MONITOR_WOOF_NAME_LENGTH];
	unsigned long last_invoked;
} MONITOR_INVOKER_ARG;

typedef struct monitor_pool_item {
	char handler_name[MONITOR_WOOF_NAME_LENGTH];
	char woof_name[MONITOR_WOOF_NAME_LENGTH];
} MONITOR_POOL_ITEM;

typedef struct monitor_done_item {
	unsigned long seq_no;
} MONITOR_DONE_ITEM;

int monitor_create(char *woof_name, unsigned long element_size, unsigned long history_size);
unsigned long monitor_put(char *woof_name, char handler[MONITOR_WOOF_NAME_LENGTH], void *ptr);
int monitor_get(char *woof_name, void *element, unsigned long seq);
int monitor_exit(WOOF *wf, unsigned long seq_no);
unsigned long monitor_last_done(char *woof_name);
void monitor_set_spinlock_delay(int milliseconds);

#endif

