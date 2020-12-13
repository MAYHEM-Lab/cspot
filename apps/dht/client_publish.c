#include "dht.h"
#include "dht_client.h"
#include "dht_utils.h"
#include "woofc-host.h"
#include "woofc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ARGS "s:t:d:o:"
char* Usage = "client_publish -s server_namespace -t topic -d data -o timeout)\n";

int main(int argc, char** argv) {
    int timeout = 0;
    DHT_SERVER_PUBLISH_FIND_ARG arg = {0};
    char server_namespace[DHT_NAME_LENGTH] = {0};

    int c;
    while ((c = getopt(argc, argv, ARGS)) != EOF) {
        switch (c) {
        case 's': {
            strncpy(server_namespace, optarg, sizeof(server_namespace));
            break;
        }
        case 't': {
            strncpy(arg.topic_name, optarg, sizeof(arg.topic_name));
            break;
        }
        case 'd': {
            strncpy(arg.element, optarg, sizeof(arg.element));
            arg.element_size = 4096;
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

    if (arg.topic_name[0] == 0 || arg.element[0] == 0) {
        fprintf(stderr, "%s", Usage);
        exit(1);
    }

    arg.requested_ts = get_milliseconds();
    if (server_namespace[0] != 0) {
        char server_woof[DHT_NAME_LENGTH] = {0};
        sprintf(server_woof, "%s/%s", server_namespace, DHT_SERVER_PUBLISH_FIND_WOOF);
        unsigned long seq = WooFPut(server_woof, NULL, &arg);
        if (WooFInvalid(seq)) {
            fprintf(stderr, "failed put publish request to %s", DHT_SERVER_PUBLISH_FIND_WOOF);
            exit(1);
        }
    } else {
        WooFInit();
        unsigned long seq = WooFPut(DHT_SERVER_PUBLISH_FIND_WOOF, NULL, &arg);
        if (WooFInvalid(seq)) {
            fprintf(stderr, "failed put publish request to %s", DHT_SERVER_PUBLISH_FIND_WOOF);
            exit(1);
        }
    }

    return 0;
}
