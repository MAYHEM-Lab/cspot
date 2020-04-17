#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "woofc.h"
#include "monitor.h"
#include "test_monitor.h"

#define MONITOR_NAME "monitor_test"
#define ARGS "c:t:m"
char *Usage = "test_monitor\n\
	\t-c number of runs, default to 10\n\
	\t-t spinlock delay (ms), default to 0\n\
	\t-m to monitor handlers\n";

int main(int argc, char **argv) {
	int monitored = 0;
	int count = 10;
	int c;
	while ((c = getopt(argc, argv, ARGS)) != EOF) {
		switch (c) {
			case 'c': {
				count = atoi(optarg);
				break;
			}
			case 'm': {
				monitored = 1;
				break;
			}
			case 't': {
				monitor_set_spinlock_delay(atoi(optarg));
				break;
			}
			default: {
				fprintf(stderr, "unrecognized command %c\n", (char)c);
				exit(1);
			}
		}
	}

	WooFInit();
	if (monitored) {
		if (monitor_create(MONITOR_NAME) < 0) {
			fprintf(stderr, "can't create monitor %s\n", MONITOR_NAME);
			exit(1);
		}
	}

	if (WooFCreate(TEST_MONITOR_ARG_WOOF, sizeof(TEST_MONITOR_ARG), MONITOR_HISTORY_LENGTH) < 0) {
		fprintf(stderr, "can't create %s\n", TEST_MONITOR_ARG_WOOF);
		exit(1);
	}
	if (WooFCreate(TEST_MONITOR_COUNTER_WOOF, sizeof(TEST_MONITOR_COUNTER), MONITOR_HISTORY_LENGTH) < 0) {
		fprintf(stderr, "can't create %s\n", TEST_MONITOR_COUNTER_WOOF);
		exit(1);
	}
	TEST_MONITOR_COUNTER counter;
	counter.counter = 0;
	unsigned long seq = WooFPut(TEST_MONITOR_COUNTER_WOOF, NULL, &counter);
	if (WooFInvalid(seq)) {
		fprintf(stderr, "can't initialize test counter\n");
		exit(1);
	}

	int i;
	for (i = 0; i < count; ++i) {
		TEST_MONITOR_ARG arg;
		if (monitored) {
			sprintf(arg.msg, "monitored %d", i);
			unsigned long seq = monitor_put(MONITOR_NAME, TEST_MONITOR_ARG_WOOF, "test_handler_monitored", &arg);
			if (WooFInvalid(seq)) {
				fprintf(stderr, "can't invoke test_handler_monitored\n");
				exit(1);
			}
		} else {
			sprintf(arg.msg, "concurrent %d", i);
			unsigned long seq = WooFPut(TEST_MONITOR_ARG_WOOF, "test_handler_concurrent", &arg);
			if (WooFInvalid(seq)) {
				fprintf(stderr, "can't invoke test_handler_concurrent\n");
				exit(1);
			}
		}
	}

	return 0;
}

