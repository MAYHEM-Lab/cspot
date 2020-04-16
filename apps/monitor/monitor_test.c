#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "woofc.h"
#include "monitor.h"
#include "monitor_test.h"

#define ARGS "c:t:m"
char *Usage = "monitor_test\n\
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
	int err;
	if (monitored) {
		err = monitor_create(MONITOR_TEST_ARG_WOOF, sizeof(MONITOR_TEST_ARG), MONITOR_HISTORY_LENGTH);
	} else {
		err = WooFCreate(MONITOR_TEST_ARG_WOOF, sizeof(MONITOR_TEST_ARG), MONITOR_HISTORY_LENGTH);
	}
	if (err < 0) {
		fprintf(stderr, "can't create %s\n", MONITOR_TEST_ARG_WOOF);
		exit(1);
	}
	err = WooFCreate(MONITOR_TEST_COUNTER_WOOF, sizeof(MONITOR_TEST_COUNTER), MONITOR_HISTORY_LENGTH);
	if (err < 0) {
		fprintf(stderr, "can't create %s\n", MONITOR_TEST_COUNTER_WOOF);
		exit(1);
	}
	MONITOR_TEST_COUNTER counter;
	counter.counter = 0;
	unsigned long seq = WooFPut(MONITOR_TEST_COUNTER_WOOF, NULL, &counter);
	if (WooFInvalid(seq)) {
		fprintf(stderr, "can't initialize test counter\n");
		exit(1);
	}

	int i;
	for (i = 0; i < count; ++i) {
		MONITOR_TEST_ARG arg;
		unsigned long seq;
		if (monitored) {
			arg.monitored = 1;
			seq = monitor_put(MONITOR_TEST_ARG_WOOF, "monitor_test_handler", &arg);
		} else {
			arg.monitored = 0;
			seq = WooFPut(MONITOR_TEST_ARG_WOOF, "monitor_test_handler", &arg);
		}
		if (WooFInvalid(seq)) {
			fprintf(stderr, "can't invoke monitor_test_handler\n");
			exit(1);
		}
	}

	for (seq = 1; seq <= count; ++seq) {
		while (monitored && monitor_last_done(MONITOR_TEST_ARG_WOOF) < seq) {
			continue;
		}
		MONITOR_TEST_ARG arg;
		int err;
		if (monitored) {
			err = monitor_get(MONITOR_TEST_ARG_WOOF, &arg, seq);
		} else {
			err = WooFGet(MONITOR_TEST_ARG_WOOF, &arg, seq);
		}
		if (err < 0) {
			fprintf(stderr, "can't get the appended test arg at %lu\n", seq);
			exit(1);
		}
		printf("%lu: %d\n", seq, arg.monitored);
	}
	return 0;
}

