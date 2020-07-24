#include "avg_temp.h"
#include "dht_client.h"
#include "dht_utils.h"

#include <stdio.h>

int h_avg_temp(char* woof_name, char* topic_name, unsigned long seq_no, void* ptr) {
    unsigned long now = get_milliseconds();
    printf("received new temperature at %lu\n", now);
    printf("pulling previous temperature\n");

    unsigned long latest_topic_seqno = dht_remote_topic_latest_seqno(woof_name, topic_name);
    if (WooFInvalid(latest_topic_seqno)) {
        fprintf(stderr, "failed to get latest seqno of %s from %s\n", topic_name, woof_name);
        exit(1);
    }

    unsigned long one_hour_earlier = get_milliseconds() - (60 * 60 * 1000);
    double sum = 0;
    int cnt = 0;
    unsigned long i;
    for (i = latest_topic_seqno; i != 0; --i) {
        TEMP_EL prev = {0};
        int err = dht_remote_topic_get_range(woof_name, topic_name, &prev, sizeof(prev), i, one_hour_earlier, 0);
        if (err == -2) {
            break;
        } else if (err == -1) {
            fprintf(stderr, "failed to get topic data[%d]\n", i);
            exit(1);
        }
        printf("[%lu]: %f\n", i, prev.value);
        sum += prev.value;
        ++cnt;
    }
    TEMP_EL avg_temp = {0};
    avg_temp.value = sum / (double)cnt;

    unsigned long index = dht_publish(AVG_TEMP_TOPIC, &avg_temp, sizeof(TEMP_EL));
    if (WooFInvalid(index)) {
        fprintf(stderr, "failed to publish to topic %s\n", AVG_TEMP_TOPIC);
        exit(1);
    }

    printf("average temperature %f published to %s\n", avg_temp.value, AVG_TEMP_TOPIC);
    return 1;
}