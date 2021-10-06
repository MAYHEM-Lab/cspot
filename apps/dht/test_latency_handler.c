#include "dht_utils.h"
#include "woofc.h"

#include <stdio.h>

typedef struct test_stc {
    unsigned long sent;
} TEST_EL;

int test_latency_handler(WOOF* wf, unsigned long seq_no, void* ptr) {
    TEST_EL* el = (TEST_EL*)ptr;
    printf("test_latency_handler: %lu\n", get_milliseconds() - el->sent);
    return 1;
}