#include "dht.h"
#include "dht_client.h"
#include "woofc-host.h"
#include "woofc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ARGS "t:d:"
char* Usage = "client_publish -t topic -d data\n";

int main(int argc, char** argv) {
    char topic[DHT_NAME_LENGTH];
    char data[4096];
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

    unsigned long seq = dht_publish(topic, data);
    if (WooFInvalid(seq)) {
        fprintf(stderr, "failed to publish data: %s\n", dht_error_msg);
        exit(1);
    }

    return 0;
}
