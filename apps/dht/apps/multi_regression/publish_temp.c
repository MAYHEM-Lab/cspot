#include "dht_client.h"
#include "multi_regression.h"
#include "woofc-host.h"
#include "woofc.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char** argv) {
    char topic[DHT_NAME_LENGTH] = {0};
    double temperature = 0;
    uint64_t timestamp = 0;

    int c;
    while ((c = getopt(argc, argv, "t:v:s:")) != EOF) {
        switch (c) {
        case 't': {
            strncpy(topic, optarg, sizeof(topic));
            break;
        }
        case 'v': {
            temperature = strtod(optarg, NULL);
            break;
        }
        case 's': {
            timestamp = (uint64_t)strtoul(optarg, NULL, 0);
            break;
        }
        default: {
            fprintf(stderr, "./publish_temp -t topic -v temperature -s timestamp\n");
            exit(1);
        }
        }
    }

    printf("publishing to %s at %lu: %f\n", topic, timestamp, temperature);
    DATA_ELEMENT data = {0};
    data.val = temperature;
    data.ts = timestamp;

    WooFInit();
    unsigned long index = dht_publish(topic, &data, sizeof(DATA_ELEMENT));
    if (WooFInvalid(index)) {
        fprintf(stderr, "failed to publish to topic %s: %s\n", topic, dht_error_msg);
        exit(1);
    }

    return 0;
}
