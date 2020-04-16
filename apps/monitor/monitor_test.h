#ifndef MONITOR_TEST_H
#define MONITOR_TEST_H

#define MONITOR_TEST_ARG_WOOF "monitor_test_arg.woof"
#define MONITOR_TEST_COUNTER_WOOF "monitor_test_counter.woof"

typedef struct monitor_test_arg {
	int monitored;
} MONITOR_TEST_ARG;

typedef struct monitor_test_counter {
	int counter;
} MONITOR_TEST_COUNTER;

#endif

