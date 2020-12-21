#include "dht.h"
#include "dht_utils.h"
#include "monitor.h"
#include "woofc-access.h"
#include "woofc.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int retry_replica_delay = 1;

int r_try_replicas(WOOF* wf, unsigned long seq_no, void* ptr) {
    log_set_tag("r_try_replicas");
    log_set_level(DHT_LOG_INFO);
    log_set_level(DHT_LOG_DEBUG);
    log_set_output(stdout);
    WooFMsgCacheInit();

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
    if (is_empty(successor.hash[0])) {
        log_debug("successor is nil");
        monitor_exit(ptr);
        WooFMsgCacheShutdown();
        return 1;
    }

    // BLOCKED_NODES blocked_nodes = {0};
    // if (get_latest_element(BLOCKED_NODES_WOOF, &blocked_nodes) < 0) {
    //     log_error("failed to get blocked nodes");
    // }

    uint64_t begin = get_milliseconds();
    int i = successor.leader[0];
    while (get_milliseconds() - begin < TRY_SUCCESSOR_REPLICAS_TIMEOUT) {
        i = (i + 1) % DHT_REPLICA_NUMBER;
        // int j;
        // for (j = 1; j <= DHT_REPLICA_NUMBER; ++j) {
        //     i = (successor.leader[0] + j) % DHT_REPLICA_NUMBER;
        successor.leader[0] = i;
        if (successor_addr(&successor, 0)[0] == 0) {
            continue;
        }
        log_warn("trying successor replica[%d]: %s", i, successor_addr(&successor, 0));
        // check if the replica is leader
        char replica_woof[DHT_NAME_LENGTH] = {0};
        sprintf(replica_woof, "%s/%s", successor_addr(&successor, 0), DHT_NODE_INFO_WOOF);
        unsigned long latest_node = WooFGetLatestSeqno(replica_woof);
        if (WooFInvalid(latest_node)) {
            log_warn("failed to get the latest seq_no of %s", replica_woof);
            sleep(retry_replica_delay);
            continue;
        }
        DHT_NODE_INFO replica_node = {0};
        if (WooFGet(replica_woof, &replica_node, latest_node) < 0) {
            log_warn("failed to get the latest node info from %s", replica_woof);
            sleep(retry_replica_delay);
            continue;
        }
        log_warn("got leader_id %d from %s", replica_node.leader_id, replica_woof);
        if (replica_node.leader_id == -1) {
            sleep(retry_replica_delay);
            continue;
        }
        successor.leader[0] = replica_node.leader_id;
        unsigned long seq = WooFPut(DHT_SUCCESSOR_INFO_WOOF, NULL, &successor);
        if (WooFInvalid(seq)) {
            log_error("failed to update successor to use replica[%d]: %s",
                      replica_node.leader_id,
                      successor_addr(&successor, 0));
            monitor_exit(ptr);
            WooFMsgCacheShutdown();
            exit(1);
        }
        log_warn("updated successor to use replica[%d]: %s", replica_node.leader_id, successor_addr(&successor, 0));
#ifdef USE_RAFT
        unsigned long index = raft_put_handler(
            node.replicas[node.leader_id], "r_set_successor", &successor, sizeof(DHT_SUCCESSOR_INFO), 0, NULL);
        if (raft_is_error(index)) {
            log_error("failed to invoke r_set_successor using raft: %s", raft_error_msg);
            monitor_exit(ptr);
            WooFMsgCacheShutdown();
            exit(1);
        }
#endif
        monitor_exit(ptr);
        WooFMsgCacheShutdown();
        return 1;
    }
    log_warn("none of successor replicas is responding, shifting successors");

    for (i = 0; i < DHT_SUCCESSOR_LIST_R - 1; ++i) {
        memcpy(successor.hash[i], successor.hash[i + 1], sizeof(successor.hash[i]));
        memcpy(successor.replicas[i], successor.replicas[i + 1], sizeof(successor.replicas[i]));
        successor.leader[i] = successor.leader[i + 1];
    }
    memset(successor.hash[DHT_SUCCESSOR_LIST_R - 1], 0, sizeof(successor.hash[DHT_SUCCESSOR_LIST_R - 1]));
    memset(successor.replicas[DHT_SUCCESSOR_LIST_R - 1], 0, sizeof(successor.replicas[DHT_SUCCESSOR_LIST_R - 1]));
    successor.leader[DHT_SUCCESSOR_LIST_R - 1] = 0;

    unsigned long seq = WooFPut(DHT_SUCCESSOR_INFO_WOOF, NULL, &successor);
    if (WooFInvalid(seq)) {
        log_error("failed to shift successor list");
        monitor_exit(ptr);
        WooFMsgCacheShutdown();
        exit(1);
    }
    log_warn("shifted new successor: %s", successor.replicas[0][successor.leader[0]]);
#ifdef USE_RAFT
    unsigned long index = raft_put_handler(
        node.replicas[node.leader_id], "r_set_successor", &successor, sizeof(DHT_SUCCESSOR_INFO), 0, NULL);
    if (raft_is_error(index)) {
        log_error("failed to invoke r_set_successor using raft: %s", raft_error_msg);
        monitor_exit(ptr);
        WooFMsgCacheShutdown();
        exit(1);
    }
#endif

    monitor_exit(ptr);
    WooFMsgCacheShutdown();
    return 1;
}
