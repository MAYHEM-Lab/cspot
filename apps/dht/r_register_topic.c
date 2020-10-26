#include "dht.h"
#include "dht_utils.h"
#include "monitor.h"
#include "woofc.h"

#include <stdlib.h>
#include <string.h>

int r_register_topic(WOOF* wf, unsigned long seq_no, void* ptr) {
    log_set_tag("r_register_topic");
    log_set_level(DHT_LOG_INFO);
    log_set_level(DHT_LOG_DEBUG);
    log_set_output(stdout);

    DHT_REGISTER_TOPIC_ARG arg = {0};
    if (monitor_cast(ptr, &arg, sizeof(DHT_REGISTER_TOPIC_ARG)) < 0) {
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
    BLOCKED_NODES blocked_nodes = {0};
    if (get_latest_element(BLOCKED_NODES_WOOF, &blocked_nodes) < 0) {
        log_error("failed to get blocked nodes");
    }

    DHT_SUCCESSOR_INFO successor = {0};
    if (get_latest_successor_info(&successor) < 0) {
        log_error("couldn't get latest successor info: %s", dht_error_msg);
        exit(1);
    }

    log_debug("calling h_register_topic for topic %s on self: %s", arg.topic_name, node.addr);
    unsigned long index = checked_raft_sessionless_put_handler(&blocked_nodes,
                                                               node.addr,
                                                               node.addr,
                                                               "h_register_topic",
                                                               &arg,
                                                               sizeof(DHT_REGISTER_TOPIC_ARG),
                                                               1,
                                                               DHT_RAFT_TIMEOUT);
    if (raft_is_error(index)) {
        log_error("failed to invoke h_register_topic on %s: %s", node.addr, raft_error_msg);
        monitor_exit(ptr);
        exit(1);
    }

    int i;
    for (i = 0; i < DHT_REGISTER_TOPIC_REPLICA - 1; ++i) {
        char* successor_leader = successor_addr(&successor, i);
        if (successor_leader[0] == 0) {
            break;
        }
        log_debug("replicating h_register_topic for topic %s on successor[%d]: %s", arg.topic_name, i, successor_leader);
        unsigned long index = checked_raft_sessionless_put_handler(&blocked_nodes,
                                                                   node.addr,
                                                                   successor_leader,
                                                                   "h_register_topic",
                                                                   &arg,
                                                                   sizeof(DHT_REGISTER_TOPIC_ARG),
                                                                   1,
                                                                   DHT_RAFT_TIMEOUT);
        if (raft_is_error(index)) {
            log_error("failed to invoke h_register_topic on %s: %s", successor_leader, raft_error_msg);
            continue;
        }
    }

    monitor_exit(ptr);
    return 1;
}
