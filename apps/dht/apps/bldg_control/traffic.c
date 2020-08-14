#include "bldg_control.h"
#include "dht_client.h"
#include "woofc-host.h"
#include "woofc.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    WooFInit();

    TRAFFIC_EL traffic = {0};
    if (strcmp(argv[2], "e") == 0) {
        traffic.type = TRAFFIC_EGRESS;
    } else if (strcmp(argv[2], "i") == 0) {
        traffic.type = TRAFFIC_INGRESS;
    }

    char topic[DHT_NAME_LENGTH] = {0};
    if (strcmp(argv[1], "101") == 0) {
        sprintf(topic, TOPIC_ROOM_101_TRAFFIC);
    } else if (strcmp(argv[1], "102") == 0) {
        sprintf(topic, TOPIC_ROOM_102_TRAFFIC);
    } else if (strcmp(argv[1], "103") == 0) {
        sprintf(topic, TOPIC_ROOM_103_TRAFFIC);
    } else if (strcmp(argv[1], "201") == 0) {
        sprintf(topic, TOPIC_ROOM_201_TRAFFIC);
    } else if (strcmp(argv[1], "202") == 0) {
        sprintf(topic, TOPIC_ROOM_202_TRAFFIC);
    } else if (strcmp(argv[1], "gate") == 0) {
        sprintf(topic, TOPIC_BUILDING_GATE_TRAFFIC);
    }

    unsigned long index = dht_publish(topic, &traffic, sizeof(TRAFFIC_EL), 5000);
    if (WooFInvalid(index)) {
        fprintf(stderr, "failed to publish to topic %s\n", topic);
        exit(1);
    }

    return 0;
}
