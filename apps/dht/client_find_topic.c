#include "dht.h"
#include "dht_client.h"
#include "dht_utils.h"
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

    uint64_t begin = get_milliseconds();
    char local_namespace[DHT_NAME_LENGTH] = {0};
    node_woof_namespace(local_namespace);
    char callback_namespace[DHT_NAME_LENGTH] = {0};
    if (client_ip[0] == 0) {
        char ip[DHT_NAME_LENGTH] = {0};
        if (WooFLocalIP(ip, sizeof(ip)) < 0) {
            fprintf(stderr, "failed to get local IP");
            exit(1);
        }
        sprintf(callback_namespace, "woof://%s%s", ip, local_namespace);
    } else {
        sprintf(callback_namespace, "woof://%s%s", client_ip, local_namespace);
    }

    char hashed_key[SHA_DIGEST_LENGTH];
    dht_hash(hashed_key, topic);
    DHT_FIND_SUCCESSOR_ARG arg = {0};
    dht_init_find_arg(&arg, topic, hashed_key, callback_namespace);
    arg.action = DHT_ACTION_FIND_NODE;

    unsigned long last_checked_result = WooFGetLatestSeqno(DHT_FIND_NODE_RESULT_WOOF);
    unsigned long seq_no = WooFPut(DHT_FIND_SUCCESSOR_WOOF, NULL, &arg);
    if (WooFInvalid(seq_no)) {
        fprintf(stderr, "failed to invoke h_find_successor");
        exit(1);
    }

    char result_replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH];
    int result_leader = -1;
    int query_count;
    int message_count;
    int failure_count;

    while (result_leader == -1) {
        if (timeout > 0 && get_milliseconds() - begin > timeout) {
            fprintf(stderr, "timeout after %d ms", timeout);
            exit(1);
        }

        unsigned long latest_result = WooFGetLatestSeqno(DHT_FIND_NODE_RESULT_WOOF);
        unsigned long i;
        for (i = last_checked_result + 1; i <= latest_result; ++i) {
            DHT_FIND_NODE_RESULT result = {0};
            if (WooFGet(DHT_FIND_NODE_RESULT_WOOF, &result, i) < 0) {
                fprintf(stderr, "failed to get result at %lu from %s", i, DHT_FIND_NODE_RESULT_WOOF);
                exit(1);
            }
            if (memcmp(result.topic, topic, SHA_DIGEST_LENGTH) == 0) {
                memcpy(result_replicas, result.node_replicas, sizeof(result.node_replicas));
                result_leader = result.node_leader;
                break;
            }
        }
        last_checked_result = latest_result;
        usleep(10 * 1000);
    }

    printf("found_node_latency: %" PRIu64 " ms\n", get_milliseconds() - begin);
    printf("query_count: %d\n", query_count);
    printf("message_count: %d\n", message_count);
    printf("failure_count: %d\n", failure_count);
    printf("replicas:\n");
    int i;
    for (i = 0; i < DHT_REPLICA_NUMBER; ++i) {
        if (result_replicas[i][0] == 0) {
            break;
        }
        printf("%s\n", result_replicas[i]);
    }
    printf("leader: %d\n", result_leader);

    DHT_TOPIC_REGISTRY topic_entry = {0};
    char* node_addr;
    for (i = 0; i < DHT_REPLICA_NUMBER; ++i) {
        node_addr = result_replicas[(result_leader + i) % DHT_REPLICA_NUMBER];
        // get the topic namespace
        char registration_woof[DHT_NAME_LENGTH] = {0};
        sprintf(registration_woof, "%s/%s_%s", node_addr, topic, DHT_TOPIC_REGISTRATION_WOOF);
        if (WooFGet(registration_woof, &topic_entry, 0) < 0) {
            sprintf(
                dht_error_msg, "failed to get topic registration info from %s: %s", registration_woof, dht_error_msg);
            continue;
        }
        break;
    }
    printf("found_replica: %s\n", topic_entry.topic_replicas[topic_entry.last_leader]);
    printf("found_replica_latency: %" PRIu64 " ms\n", get_milliseconds() - begin);
    return 0;
}
