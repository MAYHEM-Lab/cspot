#include "bldg_control.h"
#include "dht_client.h"
#include "dht_utils.h"

#include <stdio.h>

int h_hourly_change(char* woof_name, char* topic_name, unsigned long seq_no, void* ptr) {
    COUNT_EL* arg = (COUNT_EL*)ptr;

    unsigned long now = get_milliseconds();
    unsigned long one_hour_earlier = now - (60 * 60 * 1000);

    unsigned long seq = dht_topic_latest_seqno(TOPIC_BUILDING_TOTAL);
    if (WooFInvalid(seq)) {
        fprintf(stderr, "failed to get latest seqno of %s\n", TOPIC_BUILDING_TOTAL);
        exit(1);
    }
    printf("latest %s seq: %lu\n", TOPIC_BUILDING_TOTAL, seq);

    COUNT_EL prev_total = {0};
    unsigned long i;
    for (i = seq - 1; i != 0; --i) {
        int err = dht_topic_get_range(TOPIC_BUILDING_TOTAL, &prev_total, sizeof(COUNT_EL), i, one_hour_earlier, 0);
        if (err < 0) {
            break;
        }
    }
    COUNT_EL change = {0};
    change.count = arg->count - prev_total.count;
    printf("change: %d - %d = %d\n", arg->count, prev_total.count, change.count);

    seq = dht_publish(TOPIC_HOURLY_CHANGE, &change, sizeof(COUNT_EL));
    if (WooFInvalid(seq)) {
        fprintf(stderr, "failed to publish total count to %s\n", TOPIC_HOURLY_CHANGE);
        exit(1);
    }
    printf("published to %s\n", TOPIC_HOURLY_CHANGE);

    return 1;
}