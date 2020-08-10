#include "dht_client.h"
#include "multi_regression.h"
#include "woofc-host.h"
#include "woofc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv) {
    if (argc != 4) {
        fprintf(stderr, "./publish_temp topic temperature timestamp\n");
        fprintf(stderr, "topic list:\n");
        for (int i = 0; i < sizeof(reading_topics) / DHT_NAME_LENGTH; ++i) {
            fprintf(stderr, "\t%s\n", reading_topics[i]);
        }
        exit(1);
    }

    int match = 0;
    for (int i = 0; i < sizeof(reading_topics) / DHT_NAME_LENGTH; ++i) {
        if (strcmp(argv[1], reading_topics[i]) == 0) {
            match = 1;
            break;
        }
    }
    if (!match) {
        fprintf(stderr, "topic %s not supported\n", argv[1]);
        fprintf(stderr, "topic list:\n");
        for (int i = 0; i < sizeof(reading_topics) / DHT_NAME_LENGTH; ++i) {
            fprintf(stderr, "\t%s\n", reading_topics[i]);
        }
        exit(1);
    }

    double temperature = strtod(argv[2], NULL);
    unsigned long timestamp = strtoul(argv[3], NULL, 10);

    printf("publishing to %s at %lu: %f\n", argv[1], timestamp, temperature);
    TEMPERATURE_ELEMENT el = {0};
    el.temp = temperature;
    el.timestamp = timestamp;

    WooFInit();

    unsigned long index = dht_publish(argv[1], &el, sizeof(TEMPERATURE_ELEMENT));
    if (WooFInvalid(index)) {
        fprintf(stderr, "failed to publish to topic %s\n", argv[1]);
        exit(1);
    }
    printf("published to %s\n", argv[1]);

    return 0;
}
