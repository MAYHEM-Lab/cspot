#include "bldg_control.h"
#include "dht_client.h"

#include <stdio.h>

int h_bldg_total(char* woof_name, char* topic_name, unsigned long seq_no, void* ptr) {
    TRAFFIC_EL* arg = (TRAFFIC_EL*)ptr;

    unsigned long seq = dht_topic_latest_seqno(TOPIC_BUILDING_TOTAL);
    if (WooFInvalid(seq)) {
        fprintf(stderr, "failed to get latest seqno of %s\n", TOPIC_BUILDING_TOTAL);
        exit(1);
    }
    printf("latest %s seq: %lu\n", TOPIC_BUILDING_TOTAL, seq);

    COUNT_EL total = {0};
    if (dht_topic_get(TOPIC_BUILDING_TOTAL, &total, sizeof(COUNT_EL), seq) < 0) {
        fprintf(stderr, "failed to get data from topic %s[%lu]\n", TOPIC_BUILDING_TOTAL, seq);
        exit(1);
    }
    printf("previous count: %d\n", total.count);

    if (arg->type == TRAFFIC_INGRESS) {
        ++total.count;
    } else if (arg->type == TRAFFIC_EGRESS) {
        --total.count;
    }
    printf("after count: %d\n", total.count);

    seq = dht_publish(TOPIC_BUILDING_TOTAL, &total, sizeof(COUNT_EL));
    if (WooFInvalid(seq)) {
        fprintf(stderr, "failed to publish total count to %s\n", TOPIC_BUILDING_TOTAL);
        exit(1);
    }
    printf("published to %s\n", TOPIC_BUILDING_TOTAL);

    return 1;
}