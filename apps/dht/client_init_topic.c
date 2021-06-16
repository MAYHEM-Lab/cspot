#include "dht.h"
#include "dht_client.h"
#include "woofc-host.h"
#include "woofc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ARGS "t:s:n:i:"
char* Usage = "client_init_topic -t topic -s element_size -n history_size (-i client_ip)\n";

int main(int argc, char** argv) {
    char topic[DHT_NAME_LENGTH] = {0};
    unsigned long element_size;
    unsigned long history_size;
    char client_ip[DHT_NAME_LENGTH] = {0};

    int c;
    while ((c = getopt(argc, argv, ARGS)) != EOF) {
        switch (c) {
        case 't': {
            strncpy(topic, optarg, sizeof(topic));
            break;
        }
        case 's': {
            element_size = strtoul(optarg, NULL, 0);
            break;
        }
        case 'n': {
            history_size = strtoul(optarg, NULL, 0);
            break;
        }
        case 'i': {
            strncpy(client_ip, optarg, sizeof(client_ip));
            break;
        }
        default: {
            fprintf(stderr, "unrecognized command %c\n", (char)c);
            fprintf(stderr, "%s", Usage);
            exit(1);
        }
        }
    }

    if (topic[0] == 0 || element_size == 0 || history_size == 0) {
        fprintf(stderr, "%s", Usage);
        exit(1);
    }

    WooFInit();

    if (dht_create_topic(topic, element_size, history_size) < 0) {
        fprintf(stderr, "failed to create topic woof: %s\n", dht_error_msg);
        exit(1);
    }

    if (dht_register_topic(topic) < 0) {
        fprintf(stderr, "failed to register topic on DHT: %s\n", dht_error_msg);
        exit(1);
    }

    return 0;
}
