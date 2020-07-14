#include "dht.h"
#include "dht_client.h"
#include "dht_utils.h"
#include "woofc-host.h"
#include "woofc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ARGS "t:i:"
char* Usage = "client_find -t topic -i client_ip\n";

int main(int argc, char** argv) {
    char topic[DHT_NAME_LENGTH] = {0};
    char client_ip[DHT_NAME_LENGTH] = {0};

    int c;
    while ((c = getopt(argc, argv, ARGS)) != EOF) {
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

    if (client_ip[0] != 0) {
        dht_set_client_ip(client_ip);
    }

    char result_replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH];
    int result_leader;
    int hops;
    unsigned long begin = get_milliseconds();
    if (dht_find_node(topic, result_replicas, &result_leader, &hops) < 0) {
        fprintf(stderr, "failed to find the topic\n");
        exit(1);
    }
    printf("latency: %lu ms\n", get_milliseconds() - begin);
    printf("hops: %d\n", hops);
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
