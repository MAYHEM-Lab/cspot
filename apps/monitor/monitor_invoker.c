#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "woofc.h"
#include "monitor.h"

int monitor_invoker(WOOF *wf, unsigned long seq_no, void *ptr) {
	MONITOR_INVOKER_ARG *arg = (MONITOR_INVOKER_ARG *)ptr;

	char woof_name[MONITOR_WOOF_NAME_LENGTH];
	sprintf(woof_name, "%s_%s", arg->monitor_name, MONITOR_DONE_WOOF);
	unsigned long last_done = WooFGetLatestSeqno(woof_name);
	if (WooFInvalid(last_done)) {
		fprintf(stderr, "can't get the last done seqno from %s\n", woof_name);
		exit(1);
	}
	sprintf(woof_name, "%s_%s", arg->monitor_name, MONITOR_POOL_WOOF);
	WOOF *pool_wf = WooFOpen(woof_name);
	if (pool_wf == NULL) {
		fprintf(stderr, "can't open woof %s\n", woof_name);
		exit(1);
	}
	unsigned long last_queued = WooFLatestSeqno(pool_wf);
	if (WooFInvalid(last_queued)) {
		fprintf(stderr, "can't get the last queued seqno from %s\n", woof_name);
		WooFFree(pool_wf);
		exit(1);
	}
	unsigned long to_invoke = arg->last_invoked + 1;
	if (to_invoke <= last_queued && to_invoke == last_done + 1) {
		void *element = malloc(pool_wf->shared->element_size * 10);
		if (element == NULL) {
			fprintf(stderr, "failed to allocate memory for element with size %lu\n", pool_wf->shared->element_size);
			WooFFree(pool_wf);
		}
		int err = WooFRead(pool_wf, element, to_invoke);
		if (err < 0) {
			fprintf(stderr, "can't get queued handler infomation\n");
			free(element);
			WooFFree(pool_wf);
			exit(1);
		}
		MONITOR_POOL_ITEM *pool_item = (MONITOR_POOL_ITEM *)element;
		unsigned long seq = WooFPut(arg->monitor_name, pool_item->handler_name, element + sizeof(MONITOR_POOL_ITEM));
		if (WooFInvalid(seq)) {
			fprintf(stderr, "can't invoke queued handler %s in woof %s\n", pool_item->handler_name, arg->monitor_name);
			free(element);
			WooFFree(pool_wf);
			exit(1);
		}
		arg->last_invoked = to_invoke;
	} else {
		usleep(monitor_spinlock_delay * 1000);
	}

	WooFFree(pool_wf);
	unsigned long seq = WooFPut(wf->shared->filename, "monitor_invoker", arg);
	if (WooFInvalid(seq)) {
		fprintf(stderr, "failed to invoke the next invoker\n");
		exit(1);
	}
	return 0;
}
