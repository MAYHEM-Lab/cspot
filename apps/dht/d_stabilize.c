#include "dht.h"
#include "dht_utils.h"
#include "monitor.h"
#include "raft_client.h"
#include "woofc.h"

#include <stdio.h>
#include <string.h>

int d_stabilize(WOOF* wf, unsigned long seq_no, void* ptr) {
    log_set_tag("d_stabilize");
    log_set_level(DHT_LOG_INFO);
    // log_set_level(DHT_LOG_DEBUG);
    log_set_output(stdout);
    monitor_init();
    WooFMsgCacheInit();

    DHT_NODE_INFO node = {0};
    if (get_latest_node_info(&node) < 0) {
        log_error("couldn't get latest node info: %s", dht_error_msg);
        monitor_exit(ptr);
        WooFMsgCacheShutdown();
        exit(1);
    }
    DHT_PREDECESSOR_INFO predecessor = {0};
    if (get_latest_predecessor_info(&predecessor) < 0) {
        log_error("couldn't get latest predecessor info: %s", dht_error_msg);
        monitor_exit(ptr);
        WooFMsgCacheShutdown();
        exit(1);
    }
    DHT_SUCCESSOR_INFO successor = {0};
    if (get_latest_successor_info(&successor) < 0) {
        log_error("couldn't get latest successor info: %s", dht_error_msg);
        monitor_exit(ptr);
        WooFMsgCacheShutdown();
        exit(1);
    }

    if (memcmp(successor.hash[0], node.hash, SHA_DIGEST_LENGTH) == 0) {
        // successor == predecessor;
        if (!is_empty(predecessor.hash) && (memcmp(successor.hash[0], predecessor.hash, SHA_DIGEST_LENGTH) != 0)) {
            // predecessor's replicas should be in the hashmap already
            memcpy(successor.hash[0], predecessor.hash, sizeof(successor.hash[0]));
            memcpy(successor.replicas[0], predecessor.replicas, sizeof(successor.replicas[0]));
            successor.leader[0] = predecessor.leader;
            unsigned long index = raft_put_handler(
                node.replicas[node.leader_id], "r_set_successor", &successor, sizeof(DHT_SUCCESSOR_INFO), NULL);
            if (raft_is_error(index)) {
                log_error("failed to invoke r_set_successor using raft: %s", raft_error_msg);
                monitor_exit(ptr);
                WooFMsgCacheShutdown();
                exit(1);
            }
            log_info("updated successor to predecessor because the current successor is itself");
        }

        // successor.notify(n);
        DHT_NOTIFY_ARG notify_arg = {0};
        memcpy(notify_arg.node_hash, node.hash, sizeof(notify_arg.node_hash));
        memcpy(notify_arg.node_replicas, node.replicas, sizeof(notify_arg.node_replicas));
        notify_arg.node_leader = node.replica_id;
        unsigned long seq = monitor_put(DHT_MONITOR_NAME, DHT_NOTIFY_WOOF, "h_notify", &notify_arg, 1);
        if (WooFInvalid(seq)) {
            log_error("failed to call notify on self %s", node.addr);
            monitor_exit(ptr);
            WooFMsgCacheShutdown();
            exit(1);
        }
        log_debug("calling notify on self");
    } else if (is_empty(successor.hash[0])) {
        log_info("no current successor");
        // node_hash should be in the hashmap already
        memcpy(successor.hash[0], node.hash, sizeof(successor.hash[0]));
        memcpy(successor.replicas[0], node.replicas, sizeof(successor.replicas[0]));
        successor.leader[0] = node.replica_id;
        unsigned long index = raft_put_handler(
            node.replicas[node.leader_id], "r_set_successor", &successor, sizeof(DHT_SUCCESSOR_INFO), NULL);
        if (raft_is_error(index)) {
            log_error("failed to invoke r_set_successor using raft: %s", raft_error_msg);
            monitor_exit(ptr);
            WooFMsgCacheShutdown();
            exit(1);
        }
        log_info("successor set to self");
    } else {
        // x = successor.predecessor
        DHT_GET_PREDECESSOR_ARG get_predecessor_arg = {0};
        sprintf(get_predecessor_arg.callback_woof, "%s/%s", node.addr, DHT_STABILIZE_CALLBACK_WOOF);
        sprintf(get_predecessor_arg.callback_handler, "h_stabilize_callback");
        log_debug("current successor_addr: %s", successor_addr(&successor, 0));
        char successor_woof_name[DHT_NAME_LENGTH] = {0};
        sprintf(successor_woof_name, "%s/%s", successor_addr(&successor, 0), DHT_GET_PREDECESSOR_WOOF);
        unsigned long seq = WooFPut(successor_woof_name, "h_get_predecessor", &get_predecessor_arg);
        if (WooFInvalid(seq)) {
            log_warn("current successor %s is not responding, calling r_tey_replicas to try other replicas",
                     successor_woof_name);
            DHT_TRY_REPLICAS_ARG try_replicas_arg;
            seq = monitor_put(DHT_MONITOR_NAME, DHT_TRY_REPLICAS_WOOF, "r_try_replicas", &try_replicas_arg, 1);
            if (WooFInvalid(seq)) {
                log_error("failed to invoke r_try_replicas");
                monitor_exit(ptr);
                WooFMsgCacheShutdown();
                exit(1);
            }
            monitor_exit(ptr);
            monitor_join();
            WooFMsgCacheShutdown();
            return 1;
        }
        log_debug("asked to get_predecessor from %s", successor_woof_name);
        monitor_exit(ptr);
        monitor_join();
        WooFMsgCacheShutdown();
        return 1;
    }

    monitor_exit(ptr);
    monitor_join();
    WooFMsgCacheShutdown();
    return 1;
}
