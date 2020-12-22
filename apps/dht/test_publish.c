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

    if (argc >= 3) {
        dht_set_client_ip(argv[2]);
    }

    DHT_SERVER_PUBLISH_FIND_ARG arg = {0};
    sprintf(arg.topic_name, "%s", TEST_TOPIC);
    TEST_EL* el = (TEST_EL*)arg.element;
    sprintf(el->msg, argv[1]);
    el->sent = get_milliseconds();
    arg.element_size = sizeof(TEST_EL);
    
    unsigned long seq = dht_publish(&arg);
    if (WooFInvalid(seq)) {
        fprintf(stderr, "failed to publish to topic: %s\n", dht_error_msg);
        exit(1);
    }
    printf("%s published to topic at %lu\n", el->msg, el->sent);
    return 0;
}
