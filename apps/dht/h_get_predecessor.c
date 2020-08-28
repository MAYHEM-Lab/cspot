#include "dht.h"
#include "dht_utils.h"
#include "monitor.h"
#include "woofc-access.h"
#include "woofc.h"

#include <stdlib.h>
#include <string.h>

int h_get_predecessor(WOOF* wf, unsigned long seq_no, void* ptr) {
    DHT_GET_PREDECESSOR_ARG* arg = (DHT_GET_PREDECESSOR_ARG*)ptr;

    log_set_tag("get_predecessor");
    log_set_level(DHT_LOG_INFO);
    // log_set_level(DHT_LOG_DEBUG);
    log_set_output(stdout);

    DHT_NODE_INFO node = {0};
    if (get_latest_node_info(&node) < 0) {
        log_error("couldn't get latest node info: %s", dht_error_msg);
        exit(1);
    }

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
    result.successor_leader_id = node.leader_id;

    char callback_ipaddr[DHT_NAME_LENGTH] = {0};
    if (WooFIPAddrFromURI(arg->callback_woof, callback_ipaddr, DHT_NAME_LENGTH) < 0) {
        log_error("failed to extract woof ip address from callback woof %s", arg->callback_woof);
        exit(1);
    }
    int callback_port = 0;
    WooFPortFromURI(arg->callback_woof, &callback_port);
    char callback_namespace[DHT_NAME_LENGTH] = {0};
    if (WooFNameSpaceFromURI(arg->callback_woof, callback_namespace, DHT_NAME_LENGTH) < 0) {
        log_error("failed to extract woof namespace from callback woof %s", arg->callback_woof);
        exit(1);
    }
    char callback_monitor[DHT_NAME_LENGTH] = {0};
    if (callback_port > 0) {
        sprintf(callback_monitor,
                "woof://%s:%d%s/%s",
                callback_ipaddr,
                callback_port,
                callback_namespace,
                DHT_MONITOR_NAME);
    } else {
        sprintf(callback_monitor, "woof://%s%s/%s", callback_ipaddr, callback_namespace, DHT_MONITOR_NAME);
    }
    unsigned long seq = monitor_remote_put(callback_monitor, arg->callback_woof, arg->callback_handler, &result, 1);
    if (WooFInvalid(seq)) {
        log_error("failed to put get_predecessor result to %s, monitor: %s", arg->callback_woof, callback_monitor);
        exit(1);
    }
    char hash_str[2 * SHA_DIGEST_LENGTH + 1];
    print_node_hash(hash_str, predecessor.hash);
    log_debug("returning predecessor_hash: %s", hash_str);
    log_debug("returning predecessor_addr: %s", result.predecessor_replicas[result.predecessor_leader]);

    return 1;
}
