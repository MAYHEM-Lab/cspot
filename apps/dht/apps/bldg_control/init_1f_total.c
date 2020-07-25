#include "bldg_control.h"
#include "dht_client.h"
#include "woofc-host.h"
#include "woofc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ARGS "i:"
char* Usage = "init_1f_total -i client_ip\n";

int main(int argc, char** argv) {
    char client_ip[DHT_NAME_LENGTH] = {0};

    int c;
    while ((c = getopt(argc, argv, ARGS)) != EOF) {
        switch (c) {
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
    WooFInit();

    if (client_ip[0] != 0) {
        dht_set_client_ip(client_ip);
    }

    if (dht_create_topic(TOPIC_1F_TOTAL, sizeof(COUNT_EL), BLDG_HISTORY_LENGTH) < 0) {
        fprintf(stderr, "failed to create topic woof: %s\n", dht_error_msg);
        exit(1);
    }

    if (dht_register_topic(TOPIC_1F_TOTAL) < 0) {
        fprintf(stderr, "failed to register topic on DHT: %s\n", dht_error_msg);
        exit(1);
    }

    sleep(2);

    COUNT_EL count = {0};
    count.count = 0;
    unsigned long seq = dht_publish(TOPIC_1F_TOTAL, &count, sizeof(COUNT_EL));
    if (WooFInvalid(seq)) {
        fprintf(stderr, "failed to initialize %s\n", TOPIC_1F_TOTAL);
    }

    if (dht_subscribe(TOPIC_ROOM_101_TRAFFIC, "h_floor_total") < 0) {
        fprintf(stderr, "failed to subscribe to topic: %s\n", dht_error_msg);
        exit(1);
    }
    if (dht_subscribe(TOPIC_ROOM_102_TRAFFIC, "h_floor_total") < 0) {
        fprintf(stderr, "failed to subscribe to topic: %s\n", dht_error_msg);
        exit(1);
    }
    if (dht_subscribe(TOPIC_ROOM_103_TRAFFIC, "h_floor_total") < 0) {
        fprintf(stderr, "failed to subscribe to topic: %s\n", dht_error_msg);
        exit(1);
    }

    return 0;
}
