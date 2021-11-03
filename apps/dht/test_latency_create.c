#include "woofc-host.h"
#include "woofc.h"

#include <stdio.h>

typedef struct test_latency_stc {
    unsigned long sent;
} TEST_LATENCY_EL;

int main(int argc, char** argv) {
    WooFInit();

    if (WooFCreate("test_latency.woof", sizeof(TEST_LATENCY_EL), 128) < 0) {
        fprintf(stderr, "failed to create test_latency.woof\n");
        exit(1);
    }

    return 0;
}
