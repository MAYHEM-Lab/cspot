#include "dht.h"
#include "dht_client.h"
#include "dht_utils.h"
#include "woofc-host.h"
#include "woofc.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TEST_HANDLER "test_dht_handler"
#define ARGS "t:m:i:"
char* Usage = "test_trigger_publish -t topic -m message -i client_addr\n";

typedef struct test_stc {
    char msg[256 - 8];
    uint64_t sent;
} TEST_EL;

int main(int argc, char** argv) {
    DHT_SERVER_PUBLISH_FIND_ARG arg = {0};
    char message[256 - 8] = {0};
    char client_ip[DHT_NAME_LENGTH] = {0};

    int c;
    while ((c = getopt(argc, argv, ARGS)) != EOF) {
        switch (c) {
        case 't': {
            strncpy(arg.topic_name, optarg, sizeof(arg.topic_name));
            break;
        }
        case 'm': {
            strncpy(message, optarg, sizeof(message));
            break;
        }
        case 'i': {
            strncpy(client_ip, optarg, sizeof(client_ip));
            break;
        }
        default: {
            fprintf(stderr, "unrecognized command %c\n", (char)c);
            fprintf(stderr, "%s", Usage);
            exit(1);
        }
        }
    }

    if (arg.topic_name[0] == 0 || message[0] == 0) {
        fprintf(stderr, "%s", Usage);
        exit(1);
    }

    WooFInit();

    TEST_EL* el = (TEST_EL*)arg.element;
    sprintf(el->msg, message);
    el->sent = get_milliseconds();
    arg.element_size = sizeof(TEST_EL);
    arg.requested_ts = get_milliseconds();

    if (client_ip[0] != 0) {
        printf("set client ip to %s\n", client_ip);
        fflush(stdout);
        dht_set_client_ip(client_ip);
    }

    unsigned long seq = dht_publish(&arg);
    if (WooFInvalid(seq)) {
        fprintf(stderr, "failed to publish to topic: %s\n", dht_error_msg);
        exit(1);
    }

    return 0;
}