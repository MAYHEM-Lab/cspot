#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

#include "woofc.h"
#include "monitor.h"

int monitor_invoker(WOOF *wf, unsigned long seq_no, void *ptr) {
	MONITOR_INVOKER_ARG *arg = (MONITOR_INVOKER_ARG *)ptr;

	unsigned long last_done = WooFGetLatestSeqno(arg->done_woof);
	if (WooFInvalid(last_done)) {
		fprintf(stderr, "can't get the last done seqno from %s\n", arg->done_woof);
		exit(1);
	}
	unsigned long last_queued = WooFGetLatestSeqno(arg->pool_woof);
	if (WooFInvalid(last_queued)) {
		fprintf(stderr, "can't get the last queued seqno from %s\n", arg->pool_woof);
		exit(1);
	}
	if (last_done + 1 <= last_queued) {
		MONITOR_POOL_ITEM pool_item;
		if (WooFGet(arg->pool_woof, &pool_item, last_done + 1) < 0) {
			fprintf(stderr, "failed to get queued handler information from %s\n", arg->pool_woof);
			exit(1);
		}
		unsigned long seq = WooFPut(arg->handler_woof, pool_item.handler, &pool_item);
		if (WooFInvalid(seq)) {
			fprintf(stderr, "failed to invoke handler %s\n", pool_item.handler);
			exit(1);
		}
	} else {
		usleep(monitor_spinlock_delay * 1000);
		unsigned long seq = WooFPut(arg->handler_woof, "monitor_invoker", arg);
		if (WooFInvalid(seq)) {
			fprintf(stderr, "failed to spinlock on %s\n", arg->handler_woof);
			exit(1);
		}
	}

	return 0;
}
