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

typedef struct test_stc {
    char msg[256 - 8];
    unsigned long sent;
} TEST_EL;

int main(int argc, char** argv) {
    WooFInit();

    char* topic_addr = NULL;
    if (argc >= 2) {
        topic_addr = malloc(DHT_NAME_LENGTH * sizeof(char));
        strcpy(topic_addr, argv[1]);
    }

    if (dht_create_topic(TEST_TOPIC, sizeof(TEST_EL), DHT_HISTORY_LENGTH_SHORT) < 0) {
        fprintf(stderr, "failed to create topic: %s\n", dht_error_msg);
        free(topic_addr);
        exit(1);
    }
    printf("topic created\n");
    usleep(500 * 1000);

    if (dht_register_topic(TEST_TOPIC, topic_addr) < 0) {
        fprintf(stderr, "failed to register topic: %s\n", dht_error_msg);
        free(topic_addr);
        exit(1);
    }
    printf("topic registered\n");
    usleep(500 * 1000);

    if (dht_subscribe(TEST_TOPIC, topic_addr, TEST_HANDLER) < 0) {
        fprintf(stderr, "failed to subscribe to topic: %s\n", dht_error_msg);
        free(topic_addr);
        exit(1);
    }
    free(topic_addr);
    printf("handler subscribed to topic\n");
    usleep(500 * 1000);

    int i;
    for (i = 0; i < TEST_COUNT; ++i) {
        TEST_EL el = {0};
        sprintf(el.msg, "%s", TEST_MESSAGE);
        el.sent = get_milliseconds();
        int err = dht_publish(TEST_TOPIC, &el, sizeof(TEST_EL));
        if (err < 0) {
            fprintf(stderr, "failed to publish to topic: %s\n", dht_error_msg);
            exit(1);
        }
        printf("%s published to topic at %lu\n", el.msg, el.sent);
    }
    printf("test done\n");
    return 0;
}
