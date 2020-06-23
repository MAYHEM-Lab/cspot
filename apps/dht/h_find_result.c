#include "dht.h"
#include "dht_utils.h"
#include "woofc.h"

#include <stdio.h>
#include <string.h>

int h_find_result(WOOF* wf, unsigned long seq_no, void* ptr) {
    DHT_FIND_NODE_RESULT* result = (DHT_FIND_NODE_RESULT*)ptr;

    log_set_tag("find_result");
    // log_set_level(DHT_LOG_DEBUG);
    log_set_level(DHT_LOG_INFO);
    log_set_output(stdout);

    char hash_str[2 * SHA_DIGEST_LENGTH + 1];
    print_node_hash(hash_str, result->node_hash);
    log_info("node_hash: %s, hops: %d", hash_str, result->hops);
    int i;
    for (i = 0; i < DHT_REPLICA_NUMBER; ++i) {
        if (result->node_replicas[i][0] == 0) {
            break;
        }
        log_info("replica: %s", result->node_replicas[i]);
    }
    log_info("leader: %s(%d)", result->node_replicas[result->node_leader], result->node_leader);

    return 1;
}
