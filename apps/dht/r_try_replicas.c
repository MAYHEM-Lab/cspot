#include "dht.h"
#include "dht_utils.h"
#include "woofc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// TODO: find the leader
int r_try_replicas(WOOF* wf, unsigned long seq_no, void* ptr) {
    DHT_TRY_REPLICAS_ARG* arg = (DHT_TRY_REPLICAS_ARG*)ptr;

    log_set_tag("try_replicas");
    log_set_level(DHT_LOG_INFO);
    log_set_level(DHT_LOG_DEBUG);
    log_set_output(stdout);

    switch (arg->type) {
    case DHT_TRY_PREDECESSOR: {
        DHT_PREDECESSOR_INFO predecessor = {0};
        if (get_latest_predecessor_info(&predecessor) < 0) {
            log_error("couldn't get latest predecessor info: %s", dht_error_msg);
            exit(1);
        }
        if (is_empty(predecessor.hash)) {
            log_debug("predecessor is nil");
            return 1;
        }

        int start_id = predecessor.leader;
        int i;
        for (i = (start_id + 1) % DHT_REPLICA_NUMBER; i != start_id; i = (i + 1) % DHT_REPLICA_NUMBER) {
            predecessor.leader = i;
            if (predecessor_addr(&predecessor)[0] == 0) {
                break;
            }
            log_warn("trying predecessor replica[%d]: %s", i, predecessor_addr(&predecessor));
            // check if the replica is leader
            char replica_woof[DHT_NAME_LENGTH];
            sprintf(replica_woof, "%s/%s", predecessor_addr(&predecessor), DHT_NODE_INFO_WOOF);
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
            predecessor.leader = node.leader_id;
			log_debug("got leader_id %d from %s", node.leader_id, replica_woof);
            unsigned long seq = WooFPut(DHT_PREDECESSOR_INFO_WOOF, NULL, &predecessor);
            if (WooFInvalid(seq)) {
                log_error("failed to update predecessor to use replica[%d]: %s", node.leader_id, predecessor_addr(&predecessor));
                exit(1);
            }
            log_debug("updated predecessor to use replica[%d]: %s", node.leader_id, predecessor_addr(&predecessor));
            return 1;
        }
        log_warn("none of predecessor replicas is responding");
        memset(&predecessor, 0, sizeof(predecessor));
        unsigned long seq = WooFPut(DHT_PREDECESSOR_INFO_WOOF, NULL, &predecessor);
        if (WooFInvalid(seq)) {
            log_error("failed to set predecessor to nil");
            exit(1);
        }
        log_warn("set predecessor to nil");
        break;
    }
    case DHT_TRY_SUCCESSOR: {
        DHT_SUCCESSOR_INFO successor = {0};
        if (get_latest_successor_info(&successor) < 0) {
            log_error("couldn't get latest successor info: %s", dht_error_msg);
            exit(1);
        }
        if (is_empty(successor.hash[0])) {
            log_debug("successor is nil");
            return 1;
        }

        int start_id = successor.leader[0];
        int i;
        for (i = (start_id + 1) % DHT_REPLICA_NUMBER; i != start_id; i = (i + 1) % DHT_REPLICA_NUMBER) {
            successor.leader[0] = i;
            if (successor_addr(&successor, 0)[0] == 0) {
                break;
            }
            log_warn("trying successor replica[%d]: %s", i, successor_addr(&successor, 0));
            // check if the replica is leader
            char replica_woof[DHT_NAME_LENGTH];
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
            successor.leader[0] = node.leader_id;
			log_debug("got leader_id %d from %s", node.leader_id, replica_woof);
            unsigned long seq = WooFPut(DHT_SUCCESSOR_INFO_WOOF, NULL, &successor);
            if (WooFInvalid(seq)) {
                log_error("failed to update successor to use replica[%d]: %s", node.leader_id, successor_addr(&successor, 0));
                exit(1);
            }
            log_debug("updated successor to use replica[%d]: %s", node.leader_id, successor_addr(&successor, 0));
            return 1;
        }
        log_warn("none of successor replicas is responding");
        shift_successor_list(&successor);
        unsigned long seq = WooFPut(DHT_SUCCESSOR_INFO_WOOF, NULL, &successor);
        if (WooFInvalid(seq)) {
            log_error("failed to shift successor");
            exit(1);
        }
        log_warn("use the next successor in line: %s", successor_addr(&successor, 0));
        break;
    }
    case DHT_TRY_FINGER: {
        int finger_id = arg->finger_id;
        DHT_FINGER_INFO finger = {0};
        if (get_latest_finger_info(finger_id, &finger) < 0) {
            log_error("couldn't get latest finger[%d] info: %s", finger_id, dht_error_msg);
            exit(1);
        }
        if (is_empty(finger.hash)) {
            log_debug("finger[%d] is nil", finger_id);
            return 1;
        }

        int start_id = finger.leader;
        int i;
        for (i = (start_id + 1) % DHT_REPLICA_NUMBER; i != start_id; i = (i + 1) % DHT_REPLICA_NUMBER) {
            finger.leader = i;
            if (finger_addr(&finger)[0] == 0) {
                continue;
            }
            log_warn("trying finger replica[%d]: %s", i, finger_addr(&finger));
            // check if predecessor woof is working, do nothing
            DHT_GET_PREDECESSOR_ARG get_predecessor_arg = {0};
            char finger_woof_name[DHT_NAME_LENGTH];
            sprintf(finger_woof_name, "%s/%s", finger_addr(&finger), DHT_GET_PREDECESSOR_WOOF);
            unsigned long seq = WooFPut(finger_woof_name, NULL, &get_predecessor_arg);
            if (WooFInvalid(seq)) {
                log_warn("finger replica[%d] is not responding: %s", i, finger_addr(&finger));
                continue;
            }
            log_debug("finger replica[%d] %s is working", i, finger_addr(&finger));
            seq = set_finger_info(finger_id, &finger);
            if (WooFInvalid(seq)) {
                log_error("failed to update finger[%d] to use replica[%d]: %s", finger_id, i, finger_addr(&finger));
                exit(1);
            }
            log_debug("updated finger[%d] to use replica[%d]: %s", finger_id, i, finger_addr(&finger));
            return 1;
        }
        log_warn("none of finger[%d] replicas is responding", finger_id);
        memset(&finger, 0, sizeof(finger));
        unsigned long seq = set_finger_info(finger_id, &finger);
        if (WooFInvalid(seq)) {
            log_error("failed to set finger[%d] to nil", finger_id);
            exit(1);
        }
        log_warn("set finger[%d] to nil", finger_id);
        break;
    }
    default:
        break;
    }

    if (arg->handler_name[0] == 0) {
        return 1;
    } else if (strcmp(arg->handler_name, "h_find_successor") == 0) {
        DHT_FIND_SUCCESSOR_ARG find_successor_arg = {0};
        int err = WooFGet(arg->woof_name, &find_successor_arg, arg->seq_no);
        if (err < 0) {
            log_error("failed to get the original h_find_successor arg woof");
            exit(1);
        }
        unsigned long seq = WooFPut(arg->woof_name, arg->handler_name, &find_successor_arg);
        if (WooFInvalid(seq)) {
            log_error("failed to invoke h_find_successor after tried other replcas");
            exit(1);
        }
        log_debug("invoked h_find_successor with the original arg after tried other replcas");
    } else {
        log_error("unrecognized handler %s", arg->handler_name);
    }
    return 1;
}
