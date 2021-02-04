#include "bldg_control.h"
#include "dht_client.h"
#include "woofc-host.h"
#include "woofc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char** argv) {
    WooFInit();

    if (dht_create_topic(TOPIC_BUILDING_TOTAL, sizeof(COUNT_EL), BLDG_HISTORY_LENGTH) < 0) {
        fprintf(stderr, "failed to create topic woof: %s\n", dht_error_msg);
        exit(1);
    }

    if (dht_register_topic(TOPIC_BUILDING_TOTAL) < 0) {
        fprintf(stderr, "failed to register topic on DHT: %s\n", dht_error_msg);
        exit(1);
    }

    sleep(2);

    COUNT_EL count = {0};
    count.count = 0;
    unsigned long seq = dht_publish(TOPIC_BUILDING_TOTAL, &count, sizeof(COUNT_EL));
    if (WooFInvalid(seq)) {
        fprintf(stderr, "failed to initialize %s\n", TOPIC_BUILDING_TOTAL);
    }

    if (dht_subscribe(TOPIC_BUILDING_GATE_TRAFFIC, "h_bldg_total") < 0) {
        fprintf(stderr, "failed to subscribe to topic: %s\n", dht_error_msg);
        exit(1);
    }

    return 0;
}
