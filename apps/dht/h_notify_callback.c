#include "dht.h"
#include "dht_utils.h"
#include "monitor.h"
#include "woofc.h"
#ifdef USE_RAFT
#include "raft_client.h"
#endif

#include <stdlib.h>
#include <string.h>

int h_notify_callback(WOOF* wf, unsigned long seq_no, void* ptr) {
    log_set_tag("notify_callback");
    log_set_level(DHT_LOG_INFO);
    // log_set_level(DHT_LOG_DEBUG);
    log_set_output(stdout);

    DHT_NOTIFY_CALLBACK_ARG result = {0};
    if (monitor_cast(ptr, &result, sizeof(DHT_NOTIFY_CALLBACK_ARG)) < 0) {
        log_error("failed to call monitor_cast");
        monitor_exit(ptr);
        exit(1);
    }

    DHT_NODE_INFO node = {0};
    if (get_latest_node_info(&node) < 0) {
        log_error("couldn't get latest node info: %s", dht_error_msg);
        monitor_exit(ptr);
        exit(1);
    }

    DHT_SUCCESSOR_INFO successor = {0};
    if (get_latest_successor_info(&successor) < 0) {
        log_error("couldn't get latest successor info: %s", dht_error_msg);
        monitor_exit(ptr);
        exit(1);
    }

    // set successor list
    int i;
    for (i = 0; i < DHT_REPLICA_NUMBER; ++i) {
        if (is_empty(result.successor_hash[i])) {
            break;
        }
    }
    memcpy(successor.hash, result.successor_hash, sizeof(successor.hash));
    memcpy(successor.replicas, result.successor_replicas, sizeof(successor.replicas));
    memcpy(successor.leader, result.successor_leader, sizeof(successor.leader));
#ifdef USE_RAFT
    unsigned long index = raft_put_handler(
        node.replicas[node.leader_id], "r_set_successor", &successor, sizeof(DHT_SUCCESSOR_INFO), 0, NULL);
    if (raft_is_error(index)) {
        log_error(
            "failed to invoke r_set_successor on %s using raft: %s", node.replicas[node.leader_id], raft_error_msg);
        monitor_exit(ptr);
        exit(1);
    }
#else
    unsigned long seq = WooFPut(DHT_SUCCESSOR_INFO_WOOF, NULL, &successor);
    if (WooFInvalid(seq)) {
        log_error("failed to update successor list");
        monitor_exit(ptr);
        exit(1);
    }
#endif
    log_debug("set successor to %s...", successor.replicas[0][successor.leader[0]]);

    monitor_exit(ptr);
    return 1;
}
