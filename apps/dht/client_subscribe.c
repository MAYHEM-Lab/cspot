#include "dht.h"
#include "dht_client.h"
#include "woofc-host.h"
#include "woofc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ARGS "t:h:"
char* Usage = "client_subscribe -t topic -h handler\n";

int main(int argc, char** argv) {
    char topic[DHT_NAME_LENGTH] = {0};
    char handler[DHT_NAME_LENGTH] = {0};

    int c;
    while ((c = getopt(argc, argv, ARGS)) != EOF) {
        switch (c) {
        case 't': {
            strncpy(topic, optarg, sizeof(topic));
            break;
        }
        case 'h': {
            strncpy(handler, optarg, sizeof(handler));
            break;
        }
        default: {
            fprintf(stderr, "unrecognized command %c\n", (char)c);
            fprintf(stderr, "%s", Usage);
            exit(1);
        }
        }
    }

    if (topic[0] == 0 || handler[0] == 0) {
        fprintf(stderr, "%s", Usage);
        exit(1);
    }

    WooFInit();

    if (dht_subscribe(topic, handler) < 0) {
        fprintf(stderr, "failed to subscribe to topic: %s\n", dht_error_msg);
        exit(1);
    }

    return 0;
}
