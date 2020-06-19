#include "dht.h"
#include "dht_utils.h"
#include "woofc.h"

#include <openssl/sha.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int h_get_predecessor(WOOF* wf, unsigned long seq_no, void* ptr) {
    DHT_GET_PREDECESSOR_ARG* arg = (DHT_GET_PREDECESSOR_ARG*)ptr;

    log_set_tag("get_predecessor");
    log_set_level(DHT_LOG_INFO);
    log_set_level(DHT_LOG_DEBUG);
    log_set_output(stdout);

    DHT_PREDECESSOR_INFO predecessor = {0};
    if (get_latest_predecessor_info(&predecessor) < 0) {
        log_error("couldn't get latest predecessor info: %s", dht_error_msg);
        exit(1);
    }
    log_debug("callback_woof: %s", arg->callback_woof);
    log_debug("callback_handler: %s", arg->callback_handler);

    DHT_STABILIZE_CALLBACK_ARG result = {0};
    if (!is_empty(predecessor.hash)) {
        memcpy(result.predecessor_hash, predecessor.hash, sizeof(result.predecessor_hash));
        memcpy(result.predecessor_replicas, predecessor.replicas, sizeof(result.predecessor_replicas));
        result.predecessor_leader = predecessor.leader;
    }
    unsigned long seq = WooFPut(arg->callback_woof, arg->callback_handler, &result);
    if (WooFInvalid(seq)) {
        log_error("failed to put get_predecessor: result to woof %s", arg->callback_woof);
        exit(1);
    }
    char hash_str[2 * SHA_DIGEST_LENGTH + 1];
    print_node_hash(hash_str, predecessor.hash);
    log_debug("returning predecessor_hash: %s", hash_str);
    log_debug("returning predecessor_addr: %s", result.predecessor_replicas[result.predecessor_leader]);

    return 1;
}
