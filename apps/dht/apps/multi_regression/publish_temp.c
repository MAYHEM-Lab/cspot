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
    char client_ip[DHT_NAME_LENGTH] = {0};
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

    int match = 0;
    for (int i = 0; i < sizeof(reading_topics) / DHT_NAME_LENGTH; ++i) {
        if (strcmp(topic, reading_topics[i]) == 0) {
            match = 1;
            break;
        }
    }
    if (!match) {
        fprintf(stderr, "./publish_temp -t topic -v temperature -s timestamp\n");
        fprintf(stderr, "topic %s not supported\n", topic);
        fprintf(stderr, "topic list:\n");
        for (int i = 0; i < sizeof(reading_topics) / DHT_NAME_LENGTH; ++i) {
            fprintf(stderr, "\t%s\n", reading_topics[i]);
        }
        exit(1);
    }

    printf("publishing to %s at %lu: %f\n", topic, timestamp, temperature);
    PUBLISH_ARG arg = {0};
    strcpy(arg.topic, topic);
    arg.val.temp = temperature;
    arg.val.timestamp = timestamp;

    WooFInit();
    unsigned long seq = WooFPut(CLIENT_WOOF_NAME, "h_publish", &arg);
    if (WooFInvalid(seq)) {
        fprintf(stderr, "failed to publish data\n");
        exit(1);
    }

    return 0;
}
