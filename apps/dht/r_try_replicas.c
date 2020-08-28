#include "dht.h"
#include "dht_utils.h"
#include "monitor.h"
#include "woofc-access.h"
#include "woofc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int r_try_replicas(WOOF* wf, unsigned long seq_no, void* ptr) {
    log_set_tag("try_replicas");
    log_set_level(DHT_LOG_INFO);
    log_set_level(DHT_LOG_DEBUG);
    log_set_output(stdout);

    DHT_SUCCESSOR_INFO successor = {0};
    if (get_latest_successor_info(&successor) < 0) {
        log_error("couldn't get latest successor info: %s", dht_error_msg);
        monitor_exit(ptr);
        exit(1);
    }
    if (is_empty(successor.hash[0])) {
        log_debug("successor is nil");
        monitor_exit(ptr);
        return 1;
    }
    uint64_t begin = get_milliseconds();
    int i = successor.leader[0];
    while (get_milliseconds() - begin < TRY_SUCCESSOR_REPLICAS_TIMEOUT) {
        i = (i + 1) % DHT_REPLICA_NUMBER;
        successor.leader[0] = i;
        if (successor_addr(&successor, 0)[0] == 0) {
            break;
        }
        log_warn("trying successor replica[%d]: %s", i, successor_addr(&successor, 0));
        // check if the replica is leader
        char replica_woof[DHT_NAME_LENGTH] = {0};
        sprintf(replica_woof, "%s/%s", successor_addr(&successor, 0), DHT_NODE_INFO_WOOF);
        unsigned long latest_node = WooFGetLatestSeqno(replica_woof);
        if (WooFInvalid(latest_node)) {
            log_warn("failed to get the latest seq_no of %s", replica_woof);
            continue;
        }
        DHT_NODE_INFO node = {0};
        if (WooFGet(replica_woof, &node, latest_node) < 0) {
            log_warn("failed to get the latest node info from %s", replica_woof);
            continue;
        }
        log_warn("got leader_id %d from %s", node.leader_id, replica_woof);
        if (node.leader_id == -1) {
            continue;
        }
        successor.leader[0] = node.leader_id;
        unsigned long seq = WooFPut(DHT_SUCCESSOR_INFO_WOOF, NULL, &successor);
        if (WooFInvalid(seq)) {
            log_error(
                "failed to update successor to use replica[%d]: %s", node.leader_id, successor_addr(&successor, 0));
            monitor_exit(ptr);
            exit(1);
        }
        log_warn("updated successor to use replica[%d]: %s", node.leader_id, successor_addr(&successor, 0));
        monitor_exit(ptr);
        return 1;
    }
    log_warn("none of successor replicas is responding, shifting successors");
    DHT_SHIFT_SUCCESSOR_ARG shift_successor_arg;
    unsigned long seq =
        monitor_put(DHT_MONITOR_NAME, DHT_SHIFT_SUCCESSOR_WOOF, "h_shift_successor", &shift_successor_arg, 0);
    if (WooFInvalid(seq)) {
        log_error("failed to shift successor");
        monitor_exit(ptr);
        exit(1);
    }
    log_warn("called h_shift_successor to use the next successor in line");
    monitor_exit(ptr);
    return 1;
}
