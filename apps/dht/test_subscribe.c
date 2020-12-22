#include "dht.h"
#include "dht_client.h"
#include "dht_utils.h"
#include "woofc-host.h"
#include "woofc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define TEST_TOPIC "test"
#define TEST_HANDLER "test_dht_handler"
#define TEST_MESSAGE "test_message"
#define TEST_COUNT 10
#define TEST_TIMEOUT 5000

typedef struct test_stc {
    char msg[256 - 8];
    unsigned long sent;
} TEST_EL;

int main(int argc, char** argv) {
    WooFInit();

    if (argc >= 2) {
        dht_set_client_ip(argv[1]);
    }

    if (dht_create_topic(TEST_TOPIC, sizeof(TEST_EL), DHT_HISTORY_LENGTH_SHORT) < 0) {
        fprintf(stderr, "failed to create topic: %s\n", dht_error_msg);
        exit(1);
    }
    printf("topic created\n");
    usleep(500 * 1000);

    if (dht_register_topic(TEST_TOPIC, TEST_TIMEOUT) < 0) {
        fprintf(stderr, "failed to register topic: %s\n", dht_error_msg);
        exit(1);
    }
    printf("topic registered\n");
    usleep(500 * 1000);

    if (dht_subscribe(TEST_TOPIC, TEST_HANDLER) < 0) {
        fprintf(stderr, "failed to subscribe to topic: %s\n", dht_error_msg);
        exit(1);
    }
    printf("handler subscribed to topic\n");
    usleep(500 * 1000);

    int i;
    for (i = 0; i < TEST_COUNT; ++i) {
        DHT_SERVER_PUBLISH_FIND_ARG arg = {0};
        sprintf(arg.topic_name, "%s", TEST_TOPIC);
        TEST_EL* el = (TEST_EL*)arg.element;
        sprintf(el->msg, "%s", TEST_MESSAGE);
        el->sent = get_milliseconds();
        arg.element_size = sizeof(TEST_EL);
        unsigned long seq = dht_publish(&arg);
        if (WooFInvalid(seq)) {
            fprintf(stderr, "failed to publish to topic: %s\n", dht_error_msg);
            exit(1);
        }
        printf("%s published to topic at %lu\n", el->msg, el->sent);
    }
    printf("test done\n");
    return 0;
}
