#include "bldg_control.h"
#include "dht_client.h"
#include "woofc-host.h"
#include "woofc.h"

#include <stdio.h>
#include <stdlib.h>

char topics[][DHT_NAME_LENGTH] = {TOPIC_BUILDING_TOTAL, TOPIC_1F_TOTAL, TOPIC_2F_TOTAL, TOPIC_HOURLY_CHANGE};

int main(int argc, char** argv) {
    WooFInit();

    int i;
    for (i = 0; i < sizeof(topics) / DHT_NAME_LENGTH; ++i) {
        unsigned long seq = dht_topic_latest_seqno(topics[i], 5000);
        if (seq <= 1 || WooFInvalid(seq)) {
            printf("%s: \n", topics[i]);
            continue;
        }
        COUNT_EL count = {0};
        if (dht_topic_get(topics[i], &count, sizeof(COUNT_EL), seq, 5000) < 0) {
            printf("%s: \n", topics[i]);
            continue;
        }
        printf("%s: %d\n", topics[i], count.count);
    }
    printf("\n");

    return 0;
}
