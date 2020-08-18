#include "dht.h"
#include "dht_utils.h"
#include "monitor.h"
#include "woofc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef USE_RAFT
#include "raft_client.h"
#endif

int put_fail_rate = 0;

unsigned long lucky_woof_put(char* woof, void* arg, int fail_rate) {
    int r = rand() % 100;
    if (r < fail_rate) {
        return -1;
    }
    return WooFPut(woof, "h_find_successor", arg);
}

int h_find_successor(WOOF* wf, unsigned long seq_no, void* ptr) {
    DHT_FIND_SUCCESSOR_ARG* arg = (DHT_FIND_SUCCESSOR_ARG*)ptr;

    log_set_tag("find_successor");
    log_set_level(DHT_LOG_INFO);
    // log_set_level(DHT_LOG_DEBUG);
    log_set_output(stdout);
    srand((unsigned int)get_milliseconds());

    DHT_NODE_INFO node = {0};
    if (get_latest_node_info(&node) < 0) {
        log_error("couldn't get latest node info: %s", dht_error_msg);
        exit(1);
    }
    DHT_SUCCESSOR_INFO successor = {0};
    if (get_latest_successor_info(&successor) < 0) {
        log_error("couldn't get latest successor info: %s", dht_error_msg);
        exit(1);
    }

    arg->hops++;
    char hash_str[2 * SHA_DIGEST_LENGTH + 1];
    print_node_hash(hash_str, arg->hashed_key);
    log_debug("callback_namespace: %s, hashed_key: %s, hops: %d", arg->callback_namespace, hash_str, arg->hops);

    // if id in (n, successor]
    if (in_range(arg->hashed_key, node.hash, successor.hash[0]) ||
        memcmp(arg->hashed_key, successor.hash[0], SHA_DIGEST_LENGTH) == 0) {
        // return successor
        char* replicas_leader = successor_addr(&successor, 0);
        switch (arg->action) {
        case DHT_ACTION_FIND_NODE: {
            DHT_FIND_NODE_RESULT action_result = {0};
            memcpy(action_result.topic, arg->key, sizeof(action_result.topic));
            memcpy(action_result.node_hash, successor.hash[0], sizeof(action_result.node_hash));
            memcpy(action_result.node_replicas, successor.replicas[0], sizeof(action_result.node_replicas));
            action_result.node_leader = successor.leader[0];
            action_result.hops = arg->hops;

            char callback_woof[DHT_NAME_LENGTH] = {0};
            sprintf(callback_woof, "%s/%s", arg->callback_namespace, DHT_FIND_NODE_RESULT_WOOF);
            unsigned long seq = WooFPut(callback_woof, NULL, &action_result);
            if (WooFInvalid(seq)) {
                log_error("failed to put find_node result on %s", callback_woof);
                exit(1);
            }
            log_debug("found node_addr %s for action %s", replicas_leader, "FIND_NODE");
            break;
        }
        case DHT_ACTION_JOIN: {
            DHT_JOIN_ARG action_arg = {0};
            memcpy(action_arg.node_hash, successor.hash[0], sizeof(action_arg.node_hash));
            memcpy(action_arg.node_replicas, successor.replicas[0], sizeof(action_arg.node_replicas));
            action_arg.node_leader = successor.leader[0];
            strcpy(action_arg.serialized_config, arg->action_namespace);

            char callback_monitor[DHT_NAME_LENGTH] = {0};
            sprintf(callback_monitor, "%s/%s", arg->callback_namespace, DHT_MONITOR_NAME);
            char callback_woof[DHT_NAME_LENGTH] = {0};
            sprintf(callback_woof, "%s/%s", arg->callback_namespace, DHT_JOIN_WOOF);
            unsigned long seq = monitor_remote_put(callback_monitor, callback_woof, "h_join_callback", &action_arg, 0);
            if (WooFInvalid(seq)) {
                log_error("failed to invoke h_join_callback on %s", callback_woof);
                exit(1);
            }
            log_debug("found successor_addr %s for action %s", replicas_leader, "JOIN");
            break;
        }
        case DHT_ACTION_FIX_FINGER: {
            DHT_FIX_FINGER_CALLBACK_ARG action_arg = {0};
            memcpy(action_arg.node_hash, successor.hash[0], sizeof(action_arg.node_hash));
            memcpy(action_arg.node_replicas, successor.replicas[0], sizeof(action_arg.node_replicas));
            action_arg.node_leader = successor.leader[0];
            action_arg.finger_index = (int)arg->action_seqno;

            char callback_monitor[DHT_NAME_LENGTH] = {0};
            sprintf(callback_monitor, "%s/%s", arg->callback_namespace, DHT_MONITOR_NAME);
            char callback_woof[DHT_NAME_LENGTH] = {0};
            sprintf(callback_woof, "%s/%s", arg->callback_namespace, DHT_FIX_FINGER_CALLBACK_WOOF);
            unsigned long seq =
                monitor_remote_put(callback_monitor, callback_woof, "h_fix_finger_callback", &action_arg, 0);
            if (WooFInvalid(seq)) {
                log_error("failed to invoke h_fix_finger_callback on %s", callback_woof);
                exit(1);
            }
            log_debug("found successor_addr %s for action %s", replicas_leader, "FIX_FINGER");
            break;
        }
        case DHT_ACTION_REGISTER_TOPIC: {
            char action_arg_woof[DHT_NAME_LENGTH] = {0};
            sprintf(action_arg_woof, "%s/%s", arg->action_namespace, DHT_REGISTER_TOPIC_WOOF);
            DHT_REGISTER_TOPIC_ARG action_arg = {0};
            if (WooFGet(action_arg_woof, &action_arg, arg->action_seqno) < 0) {
                log_error("failed to get register_topic_arg from %s", action_arg_woof);
                exit(1);
            }
            int i;
            for (i = 0; i < DHT_REPLICA_NUMBER; ++i) {
                char* successor_leader = successor_addr(&successor, i);
                if (successor_leader[0] == 0) {
                    break;
                }
#ifdef USE_RAFT
                unsigned long index = raft_sessionless_put_handler(
                    successor_leader, "h_register_topic", &action_arg, sizeof(DHT_REGISTER_TOPIC_ARG), 1, 0);
                if (raft_is_error(index)) {
                    log_error("failed to invoke h_register_topic on %s: %s", successor_leader, raft_error_msg);
                    exit(1);
                }
#else
                char callback_monitor[DHT_NAME_LENGTH] = {0};
                sprintf(callback_monitor, "%s/%s", successor_leader, DHT_MONITOR_NAME);
                char callback_woof[DHT_NAME_LENGTH] = {0};
                sprintf(callback_woof, "%s/%s", successor_leader, DHT_REGISTER_TOPIC_WOOF);
                unsigned long seq =
                    monitor_remote_put(callback_monitor, callback_woof, "h_register_topic", &action_arg, 0);
                if (WooFInvalid(seq)) {
                    log_error("failed to invoke h_register_topic on %s", callback_woof);
                    exit(1);
                }
#endif
                log_debug("found successor_addr[%d] %s for action %s", i, successor_leader, "REGISTER_TOPIC");
            }
            break;
        }
        case DHT_ACTION_SUBSCRIBE: {
            char action_arg_woof[DHT_NAME_LENGTH] = {0};
            sprintf(action_arg_woof, "%s/%s", arg->action_namespace, DHT_SUBSCRIBE_WOOF);
            DHT_SUBSCRIBE_ARG action_arg = {0};
            if (WooFGet(action_arg_woof, &action_arg, arg->action_seqno) < 0) {
                log_error("failed to get subscribe_arg from %s", action_arg_woof);
                exit(1);
            }
            int i;
            for (i = 0; i < DHT_REPLICA_NUMBER; ++i) {
                char* successor_leader = successor_addr(&successor, i);
                if (successor_leader[0] == 0) {
                    break;
                }
#ifdef USE_RAFT
                unsigned long index = raft_sessionless_put_handler(
                    successor_leader, "h_subscribe", &action_arg, sizeof(DHT_SUBSCRIBE_ARG), 1, 0);
                if (raft_is_error(index)) {
                    log_error("failed to invoke h_subscribe on %s: %s", successor_leader, raft_error_msg);
                    exit(1);
                }
#else
                char callback_monitor[DHT_NAME_LENGTH] = {0};
                sprintf(callback_monitor, "%s/%s", successor_leader, DHT_MONITOR_NAME);
                char callback_woof[DHT_NAME_LENGTH] = {0};
                sprintf(callback_woof, "%s/%s", successor_leader, DHT_SUBSCRIBE_WOOF);
                unsigned long seq = monitor_remote_put(callback_monitor, callback_woof, "h_subscribe", &action_arg);
                if (WooFInvalid(seq)) {
                    log_error("failed to invoke h_subscribe on %s", callback_woof);
                    exit(1);
                }
#endif
                log_debug("found successor_addr[%d] %s for action %s", i, successor_leader, "SUBSCRIBE");
            }
            break;
        }
        default: {
            break;
        }
        }
        return 1;
    }
    // else closest_preceding_node(id)
    char req_forward_woof[DHT_NAME_LENGTH] = {0};
    int i;
    for (i = SHA_DIGEST_LENGTH * 8; i > 0; --i) {
        DHT_FINGER_INFO finger = {0};
        if (get_latest_finger_info(i, &finger) < 0) {
            log_error("failed to get finger[%d]'s info: %s", i, dht_error_msg);
            exit(1);
        }
        if (is_empty(finger.hash)) {
            continue;
        }
        // if finger[i] in (n, id)
        // return finger[i].find_succesor(id)
        if (in_range(finger.hash, node.hash, arg->hashed_key)) {
            char* finger_replicas_leader = finger_addr(&finger);
            sprintf(req_forward_woof, "%s/%s", finger_replicas_leader, DHT_FIND_SUCCESSOR_WOOF);
            // unsigned long seq = WooFPut(req_forward_woof, "h_find_successor", arg);
            unsigned long seq = lucky_woof_put(req_forward_woof, arg, put_fail_rate);
            if (WooFInvalid(seq)) {
                log_warn("failed to forward find_successor request to %s, ACTION: %d", req_forward_woof, arg->action);
                DHT_INVALIDATE_FINGERS_ARG invalidate_fingers_arg = {0};
                memcpy(invalidate_fingers_arg.finger_hash, finger.hash, sizeof(invalidate_fingers_arg.finger_hash));
                seq = monitor_put(
                    DHT_MONITOR_NAME, DHT_INVALIDATE_FINGERS_WOOF, "h_invalidate_fingers", &invalidate_fingers_arg, 0);
                if (WooFInvalid(seq)) {
                    log_error("failed to call h_invalidate_fingers to invalidate failed fingers");
                    continue;
                }
                char hash_str[DHT_NAME_LENGTH] = {0};
                print_node_hash(hash_str, invalidate_fingers_arg.finger_hash);
                log_debug("invoked h_invalidate_fingers to invalidate failed finger %s", hash_str);
                continue;
            }
            log_debug("forwarded find_succesor request to finger[%d]: %s", i, finger_replicas_leader);
            return 1;
        }
    }
    // TODO: also check the other successors
    if (in_range(successor.hash[0], node.hash, arg->hashed_key)) {
        sprintf(req_forward_woof, "%s/%s", successor_addr(&successor, 0), DHT_FIND_SUCCESSOR_WOOF);
        // unsigned long seq = WooFPut(req_forward_woof, "h_find_successor", arg);
        unsigned long seq = lucky_woof_put(req_forward_woof, arg, put_fail_rate);
        if (WooFInvalid(seq)) {
            log_warn("failed to forward find_successor request to %s", req_forward_woof);
#ifdef USE_RAFT
            DHT_TRY_REPLICAS_ARG try_replicas_arg = {0};
            try_replicas_arg.type = DHT_TRY_SUCCESSOR;
            strcpy(try_replicas_arg.woof_name, wf->shared->filename);
            strcpy(try_replicas_arg.handler_name, "h_find_successor");
            try_replicas_arg.seq_no = seq_no;
            seq = WooFPut(DHT_TRY_REPLICAS_WOOF, "r_try_replicas", &try_replicas_arg);
            if (WooFInvalid(seq)) {
                log_error("failed to invoke r_try_replicas");
                exit(1);
            }
#else
            DHT_SHIFT_SUCCESSOR_ARG shift_successor_arg;
            unsigned long seq =
                monitor_put(DHT_MONITOR_NAME, DHT_SHIFT_SUCCESSOR_WOOF, "h_shift_successor", &shift_successor_arg, 0);
            if (WooFInvalid(seq)) {
                log_error("failed to shift successor");
                exit(1);
            }
            log_warn("called h_shift_successor to use the next successor in line");
#endif
        } else {
            log_debug("forwarded find_succesor request to successor: %s", successor_addr(&successor, 0));
            return 1;
        }
    }

    // forwarding to self doesn't count as an extra hop
    arg->hops--;
    // return n.find_succesor(id)
    log_debug("closest_preceding_node not found in finger table");
    unsigned long seq = WooFPut(DHT_FIND_SUCCESSOR_WOOF, "h_find_successor", arg);
    if (WooFInvalid(seq)) {
        log_error("failed to put woof to self");
        exit(1);
    }
    log_debug("forwarded find_succesor request to self");
    return 1;
}
