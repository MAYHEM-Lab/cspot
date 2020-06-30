#include "dht.h"
#include "dht_utils.h"
#include "monitor.h"
#include "woofc.h"

#include <stdlib.h>
#include <string.h>

int h_notify(WOOF* wf, unsigned long seq_no, void* ptr) {
    log_set_tag("notify");
    log_set_level(DHT_LOG_INFO);
    // log_set_level(DHT_LOG_DEBUG);
    log_set_output(stdout);

    DHT_NOTIFY_ARG arg = {0};
    if (monitor_cast(ptr, &arg) < 0) {
        log_error("failed to call monitor_cast");
        exit(1);
    }

    log_debug("potential predecessor_addr: %s", arg.node_replicas[arg.node_leader]);
    DHT_NODE_INFO node = {0};
    if (get_latest_node_info(&node) < 0) {
        log_error("couldn't get latest node info: %s", dht_error_msg);
        monitor_exit(ptr);
        exit(1);
    }
    DHT_PREDECESSOR_INFO predecessor = {0};
    if (get_latest_predecessor_info(&predecessor) < 0) {
        log_error("couldn't get latest predecessor info: %s", dht_error_msg);
        monitor_exit(ptr);
        exit(1);
    }
    DHT_SUCCESSOR_INFO successor = {0};
    if (get_latest_successor_info(&successor) < 0) {
        log_error("couldn't get latest successor info: %s", dht_error_msg);
        monitor_exit(ptr);
        exit(1);
    }

    // if (predecessor is nil or n' ∈ (predecessor, n))
    if (is_empty(predecessor.hash) || memcmp(predecessor.hash, node.hash, SHA_DIGEST_LENGTH) == 0 ||
        in_range(arg.node_hash, predecessor.hash, node.hash)) {
        if (memcmp(predecessor.hash, arg.node_hash, SHA_DIGEST_LENGTH) == 0) {
            log_debug("predecessor is the same, no need to update");
            monitor_exit(ptr);
            return 1;
        }
        memcpy(predecessor.hash, arg.node_hash, sizeof(predecessor.hash));
        memcpy(predecessor.replicas, arg.node_replicas, sizeof(predecessor.replicas));
        predecessor.leader = arg.node_leader;
        unsigned long seq = WooFPut(DHT_PREDECESSOR_INFO_WOOF, NULL, &predecessor);
        if (WooFInvalid(seq)) {
            log_error("failed to update predecessor");
            monitor_exit(ptr);
            exit(1);
        }
        char hash_str[2 * SHA_DIGEST_LENGTH + 1];
        print_node_hash(hash_str, predecessor.hash);
        log_info("updated predecessor_hash: %s", hash_str);
        log_info("updated predecessor_addr: %s", predecessor_addr(&predecessor));
    }

    if (arg.callback_woof[0] == 0) {
        monitor_exit(ptr);
        return 1;
    }
    log_debug("callback: %s@%s", arg.callback_handler, arg.callback_woof);

    // call notify_callback, where it updates successor list
    DHT_NOTIFY_CALLBACK_ARG result = {0};
    memcpy(result.successor_hash[0], node.hash, sizeof(result.successor_hash[0]));
    memcpy(result.successor_replicas[0], node.replicas, sizeof(result.successor_replicas[0]));
    result.successor_leader[0] = node.leader_id;
    int i;
    for (i = 0; i < DHT_SUCCESSOR_LIST_R - 1; ++i) {
        if (is_empty(successor.hash[i])) {
            break;
        } else {
            memcpy(result.successor_hash[i + 1], successor.hash[i], sizeof(result.successor_hash[i + 1]));
            memcpy(result.successor_replicas[i + 1], successor.replicas[i], sizeof(result.successor_replicas[i + 1]));
            result.successor_leader[i + 1] = successor.leader[i];
        }
    }

	char callback_ipaddr[DHT_NAME_LENGTH] = {0};
	if (WooFIPAddrFromURI(arg.callback_woof, callback_ipaddr, DHT_NAME_LENGTH) < 0) {
        log_error("failed to extract woof ip address from callback woof %s", arg.callback_woof);
        exit(1);
    }
    char callback_namespace[DHT_NAME_LENGTH] = {0};
    if (WooFNameSpaceFromURI(arg.callback_woof, callback_namespace, DHT_NAME_LENGTH) < 0) {
        log_error("failed to extract woof namespace from callback woof %s", arg.callback_woof);
        exit(1);
    }
    char callback_monitor[DHT_NAME_LENGTH] = {0};
    sprintf(callback_monitor, "woof://%s%s/%s", callback_ipaddr, callback_namespace, DHT_MONITOR_NAME);
	unsigned long seq = monitor_remote_put(callback_monitor, arg.callback_woof, arg.callback_handler, &result, 1);
    if (WooFInvalid(seq)) {
        log_error("failed to put notify result to woof %s", arg.callback_woof);
        monitor_exit(ptr);
        exit(1);
    }

    monitor_exit(ptr);
    return 1;
}
