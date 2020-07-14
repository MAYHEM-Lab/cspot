#include "dht.h"
#include "dht_client.h"
#include "dht_utils.h"
#include "woofc-host.h"
#include "woofc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_HANDLER "test_dht_handler"

typedef struct test_stc {
    char msg[256 - 8];
    unsigned long sent;
} TEST_EL;

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: ./test_trigger_publish topic\n");
        exit(1);
    }
    WooFInit();

    TEST_EL el = {0};
    sprintf(el.msg, "test_trigger_publish");
    el.sent = get_milliseconds();

    if (argc >= 3) {
        dht_set_client_ip(argv[2]);
    }

    unsigned long seq = dht_publish(argv[1], &el);
    if (WooFInvalid(seq)) {
        fprintf(stderr, "failed to publish to topic: %s\n", dht_error_msg);
        exit(1);
    }
    printf("%s published to topic at %lu\n", el.msg, el.sent);
    return 0;
}
