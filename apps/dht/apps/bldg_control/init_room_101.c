#include "bldg_control.h"
#include "dht_client.h"
#include "woofc-host.h"
#include "woofc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ARGS "i:"
char* Usage = "init_room_101 -i client_ip\n";

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

    if (dht_create_topic(TOPIC_ROOM_101_TRAFFIC, sizeof(TRAFFIC_EL), BLDG_HISTORY_LENGTH) < 0) {
        fprintf(stderr, "failed to create topic woof: %s\n", dht_error_msg);
        exit(1);
    }

    if (dht_register_topic(TOPIC_ROOM_101_TRAFFIC) < 0) {
        fprintf(stderr, "failed to register topic on DHT: %s\n", dht_error_msg);
        exit(1);
    }

    return 0;
}
