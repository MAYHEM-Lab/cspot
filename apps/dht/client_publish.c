#include "dht.h"
#include "dht_client.h"
#include "dht_utils.h"
#include "woofc-host.h"
#include "woofc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MESSAGE_SIZE 4096
#define ARGS "s:t:d:o:"
char* Usage = "client_publish -s server_namespace -t topic -d data -o timeout)\n";

int main(int argc, char** argv) {
    int timeout = 0;
    char topic_name[DHT_NAME_LENGTH] = {0};
    char message[MESSAGE_SIZE] = {0};
    char server_namespace[DHT_NAME_LENGTH] = {0};

    int c;
    while ((c = getopt(argc, argv, ARGS)) != EOF) {
        switch (c) {
        case 's': {
            strncpy(server_namespace, optarg, sizeof(server_namespace));
            break;
        }
        case 't': {
            strncpy(topic_name, optarg, sizeof(topic_name));
            break;
        }
        case 'd': {
            strncpy(message, optarg, sizeof(message));
            break;
        }
        case 'o': {
            timeout = (int)strtoul(optarg, NULL, 0);
            break;
        }
        default: {
            fprintf(stderr, "unrecognized command %c\n", (char)c);
            fprintf(stderr, "%s", Usage);
            exit(1);
        }
        }
    }

    if (topic_name[0] == 0 || message[0] == 0) {
        fprintf(stderr, "%s", Usage);
        exit(1);
    }

    if (server_namespace[0] != 0) {
        if (dht_remote_publish(server_namespace, topic_name, message, MESSAGE_SIZE) < 0) {
            fprintf(stderr, "failed to publish message: %s", dht_error_msg);
            exit(1);
        }
    } else {
        WooFInit();
        if (dht_publish(topic_name, message, MESSAGE_SIZE) < 0) {
            fprintf(stderr, "failed to publish message: %s", dht_error_msg);
            exit(1);
        }
    }

    return 0;
}
