#include "dht_utils.h"
#include "woofc.h"

#include <stdio.h>
#include <stdlib.h>

typedef struct test_latency_stc {
    unsigned long sent;
} TEST_LATENCY_EL;

int main(int argc, char** argv) {
    WooFInit();

    char target_woof[DHT_NAME_LENGTH] = {0};
    sprintf(target_woof, "%s/test_latency.woof", argv[1]);

    TEST_LATENCY_EL el = {0};
    el.sent = get_milliseconds();
    unsigned long seq = WooFPut(target_woof, "test_latency_handler", &el);
    if (WooFInvalid(seq)) {
        fprintf(stderr, "failed to put\n");
        exit(1);
    }

    return 0;
}
