#include "bldg_control.h"
#include "dht_client.h"

#include <stdio.h>

int h_floor_total(char* woof_name, char* topic_name, unsigned long seq_no, void* ptr) {
    TRAFFIC_EL* arg = (TRAFFIC_EL*)ptr;

    char floor_topic[DHT_NAME_LENGTH] = {0};
    if ((strcmp(topic_name, TOPIC_ROOM_101_TRAFFIC) == 0) || (strcmp(topic_name, TOPIC_ROOM_102_TRAFFIC) == 0) ||
        (strcmp(topic_name, TOPIC_ROOM_103_TRAFFIC) == 0)) {
        sprintf(floor_topic, TOPIC_1F_TOTAL);
    } else if ((strcmp(topic_name, TOPIC_ROOM_201_TRAFFIC) == 0) || (strcmp(topic_name, TOPIC_ROOM_202_TRAFFIC) == 0)) {
        sprintf(floor_topic, TOPIC_2F_TOTAL);
    }

    unsigned long seq = dht_topic_latest_seqno(floor_topic);
    if (WooFInvalid(seq)) {
        fprintf(stderr, "failed to get latest seqno of %s\n", floor_topic);
        exit(1);
    }
    printf("latest %s seq: %lu\n", floor_topic, seq);

    COUNT_EL total = {0};
    if (dht_topic_get(floor_topic, &total, sizeof(COUNT_EL), seq) < 0) {
        fprintf(stderr, "failed to get data from topic %s[%lu]\n", floor_topic, seq);
        exit(1);
    }
    printf("previous count: %d\n", total.count);

    if (arg->type == TRAFFIC_INGRESS) {
        ++total.count;
    } else if (arg->type == TRAFFIC_EGRESS) {
        --total.count;
    }
    printf("after count: %d\n", total.count);

    seq = dht_publish(floor_topic, &total, sizeof(COUNT_EL));
    if (WooFInvalid(seq)) {
        fprintf(stderr, "failed to publish total count to %s\n", floor_topic);
        exit(1);
    }
    printf("published to %s\n", floor_topic);

    return 1;
}