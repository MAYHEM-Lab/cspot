#include "dht.h"
#include "dht_utils.h"
#include "monitor.h"
#include "raft_client.h"
#include "woofc.h"

#include <stdlib.h>
#include <string.h>

int h_join_callback(WOOF* wf, unsigned long seq_no, void* ptr) {
    log_set_tag("h_join_callback");
    log_set_level(DHT_LOG_INFO);
    // log_set_level(DHT_LOG_DEBUG);
    log_set_output(stdout);
    WooFMsgCacheInit();

    DHT_JOIN_ARG arg = {0};
    if (monitor_cast(ptr, &arg, sizeof(DHT_JOIN_ARG)) < 0) {
        log_error("failed to call monitor_cast");
        monitor_exit(ptr);
        WooFMsgCacheShutdown();
        exit(1);
    }

    DHT_NODE_INFO node = {0};
    if (get_latest_node_info(&node) < 0) {
        log_error("couldn't get latest node info: %s", dht_error_msg);
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

    char hash_str[2 * SHA_DIGEST_LENGTH + 1];
    print_node_hash(hash_str, node.hash);
    log_debug("node_hash: %s", hash_str);
    print_node_hash(hash_str, arg.node_hash);
    log_debug("find_successor result hash: %s", hash_str);
    int i;
    for (i = 0; i < DHT_REPLICA_NUMBER; ++i) {
        if (arg.node_replicas[i][0] == 0) {
            break;
        }
        log_debug("find_successor result replica: %s", arg.node_replicas[i]);
    }
    log_debug("find_successor result leader: %d", arg.node_leader);
    log_debug("find_successor result addr: %s", arg.node_replicas[arg.node_leader]);

    if (memcmp(node.hash, arg.node_hash, sizeof(node.hash)) == 0) {
        // double join
        memcpy(successor.hash[0], arg.replier_hash, sizeof(successor.hash[0]));
        memcpy(successor.replicas[0], arg.replier_replicas, sizeof(successor.replicas[0]));
        successor.leader[0] = arg.replier_leader;
    } else {
        memcpy(successor.hash[0], arg.node_hash, sizeof(successor.hash[0]));
        memcpy(successor.replicas[0], arg.node_replicas, sizeof(successor.replicas[0]));
        successor.leader[0] = arg.node_leader;
    }
    unsigned long index = raft_put_handler(
        node.replicas[node.leader_id], "r_set_successor", &successor, sizeof(DHT_SUCCESSOR_INFO), 0, NULL);
    if (raft_is_error(index)) {
        log_error("failed to invoke r_set_successor using raft: %s", raft_error_msg);
        monitor_exit(ptr);
        WooFMsgCacheShutdown();
        exit(1);
    }
    log_info("set successor to %s", successor.replicas[0][successor.leader[0]]);

    int stabilize_freq;
    int chk_predecessor_freq;
    int fix_finger_freq;
    int update_leader_freq;
    int daemon_wakeup_freq;
    deserialize_dht_config(arg.serialized_config,
                           &stabilize_freq,
                           &chk_predecessor_freq,
                           &fix_finger_freq,
                           &update_leader_freq,
                           &daemon_wakeup_freq);
    log_debug("stabilize_freq: %d", stabilize_freq);
    log_debug("chk_predecessor_freq: %d", chk_predecessor_freq);
    log_debug("fix_finger_freq: %d", fix_finger_freq);
    log_debug("update_leader_freq: %d", update_leader_freq);
    log_debug("daemon_wakeup_freq: %d", daemon_wakeup_freq);
    if (dht_start_daemon(
            stabilize_freq, chk_predecessor_freq, fix_finger_freq, update_leader_freq, daemon_wakeup_freq) < 0) {
        log_error("failed to start daemon");
        monitor_exit(ptr);
        WooFMsgCacheShutdown();
        exit(1);
    }

    log_info("joined, successor: %s", arg.node_replicas[arg.node_leader]);
    monitor_exit(ptr);
    WooFMsgCacheShutdown();
    return 1;
}
