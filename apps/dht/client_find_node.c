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

#define ARGS "t:o:"
char* Usage = "client_find_node -t topic (-o timeout)\n";

int main(int argc, char** argv) {
    char topic[DHT_NAME_LENGTH] = {0};
    int timeout = 0;

    int c;
    while ((c = getopt(argc, argv, ARGS)) != EOF) {
        switch (c) {
        case 't': {
            strncpy(topic, optarg, sizeof(topic));
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

    DHT_NODE_INFO node_info = {0};
    if (get_latest_node_info(&node_info) < 0) {
        fprintf(stderr, "couldn't get latest node info: %s", dht_error_msg);
        exit(1);
    }

    char hashed_key[SHA_DIGEST_LENGTH];
    dht_hash(hashed_key, topic);
    DHT_FIND_SUCCESSOR_ARG arg = {0};
    dht_init_find_arg(&arg, topic, hashed_key, node_info.addr);
    arg.action = DHT_ACTION_FIND_NODE;

    unsigned long last_checked_result = WooFGetLatestSeqno(DHT_FIND_NODE_RESULT_WOOF);
    unsigned long seq_no = WooFPut(DHT_FIND_SUCCESSOR_WOOF, NULL, &arg);
    if (WooFInvalid(seq_no)) {
        fprintf(stderr, "failed to invoke h_find_successor");
        exit(1);
    }

    char result_node_replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH];
    int result_node_leader;
    int query_count;
    int message_count;
    int failure_count;

    while (1) {
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
                memcpy(result_node_replicas, result.node_replicas, sizeof(result.node_replicas));
                result_node_leader = result.node_leader;
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
        if (result_node_replicas[i][0] == 0) {
            break;
        }
        printf("%s\n", result_node_replicas[i]);
    }
    printf("leader: %d\n", result_node_leader);
    return 0;
}
