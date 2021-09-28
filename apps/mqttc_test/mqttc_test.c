#include "mqttc_test.h"
#include "time.h" //Check if CLOCK_REALTIME is now working
#include "woofc.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

uint64_t get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ((uint64_t)ts.tv_sec * 1000) + ((uint64_t)ts.tv_nsec / 1000000);
}

int mqttc_test(WOOF* wf, unsigned long seq_no, void* ptr) {
    MQTTC_TEST_EL* el = (MQTTC_TEST_EL*)ptr;
    unsigned long sent = strtoul(el->string, NULL, 10);
    unsigned long ts = get_time();
    printf("sent: %lu, received: %lu, latency: %lu\n", sent, ts, ts - sent);
    return 1;
}
