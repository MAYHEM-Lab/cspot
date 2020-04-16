#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "monitor.h"
#include "monitor_test.h"

int monitor_test_handler(WOOF *wf, unsigned long seq_no, void *ptr) {
	MONITOR_TEST_ARG *arg = (MONITOR_TEST_ARG *)ptr;
	int i;
	MONITOR_TEST_COUNTER counter;
	for (i = 0; i < 100; ++i) {
		unsigned long seq = WooFGetLatestSeqno(MONITOR_TEST_COUNTER_WOOF);
		if (WooFInvalid(seq)) {
			fprintf(stderr, "can't get the latest seq_no of %s\n", MONITOR_TEST_COUNTER_WOOF);
			exit(1);
		}
		int err = WooFGet(MONITOR_TEST_COUNTER_WOOF, &counter, seq);
		if (err < 0) {
			fprintf(stderr, "can't get the latest counter %lu from %s", seq, MONITOR_TEST_COUNTER_WOOF);
			exit(1);
		}
		counter.counter++;
		seq = WooFPut(MONITOR_TEST_COUNTER_WOOF, NULL, &counter);
		if (WooFInvalid(seq)) {
			fprintf(stderr, "can't increment the counter\n");
			exit(1);
		}
	}
	printf("[%lu] counter: %d\n", seq_no, counter.counter);
	if (arg->monitored == 1) {
		int err = monitor_exit(wf, seq_no);
		if (err < 0) {
			fprintf(stderr, "failed to notify the monitor\n");
			exit(1);
		}
	}
	return 0;
}
