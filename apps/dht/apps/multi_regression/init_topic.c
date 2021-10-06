#include "dht_client.h"
#include "multi_regression.h"
#include "woofc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int is_cpu_topic(char* topic) {
    return (strcmp(topic, TOPIC_CPU_1) == 0 || strcmp(topic, TOPIC_CPU_2) == 0 || strcmp(topic, TOPIC_CPU_3) == 0 ||
            strcmp(topic, TOPIC_CPU_4) == 0);
}

int main(int argc, char** argv) {
    char topic[DHT_NAME_LENGTH] = {0};

    int c;
    while ((c = getopt(argc, argv, "t:i:")) != EOF) {
        switch (c) {
        case 't': {
            strncpy(topic, optarg, sizeof(topic));
            break;
        }
        default: {
            fprintf(stderr, "./init_topic -t topic\n");
            exit(1);
        }
        }
    }

    if (topic[0] == 0) {
        fprintf(stderr, "./init_topic -t topic\n");
        exit(1);
    }
    WooFInit();

    printf("creating topic %s\n", topic);
    if (strcmp(topic, TOPIC_REGRESSOR_MODEL) == 0) {
        if (dht_create_topic(topic, sizeof(REGRESSOR_MODEL), MULTI_REGRESSION_HISTORY_LENGTH) < 0) {
            fprintf(stderr, "failed to create topic %s: %s\n", topic, dht_error_msg);
            exit(1);
        }
    } else {
        if (dht_create_topic(topic, sizeof(DATA_ELEMENT), MULTI_REGRESSION_HISTORY_LENGTH) < 0) {
            fprintf(stderr, "failed to create topic %s: %s\n", topic, dht_error_msg);
            exit(1);
        }
        if (is_cpu_topic(topic) || strcmp(topic, TOPIC_DHT_1) == 0) {
            char smooth_topic[DHT_NAME_LENGTH] = {0};
            sprintf(smooth_topic, "%s%s", topic, TOPIC_SMOOTH_SUFFIX);
            printf("creating topic %s\n", smooth_topic);
            if (dht_create_topic(smooth_topic, sizeof(DATA_ELEMENT), MULTI_REGRESSION_HISTORY_LENGTH) < 0) {
                fprintf(stderr, "failed to create topic %s: %s\n", smooth_topic, dht_error_msg);
                exit(1);
            }
        }
    }

    printf("registering topic %s\n", topic);
    if (dht_register_topic(topic) < 0) {
        fprintf(stderr, "failed to register topic %s: %s\n", topic, dht_error_msg);
        exit(1);
    }
    if (is_cpu_topic(topic) || strcmp(topic, TOPIC_DHT_1) == 0) {
        char smooth_topic[DHT_NAME_LENGTH] = {0};
        sprintf(smooth_topic, "%s%s", topic, TOPIC_SMOOTH_SUFFIX);
        printf("registering topic %s\n", smooth_topic);
        if (dht_register_topic(smooth_topic) < 0) {
            fprintf(stderr, "failed to register topic %s: %s\n", smooth_topic, dht_error_msg);
            exit(1);
        }
    }

    if (is_cpu_topic(topic) || strcmp(topic, TOPIC_DHT_1) == 0) {
        sleep(2);
        printf("subscribing topic %s\n", topic);
        if (dht_subscribe(topic, HANDLER_SMOOTH) < 0) {
            fprintf(stderr, "failed to subscribe to topic: %s\n", dht_error_msg);
            exit(1);
        }
        char smooth_topic[DHT_NAME_LENGTH] = {0};
        sprintf(smooth_topic, "%s%s", topic, TOPIC_SMOOTH_SUFFIX);
        sleep(2);
        printf("subscribing topic %s\n", smooth_topic);
        if (is_cpu_topic(topic)) {
            if (dht_subscribe(smooth_topic, HANDLER_PREDICT) < 0) {
                fprintf(stderr, "failed to subscribe to topic: %s\n", dht_error_msg);
                exit(1);
            }
        } else if (strcmp(topic, TOPIC_DHT_1) == 0) {
            if (dht_subscribe(smooth_topic, HANDLER_TRAIN) < 0) {
                fprintf(stderr, "failed to subscribe to topic: %s\n", dht_error_msg);
                exit(1);
            }
        }
    }

    return 0;
}
