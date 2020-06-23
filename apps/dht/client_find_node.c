#include "dht.h"
#include "dht_client.h"
#include "woofc-host.h"
#include "woofc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ARGS "t:"
char* Usage = "client_find -t topic\n";

int main(int argc, char** argv) {
    char topic[DHT_NAME_LENGTH];

    int c;
    while ((c = getopt(argc, argv, ARGS)) != EOF) {
        switch (c) {
        case 't': {
            strncpy(topic, optarg, sizeof(topic));
            break;
        }
        default: {
            fprintf(stderr, "unrecognized command %c\n", (char)c);
            fprintf(stderr, "%s", Usage);
            exit(1);
        }
        }
    }

    if (topic[0] == 0) {
        fprintf(stderr, "must specify topic\n");
        fprintf(stderr, "%s", Usage);
        exit(1);
    }

    WooFInit();

    char local_namespace[DHT_NAME_LENGTH];
    if (node_woof_name(local_namespace) < 0) {
        fprintf(stderr, "find: couldn't get local node's woof name\n");
        exit(1);
    }

    char result_replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH];
    int result_leader;
    if (dht_find_node(topic, result_replicas, &result_leader) < 0) {
        fprintf(stderr, "failed to find the topic\n");
        exit(1);
    }
    printf("replicas:\n");
    int i;
    for (i = 0; i < DHT_REPLICA_NUMBER; ++i) {
        if (result_replicas[i][0] == 0) {
            break;
        }
        printf("%s\n", result_replicas[i]);
    }
    printf("leader: %s(%d)\n", result_replicas[result_leader], result_leader);

    return 0;
}
