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

    int c;
    while ((c = getopt(argc, argv, "t:i:")) != EOF) {
        switch (c) {
        case 't': {
            strncpy(topic, optarg, sizeof(topic));
            break;
        }
        case 'i': {
            strncpy(client_ip, optarg, sizeof(client_ip));
            break;
        }
        default: {
            fprintf(stderr, "./init_topic -t topic -i client_ip(optional)s\n");
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
    for (int i = 0; i < sizeof(other_topics) / DHT_NAME_LENGTH; ++i) {
        if (strcmp(topic, other_topics[i]) == 0) {
            match = 1;
            break;
        }
    }

    if (!match || topic[0] == 0) {
        fprintf(stderr, "./init_topic -t topic -i client_ip(optional)\n");
        fprintf(stderr, "topic list:\n");
        for (int i = 0; i < sizeof(reading_topics) / DHT_NAME_LENGTH; ++i) {
            fprintf(stderr, "\t%s\n", reading_topics[i]);
        }
        for (int i = 0; i < sizeof(other_topics) / DHT_NAME_LENGTH; ++i) {
            fprintf(stderr, "\t%s\n", other_topics[i]);
        }
        exit(1);
    }

    WooFInit();

    if (client_ip[0] != 0) {
        dht_set_client_ip(client_ip);
    }

    if (dht_create_topic(topic, sizeof(TEMPERATURE_ELEMENT), MULTI_REGRESSION_HISTORY_LENGTH) < 0) {
        fprintf(stderr, "failed to create topic %s: %s\n", topic, dht_error_msg);
        exit(1);
    }

    if (dht_register_topic(topic) < 0) {
        fprintf(stderr, "failed to register topic %s: %s\n", topic, dht_error_msg);
        exit(1);
    }

    if (strcmp(topic, TOPIC_PIZERO02_PREDICT) == 0) {
        sleep(2);
        if (dht_subscribe(TOPIC_PIZERO02_CPU, HANDLER_REGRESS) < 0) {
            fprintf(stderr, "failed to subscribe to topic: %s\n", dht_error_msg);
            exit(1);
        }
    }
    if (strcmp(topic, TOPIC_PIZERO02_CPU) == 0) {
        sleep(2);
        if (dht_subscribe(TOPIC_PIZERO02_CPU, HANDLER_TRAIN) < 0) {
            fprintf(stderr, "failed to subscribe to topic: %s\n", dht_error_msg);
            exit(1);
        }
    }

    return 0;
}
