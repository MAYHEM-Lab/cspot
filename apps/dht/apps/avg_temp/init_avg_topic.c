#include "avg_temp.h"
#include "dht_client.h"
#include "woofc-host.h"
#include "woofc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char** argv) {
    WooFInit();

    if (dht_create_topic(AVG_TEMP_TOPIC, sizeof(TEMP_EL), AVG_TEMP_HISTORY_LENGTH) < 0) {
        fprintf(stderr, "failed to create topic woof: %s\n", dht_error_msg);
        exit(1);
    }

    if (dht_register_topic(AVG_TEMP_TOPIC) < 0) {
        fprintf(stderr, "failed to register topic on DHT: %s\n", dht_error_msg);
        exit(1);
    }

    sleep(2);
    if (dht_subscribe(ROOM_TEMP_TOPIC, AVG_TEMP_HANDLER) < 0) {
        fprintf(stderr, "failed to subscribe to topic: %s\n", dht_error_msg);
        exit(1);
    }

    return 0;
}
