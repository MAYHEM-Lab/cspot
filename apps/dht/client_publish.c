#include "dht.h"
#include "dht_client.h"
#include "woofc-host.h"
#include "woofc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ARGS "t:d:i:o:"
char* Usage = "client_publish -t topic -d data -i client_ip (-o timeout)\n";

int main(int argc, char** argv) {
    char topic[DHT_NAME_LENGTH] = {0};
    char data[4096] = {0};
    char client_ip[DHT_NAME_LENGTH] = {0};
    int timeout = 0;

    int c;
    while ((c = getopt(argc, argv, ARGS)) != EOF) {
        switch (c) {
        case 't': {
            strncpy(topic, optarg, sizeof(topic));
            break;
        }
        case 'd': {
            strncpy(data, optarg, sizeof(data));
            break;
        }
        case 'i': {
            strncpy(client_ip, optarg, sizeof(client_ip));
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

    if (topic[0] == 0 || data[0] == 0) {
        fprintf(stderr, "%s", Usage);
        exit(1);
    }

    WooFInit();

    if (client_ip[0] != 0) {
        dht_set_client_ip(client_ip);
    }

    unsigned long seq = dht_publish(topic, data, sizeof(data), timeout);
    if (WooFInvalid(seq)) {
        fprintf(stderr, "failed to publish data: %s\n", dht_error_msg);
        exit(1);
    }

    return 0;
}
