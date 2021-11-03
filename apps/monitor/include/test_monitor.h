#ifndef TEST_MONITOR_H
#define TEST_MONITOR_H

#define TEST_MONITOR_ARG_WOOF "test_monitor_arg.woof"
#define TEST_MONITOR_COUNTER_WOOF "test_monitor_counter.woof"

typedef struct test_monitor_arg {
    char msg[256];
} TEST_MONITOR_ARG;

typedef struct test_monitor_counter {
    int counter;
} TEST_MONITOR_COUNTER;

#endif
