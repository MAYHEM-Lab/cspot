#include "dht.h"
#include "dht_client.h"
#include "dht_utils.h"
#include "woofc-host.h"
#include "woofc.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ARGS "t:i:o:"
char* Usage = "client_find_topic -t topic -i client_ip (-o timeout)\n";

int main(int argc, char** argv) {
    char topic[DHT_NAME_LENGTH] = {0};
    char client_ip[DHT_NAME_LENGTH] = {0};
    int timeout = 0;

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

    if (topic[0] == 0) {
        fprintf(stderr, "must specify topic\n");
        fprintf(stderr, "%s", Usage);
        exit(1);
    }

    WooFInit();

    if (client_ip[0] != 0) {
        dht_set_client_ip(client_ip);
    }

    uint64_t begin = get_milliseconds();
    char result_replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH];
    int result_leader;
    int query_count;
    int message_count;
    int failure_count;
    int blocked_count;
    int self_forward_count;
    int delayed_time;
    if (dht_find_node_debug(topic,
                            result_replicas,
                            &result_leader,
                            &query_count,
                            &message_count,
                            &failure_count,
                            &blocked_count,
                            &self_forward_count,
                            &delayed_time,
                            timeout) < 0) {
        fprintf(stderr, "failed to find the topic: %s\n", dht_error_msg);
        exit(1);
    }
    printf("found_node_latency: %" PRIu64 " ms\n", get_milliseconds() - begin);
    printf("query_count: %d\n", query_count);
    printf("message_count: %d\n", message_count);
    printf("failure_count: %d\n", failure_count);
    printf("blocked_count: %d\n", blocked_count);
    printf("self_forward_count: %d\n", self_forward_count);
    printf("delayed_time: %d\n", delayed_time);
    printf("replicas:\n");
    int i;
    for (i = 0; i < DHT_REPLICA_NUMBER; ++i) {
        if (result_replicas[i][0] == 0) {
            break;
        }
        printf("%s\n", result_replicas[i]);
    }
    printf("leader: %s(%d)\n", result_replicas[result_leader], result_leader);
    
    DHT_TOPIC_REGISTRY topic_entry = {0};
    char* node_addr;
    for (i = 0; i < DHT_REPLICA_NUMBER; ++i) {
        node_addr = result_replicas[(result_leader + i) % DHT_REPLICA_NUMBER];
        // get the topic namespace
        char registration_woof[DHT_NAME_LENGTH] = {0};
        sprintf(registration_woof, "%s/%s_%s", node_addr, topic, DHT_TOPIC_REGISTRATION_WOOF);
        if (get_latest_element(registration_woof, &topic_entry) < 0) {
            sprintf(
                dht_error_msg, "failed to get topic registration info from %s: %s", registration_woof, dht_error_msg);
        }
        break;
    }
    printf("found_replica: %s\n", topic_entry.topic_replicas[0]);
    printf("found_replica_latency: %" PRIu64 " ms\n", get_milliseconds() - begin);
    return 0;
}
