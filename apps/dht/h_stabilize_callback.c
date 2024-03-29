#include "dht.h"
#include "dht_utils.h"
#include "monitor.h"
#include "raft_client.h"
#include "woofc.h"

#include <stdlib.h>
#include <string.h>

int h_stabilize_callback(WOOF* wf, unsigned long seq_no, void* ptr) {
    log_set_tag("h_stabilize_callback");
    log_set_level(DHT_LOG_INFO);
    // log_set_level(DHT_LOG_DEBUG);
    log_set_output(stdout);
    monitor_init();

    DHT_STABILIZE_CALLBACK_ARG result = {0};
    if (monitor_cast(ptr, &result, sizeof(DHT_STABILIZE_CALLBACK_ARG)) < 0) {
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

    log_debug("get_predecessor addr: %s", result.predecessor_replicas[result.predecessor_leader]);
    // x = successor.predecessor
    // if (x ∈ (n, successor))
    if (!is_empty(result.predecessor_hash) && in_range(result.predecessor_hash, node.hash, successor.hash[0])) {
        // successor = x;
        if (memcmp(successor.hash[0], result.predecessor_hash, SHA_DIGEST_LENGTH) != 0) {
            memcpy(successor.hash[0], result.predecessor_hash, sizeof(successor.hash[0]));
            memcpy(successor.replicas[0], result.predecessor_replicas, sizeof(successor.replicas[0]));
            successor.leader[0] = result.predecessor_leader;
            unsigned long index = raft_put_handler(
                node.replicas[node.leader_id], "r_set_successor", &successor, sizeof(DHT_SUCCESSOR_INFO), NULL);
            if (raft_is_error(index)) {
                log_error("failed to invoke r_set_successor using raft: %s", raft_error_msg);
                monitor_exit(ptr);
                        exit(1);
            }
            log_info("updated successor to %s", result.predecessor_replicas[result.predecessor_leader]);
        }
    } else {
        log_debug("get successor_leader_id %d", result.successor_leader_id);
        if (successor.leader[0] != result.successor_leader_id) {
            successor.leader[0] = result.successor_leader_id;
            unsigned long index = raft_put_handler(
                node.replicas[node.leader_id], "r_set_successor", &successor, sizeof(DHT_SUCCESSOR_INFO), NULL);
            if (raft_is_error(index)) {
                log_error("failed to invoke r_set_successor using raft: %s", raft_error_msg);
                monitor_exit(ptr);
                        exit(1);
            }
        }
    }

    char* successor_leader = successor_addr(&successor, 0);

    // successor.notify(n);
    char notify_monitor[DHT_NAME_LENGTH] = {0};
    sprintf(notify_monitor, "%s/%s", successor_leader, DHT_MONITOR_NAME);
    char notify_woof_name[DHT_NAME_LENGTH] = {0};
    sprintf(notify_woof_name, "%s/%s", successor_leader, DHT_NOTIFY_WOOF);
    DHT_NOTIFY_ARG notify_arg = {0};
    memcpy(notify_arg.node_hash, node.hash, sizeof(notify_arg.node_hash));
    memcpy(notify_arg.node_replicas, node.replicas, sizeof(notify_arg.node_replicas));
    notify_arg.node_leader = node.replica_id;
    sprintf(notify_arg.callback_woof, "%s/%s", node.addr, DHT_NOTIFY_CALLBACK_WOOF);
    sprintf(notify_arg.callback_handler, "h_notify_callback");
    unsigned long seq = monitor_remote_put(notify_monitor, notify_woof_name, "h_notify", &notify_arg, 1);
    if (WooFInvalid(seq)) {
        log_warn("failed to call notify on successor %s", notify_woof_name);
        DHT_TRY_REPLICAS_ARG try_replicas_arg;
        seq = monitor_put(DHT_MONITOR_NAME, DHT_TRY_REPLICAS_WOOF, "r_try_replicas", &try_replicas_arg, 1);
        if (WooFInvalid(seq)) {
            log_error("failed to invoke r_try_replicas");
            monitor_exit(ptr);
                exit(1);
        }
        monitor_exit(ptr);
        monitor_join();
        return 1;
    }
    log_debug("called notify on successor %s", successor_leader);

    monitor_exit(ptr);
    monitor_join();
    return 1;
}
