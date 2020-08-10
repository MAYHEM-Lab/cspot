#include "dht.h"
#include "dht_utils.h"
#include "monitor.h"
#include "woofc.h"

#ifdef USE_RAFT
#include "raft_client.h"
#endif

#include <stdlib.h>
#include <string.h>

int h_join_callback(WOOF* wf, unsigned long seq_no, void* ptr) {
    log_set_tag("join_callback");
    log_set_level(DHT_LOG_INFO);
    // log_set_level(DHT_LOG_DEBUG);
    log_set_output(stdout);

    DHT_JOIN_ARG arg = {0};
    if (monitor_cast(ptr, &arg) < 0) {
        log_error("failed to call monitor_cast");
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

    memcpy(successor.hash[0], arg.node_hash, sizeof(successor.hash[0]));
    memcpy(successor.replicas[0], arg.node_replicas, sizeof(successor.replicas[0]));
    successor.leader[0] = arg.node_leader;
#ifdef USE_RAFT
    unsigned long index = raft_put_handler("r_set_successor", &successor, sizeof(DHT_SUCCESSOR_INFO), 0);
    while (index == RAFT_REDIRECTED) {
        log_debug("r_set_successor redirected to %s", raft_client_leader);
        index = raft_put_handler("r_set_successor", &successor, sizeof(DHT_SUCCESSOR_INFO), 0);
    }
    if (raft_is_error(index)) {
        log_error("failed to invoke r_set_successor using raft: %s", raft_error_msg);
        monitor_exit(ptr);
        exit(1);
    }
#else
    unsigned long seq = WooFPut(DHT_SUCCESSOR_INFO_WOOF, NULL, &successor);
    if (WooFInvalid(seq)) {
        log_error("failed to update DHT table to woof %s", DHT_SUCCESSOR_INFO_WOOF);
        monitor_exit(ptr);
        exit(1);
    }
#endif

    if (dht_start_daemon() < 0) {
        log_error("failed to start daemon");
        monitor_exit(ptr);
        exit(1);
    }

    log_info("joined, successor: %s", arg.node_replicas[arg.node_leader]);
    monitor_exit(ptr);
    return 1;
}
