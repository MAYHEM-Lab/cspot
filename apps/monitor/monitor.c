#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "woofc.h"
#include "monitor.h"

int monitor_create(char *woof_name, unsigned long element_size, unsigned long history_size) {
	int err = WooFCreate(woof_name, element_size, history_size);
	if (err < 0) {
		fprintf(stderr, "failed to create %s", woof_name);
		return -1;
	}
	char invoker_woof[MONITOR_WOOF_NAME_LENGTH];
	sprintf(invoker_woof, "%s_%s", woof_name, MONITOR_INVOKER_WOOF);
	err = WooFCreate(invoker_woof, sizeof(MONITOR_INVOKER_ARG), MONITOR_HISTORY_LENGTH);
	if (err < 0) {
		fprintf(stderr, "failed to create %s", invoker_woof);
		return -1;
	}
	char pool_woof[MONITOR_WOOF_NAME_LENGTH];
	sprintf(pool_woof, "%s_%s", woof_name, MONITOR_POOL_WOOF);
	err = WooFCreate(pool_woof, sizeof(MONITOR_POOL_ITEM) + element_size, MONITOR_HISTORY_LENGTH);
	if (err < 0) {
		fprintf(stderr, "failed to create %s", pool_woof);
		return -1;
	}
	char done_woof[MONITOR_WOOF_NAME_LENGTH];
	sprintf(done_woof, "%s_%s", woof_name, MONITOR_DONE_WOOF);
	err = WooFCreate(done_woof, sizeof(MONITOR_DONE_ITEM), MONITOR_HISTORY_LENGTH);
	if (err < 0) {
		fprintf(stderr, "failed to create %s", done_woof);
		return -1;
	}

	MONITOR_INVOKER_ARG invoker_arg;
	invoker_arg.last_invoked = 0;
	strcpy(invoker_arg.monitor_name, woof_name);
	unsigned long seq = WooFPut(invoker_woof, "monitor_invoker", &invoker_arg);
	if (WooFInvalid(seq)) {
		fprintf(stderr, "failed to start invoker handler\n");
		return -1;
	}
	return 0;
}

unsigned long monitor_put(char *woof_name, char handler[MONITOR_WOOF_NAME_LENGTH], void *ptr) {
	// create pool item for later invocation
	MONITOR_POOL_ITEM pool_item;
	strcpy(pool_item.woof_name, woof_name);
	strcpy(pool_item.handler_name, handler);

	// allocate memory for element and pool item
	WOOF *wf = WooFOpen(woof_name);
	if (wf == NULL) {
		fprintf(stderr, "failed to open woof %s\n", woof_name);
		return -1;
	}
	void *element = malloc(sizeof(MONITOR_POOL_ITEM) + wf->shared->element_size);
	memcpy(element, &pool_item, sizeof(MONITOR_POOL_ITEM));
	memcpy(element + sizeof(MONITOR_POOL_ITEM), ptr, wf->shared->element_size);
	WooFFree(wf);

	char pool_woof[MONITOR_WOOF_NAME_LENGTH];
	sprintf(pool_woof, "%s_%s", woof_name, MONITOR_POOL_WOOF);
	unsigned long seq = WooFPut(pool_woof, NULL, element);
	free(element);
	if (WooFInvalid(seq)) {
		fprintf(stderr, "failed to queue handler %s to monitor %s\n", pool_item.handler_name, wf->shared->filename);
		return -1;
	}
	return seq;
}

int monitor_get(char *woof_name, void *element, unsigned long seq) {
	char done_woof[MONITOR_WOOF_NAME_LENGTH];
	sprintf(done_woof, "%s_%s", woof_name, MONITOR_DONE_WOOF);
	unsigned long last_done = WooFGetLatestSeqno(done_woof);
	if (WooFInvalid(last_done)) {
		fprintf(stderr, "failed get the latest seqno from %s\n", done_woof);
		return -1;
	}
	if (seq < last_done) {
		fprintf(stderr, "handler not done yet\n");
		return -1;
	}
	MONITOR_DONE_ITEM done_item;
	int err = WooFGet(done_woof, &done_item, seq);
	if (err < 0) {
		fprintf(stderr, "failed to get done handler information\n");
		return -1;
	}
	return WooFGet(woof_name, element, done_item.seq_no);
}

int monitor_exit(WOOF *wf, unsigned long seq_no) {
	MONITOR_DONE_ITEM done_item;
	done_item.seq_no = seq_no;
	char done_woof[MONITOR_WOOF_NAME_LENGTH];
	sprintf(done_woof, "%s_%s", wf->shared->filename, MONITOR_DONE_WOOF);
	unsigned long seq = WooFPut(done_woof, NULL, &done_item);
	if (WooFInvalid(seq)) {
		fprintf(stderr, "failed to notify the monitor %s that the handler is done\n", wf->shared->filename);
		return -1;
	}
	return 0;
}

unsigned long monitor_last_done(char *woof_name) {
	char done_woof[MONITOR_WOOF_NAME_LENGTH];
	sprintf(done_woof, "%s_%s", woof_name, MONITOR_DONE_WOOF);
	return WooFGetLatestSeqno(done_woof);
}

void monitor_set_spinlock_delay(int milliseconds) {
	monitor_spinlock_delay = milliseconds;
}