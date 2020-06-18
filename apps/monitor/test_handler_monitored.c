#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "monitor.h"
#include "test_monitor.h"

int test_handler_monitored(WOOF *wf, unsigned long seq_no, void *ptr) {
	TEST_MONITOR_ARG *arg = (TEST_MONITOR_ARG *)monitor_cast(ptr);
	
	TEST_MONITOR_COUNTER counter;
	int i;
	for (i = 0; i < 100; ++i) {
		unsigned long seq = WooFGetLatestSeqno(TEST_MONITOR_COUNTER_WOOF);
		if (WooFInvalid(seq)) {
			fprintf(stderr, "can't get the latest seq_no of %s\n", TEST_MONITOR_COUNTER_WOOF);
			exit(1);
		}
		int err = WooFGet(TEST_MONITOR_COUNTER_WOOF, &counter, seq);
		if (err < 0) {
			fprintf(stderr, "can't get the latest counter %lu from %s", seq, TEST_MONITOR_COUNTER_WOOF);
			exit(1);
		}
		counter.counter++;
		seq = WooFPut(TEST_MONITOR_COUNTER_WOOF, NULL, &counter);
		if (WooFInvalid(seq)) {
			fprintf(stderr, "can't increment the counter\n");
			exit(1);
		}
	}
	printf("[%lu] counter: %d\n", seq_no, counter.counter);
	if (monitor_exit(ptr) < 0) {
		fprintf(stderr, "failed to notify the monitor\n");
		exit(1);
	}
	return 0;
}
