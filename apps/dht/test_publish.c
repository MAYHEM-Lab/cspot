#include "dht.h"
#include "dht_client.h"
#include "dht_utils.h"
#include "woofc-host.h"
#include "woofc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_TOPIC "test"
#define TEST_HANDLER "test_dht_handler"
#define TEST_TIMEOUT 5000

typedef struct test_stc {
    char msg[256 - 8];
    unsigned long sent;
} TEST_EL;

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: ./test_publish data\n");
        exit(1);
    }
    WooFInit();

    TEST_EL el = {0};
    sprintf(el.msg, argv[1]);
    el.sent = get_milliseconds();

    int err = dht_publish(TEST_TOPIC, &el, sizeof(TEST_EL));
    if (err < 0) {
        fprintf(stderr, "failed to publish to topic: %s\n", dht_error_msg);
        exit(1);
    }
    printf("%s published to topic at %lu\n", el.msg, el.sent);
    return 0;
}
