#include "dht_client.h"
#include "multi_regression.h"
#include "woofc-host.h"
#include "woofc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char** argv) {
    char topic[DHT_NAME_LENGTH] = {0};
    char client_ip[DHT_NAME_LENGTH] = {0};
    double temperature = 0;
    unsigned long timestamp = 0;
    int timeout = 0;

    int c;
    while ((c = getopt(argc, argv, "t:i:v:s:d:")) != EOF) {
        switch (c) {
        case 't': {
            strncpy(topic, optarg, sizeof(topic));
            break;
        }
        case 'i': {
            strncpy(client_ip, optarg, sizeof(client_ip));
            break;
        }
        case 'v': {
            temperature = strtod(optarg, NULL);
            break;
        }
        case 's': {
            timestamp = strtoul(optarg, NULL, 0);
            break;
        }
        case 'd': {
            timeout = (int)strtoul(optarg, NULL, 0);
            break;
        }
        default: {
            fprintf(stderr, "./publish_temp -t topic -v temperature -s timestamp (-i client_ip -d timeout)\n");
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
        fprintf(stderr, "topic %s not supported\n", topic);
        fprintf(stderr, "topic list:\n");
        for (int i = 0; i < sizeof(reading_topics) / DHT_NAME_LENGTH; ++i) {
            fprintf(stderr, "\t%s\n", reading_topics[i]);
        }
        exit(1);
    }

    printf("publishing to %s at %lu: %f\n", topic, timestamp, temperature);
    TEMPERATURE_ELEMENT el = {0};
    el.temp = temperature;
    el.timestamp = timestamp;

    WooFInit();

    if (client_ip[0] != 0) {
        dht_set_client_ip(client_ip);
    }

    unsigned long index = dht_publish(topic, &el, sizeof(TEMPERATURE_ELEMENT), timeout);
    if (WooFInvalid(index)) {
        fprintf(stderr, "failed to publish to topic %s: %s\n", topic, dht_error_msg);
        exit(1);
    }
    printf("published to %s\n", topic);

    return 0;
}
