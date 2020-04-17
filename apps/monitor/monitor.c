#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "woofc.h"
#include "monitor.h"

int monitor_create(char *monitor_name) {
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

	MONITOR_INVOKER_ARG invoker_arg;
	sprintf(invoker_arg.pool_woof, "%s_%s", monitor_name, MONITOR_POOL_WOOF);
	sprintf(invoker_arg.done_woof, "%s_%s", monitor_name, MONITOR_DONE_WOOF);
	sprintf(invoker_arg.handler_woof, "%s_%s", monitor_name, MONITOR_HANDLER_WOOF);
	unsigned long seq = WooFPut(invoker_woof, "monitor_invoker", &invoker_arg);
	if (WooFInvalid(seq)) {
		return -1;
	}
	return 0;
}

unsigned long monitor_put(char *monitor_name, char *woof_name, char *handler, void *ptr) {
	WOOF *wf = WooFOpen(woof_name);
	if (wf == NULL) {
		return -1;
	}
	unsigned long woof_seq = WooFAppend(wf, NULL, ptr);
	if (WooFInvalid(woof_seq)) {
		WooFFree(wf);
		return -1;
	}
	MONITOR_POOL_ITEM pool_item;
	strcpy(pool_item.woof_name, woof_name);
	strcpy(pool_item.handler, handler);
	pool_item.seq_no = woof_seq;
	pool_item.element_size = wf->shared->element_size;
	strcpy(pool_item.monitor_name, monitor_name);
	WooFFree(wf);
	
	char pool_woof[MONITOR_WOOF_NAME_LENGTH];
	sprintf(pool_woof, "%s_%s", monitor_name, MONITOR_POOL_WOOF);
	unsigned long monitor_seq = WooFPut(pool_woof, NULL, &pool_item);
	if (WooFInvalid(monitor_seq)) {
		return -1;
	}
	return woof_seq;
}

void *monitor_cast(void *ptr) {
	MONITOR_POOL_ITEM *pool_item = (MONITOR_POOL_ITEM *)ptr;
	void *element = malloc(pool_item->element_size);
	int err = WooFGet(pool_item->woof_name, element, pool_item->seq_no);
	if (err < 0) {
		return NULL;
	}
	return element;
}

int monitor_exit(void *ptr) {
	MONITOR_POOL_ITEM *pool_item = (MONITOR_POOL_ITEM *)ptr;

	MONITOR_DONE_ITEM done_item;
	char done_woof[MONITOR_WOOF_NAME_LENGTH];
	sprintf(done_woof, "%s_%s", pool_item->monitor_name, MONITOR_DONE_WOOF);
	unsigned long seq = WooFPut(done_woof, NULL, &done_item);
	if (WooFInvalid(seq)) {
		return -1;
	}

	MONITOR_INVOKER_ARG invoker_arg;
	sprintf(invoker_arg.pool_woof, "%s_%s", pool_item->monitor_name, MONITOR_POOL_WOOF);
	sprintf(invoker_arg.done_woof, "%s_%s", pool_item->monitor_name, MONITOR_DONE_WOOF);
	sprintf(invoker_arg.handler_woof, "%s_%s", pool_item->monitor_name, MONITOR_HANDLER_WOOF);
	char invoker_woof[MONITOR_WOOF_NAME_LENGTH];
	sprintf(invoker_woof, "%s_%s", pool_item->monitor_name, MONITOR_INVOKER_WOOF);
	seq = WooFPut(invoker_woof, "monitor_invoker", &invoker_arg);
	if (WooFInvalid(seq)) {
		fprintf(stderr, "failed to spinlock on %s\n", invoker_woof);
		exit(1);
	}
}

void monitor_set_spinlock_delay(int milliseconds) {
	monitor_spinlock_delay = milliseconds;
}