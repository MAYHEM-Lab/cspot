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
int self_forward_delay = 1000;

int h_find_successor(WOOF* wf, unsigned long seq_no, void* ptr) {
    DHT_FIND_SUCCESSOR_ARG* arg = (DHT_FIND_SUCCESSOR_ARG*)ptr;

    log_set_tag("find_successor");
    log_set_level(DHT_LOG_INFO);
    // log_set_level(DHT_LOG_DEBUG);
    log_set_output(stdout);

    // srand((unsigned int)get_milliseconds());

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
    BLOCKED_NODES blocked_nodes = {0};
    if (get_latest_element(BLOCKED_NODES_WOOF, &blocked_nodes) < 0) {
        log_error("failed to get blocked nodes");
    }

    // sprintf(arg->path + strlen(arg->path), " %s", node.addr);
    char hash_str[2 * SHA_DIGEST_LENGTH + 1];
    print_node_hash(hash_str, arg->hashed_key);
    log_debug("callback_namespace: %s, hashed_key: %s, query_count: %d, message_count: %d, failure_count: %d",
              arg->callback_namespace,
              hash_str,
              arg->query_count,
              arg->message_count,
              arg->failure_count);
    // if id in (n, successor]
    int i;
    for (i = 0; i < DHT_SUCCESSOR_LIST_R && !is_empty(successor.hash[i]); ++i) {
        unsigned char prev_hash[20] = {0};
        if (i == 0) {
            memcpy(prev_hash, node.hash, sizeof(prev_hash));
        } else {
            memcpy(prev_hash, successor.hash[i - 1], sizeof(prev_hash));
        }
        if (in_range(arg->hashed_key, prev_hash, successor.hash[i]) ||
            memcmp(arg->hashed_key, successor.hash[i], SHA_DIGEST_LENGTH) == 0) {
            // return successor
            char* replicas_leader = successor_addr(&successor, i);
            switch (arg->action) {
            case DHT_ACTION_FIND_NODE: {
                DHT_FIND_NODE_RESULT action_result = {0};
                memcpy(action_result.topic, arg->key, sizeof(action_result.topic));
                memcpy(action_result.node_hash, successor.hash[i], sizeof(action_result.node_hash));
                memcpy(action_result.node_replicas, successor.replicas[i], sizeof(action_result.node_replicas));
                action_result.node_leader = successor.leader[i];
                action_result.find_successor_query_count = arg->query_count;
                action_result.find_successor_message_count = arg->message_count;
                action_result.find_successor_failure_count = arg->failure_count;
                // strcpy(action_result.path, arg->path);

                log_info("found_node[%d]: %s", i, successor.replicas[i][action_result.node_leader]);
                char tmp[32];
                print_node_hash(tmp, arg->hashed_key);
                log_info("key: %s", arg->key);
                char callback_woof[DHT_NAME_LENGTH] = {0};
                sprintf(callback_woof, "%s/%s", arg->callback_namespace, DHT_FIND_NODE_RESULT_WOOF);
                unsigned long seq = checkedWooFPut(&blocked_nodes, node.addr, callback_woof, NULL, &action_result);
                if (WooFInvalid(seq)) {
                    log_error("failed to put find_node result on %s", callback_woof);
                    continue;
                }
                log_debug("found node_addr %s for action %s", replicas_leader, "FIND_NODE");
                break;
            }
            case DHT_ACTION_JOIN: {
                DHT_JOIN_ARG action_arg = {0};
                memcpy(action_arg.replier_hash, node.hash, sizeof(action_arg.node_hash));
                memcpy(action_arg.replier_replicas, node.replicas, sizeof(action_arg.node_replicas));
                action_arg.replier_leader = node.leader_id;
                memcpy(action_arg.node_hash, successor.hash[i], sizeof(action_arg.node_hash));
                memcpy(action_arg.node_replicas, successor.replicas[i], sizeof(action_arg.node_replicas));
                action_arg.node_leader = successor.leader[i];
                strcpy(action_arg.serialized_config, arg->action_namespace);

                char callback_monitor[DHT_NAME_LENGTH] = {0};
                sprintf(callback_monitor, "%s/%s", arg->callback_namespace, DHT_MONITOR_NAME);
                char callback_woof[DHT_NAME_LENGTH] = {0};
                sprintf(callback_woof, "%s/%s", arg->callback_namespace, DHT_JOIN_WOOF);
                unsigned long seq = checkedMonitorRemotePut(
                    &blocked_nodes, node.addr, callback_monitor, callback_woof, "h_join_callback", &action_arg, 0);
                if (WooFInvalid(seq)) {
                    log_error("failed to invoke h_join_callback on %s", callback_woof);
                    continue;
                }
                log_debug("found successor_addr %s for action %s", replicas_leader, "JOIN");
                break;
            }
            case DHT_ACTION_FIX_FINGER: {
                DHT_FIX_FINGER_CALLBACK_ARG action_arg = {0};
                memcpy(action_arg.node_hash, successor.hash[i], sizeof(action_arg.node_hash));
                memcpy(action_arg.node_replicas, successor.replicas[i], sizeof(action_arg.node_replicas));
                action_arg.node_leader = successor.leader[i];
                action_arg.finger_index = (int)arg->action_seqno;

                char callback_monitor[DHT_NAME_LENGTH] = {0};
                sprintf(callback_monitor, "%s/%s", arg->callback_namespace, DHT_MONITOR_NAME);
                char callback_woof[DHT_NAME_LENGTH] = {0};
                sprintf(callback_woof, "%s/%s", arg->callback_namespace, DHT_FIX_FINGER_CALLBACK_WOOF);
                unsigned long seq = checkedMonitorRemotePut(&blocked_nodes,
                                                            node.addr,
                                                            callback_monitor,
                                                            callback_woof,
                                                            "h_fix_finger_callback",
                                                            &action_arg,
                                                            0);
                if (WooFInvalid(seq)) {
                    log_warn("failed to invoke h_fix_finger_callback on %s", callback_woof);
                    continue;
                }
                log_debug("found successor_addr %s for action %s", replicas_leader, "FIX_FINGER");
                break;
            }
            case DHT_ACTION_REGISTER_TOPIC: {
                char action_arg_woof[DHT_NAME_LENGTH] = {0};
                sprintf(action_arg_woof, "%s/%s", arg->action_namespace, DHT_REGISTER_TOPIC_WOOF);
                DHT_REGISTER_TOPIC_ARG action_arg = {0};
                if (checkedWooFGet(&blocked_nodes, node.addr, action_arg_woof, &action_arg, arg->action_seqno) < 0) {
                    log_error("failed to get register_topic_arg from %s", action_arg_woof);
                    continue;
                }
                char* successor_leader = successor_addr(&successor, i);
                log_info("calling r_register_topic on %s[%d]", successor_leader, i);
                char tmp[32];
                print_node_hash(tmp, arg->hashed_key);
                log_info("key: %s, hashed: %s", arg->key, tmp);
                char successor_monitor[DHT_NAME_LENGTH] = {0};
                sprintf(successor_monitor, "%s/%s", successor_leader, DHT_MONITOR_NAME);
                char successor_woof[DHT_NAME_LENGTH] = {0};
                sprintf(successor_woof, "%s/%s", successor_leader, DHT_REGISTER_TOPIC_WOOF);
                unsigned long seq = checkedMonitorRemotePut(
                    &blocked_nodes, node.addr, successor_monitor, successor_woof, "r_register_topic", &action_arg, 0);
                if (WooFInvalid(seq)) {
                    log_warn("failed to invoke r_register_topic on %s", successor_woof);
                    continue;
                }
                log_debug("found successor_addr[%d] %s for action %s", i, successor_leader, "REGISTER_TOPIC");
                break;
            }
            case DHT_ACTION_SUBSCRIBE: {
                char action_arg_woof[DHT_NAME_LENGTH] = {0};
                sprintf(action_arg_woof, "%s/%s", arg->action_namespace, DHT_SUBSCRIBE_WOOF);
                DHT_SUBSCRIBE_ARG action_arg = {0};
                if (checkedWooFGet(&blocked_nodes, node.addr, action_arg_woof, &action_arg, arg->action_seqno) < 0) {
                    log_error("failed to get subscribe_arg from %s", action_arg_woof);
                    continue;
                }
                char* successor_leader = successor_addr(&successor, i);
                log_info("calling r_subscribe on %s[%d]", successor_leader, i);
                char tmp[32];
                print_node_hash(tmp, arg->hashed_key);
                log_info("key: %s, hashed: %s", arg->key, tmp);
                char successor_monitor[DHT_NAME_LENGTH] = {0};
                sprintf(successor_monitor, "%s/%s", successor_leader, DHT_MONITOR_NAME);
                char successor_woof[DHT_NAME_LENGTH] = {0};
                sprintf(successor_woof, "%s/%s", successor_leader, DHT_SUBSCRIBE_WOOF);
                unsigned long seq = checkedMonitorRemotePut(
                    &blocked_nodes, node.addr, successor_monitor, successor_woof, "r_subscribe", &action_arg, 0);
                if (WooFInvalid(seq)) {
                    log_warn("failed to invoke r_subscribe on %s", successor_woof);
                    continue;
                }
                log_debug("found successor_addr[%d] %s for action %s", i, successor_leader, "REGISTER_TOPIC");
                break;
            }
            default: {
                break;
            }
            }
            return 1;
        }
    }
    // else closest_preceding_node(id)
    char req_forward_woof[DHT_NAME_LENGTH] = {0};
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
            if (finger_replicas_leader[0] == 0) {
                continue;
            }
            sprintf(req_forward_woof, "%s/%s", finger_replicas_leader, DHT_FIND_SUCCESSOR_WOOF);
            ++arg->query_count;
            ++arg->message_count;
            unsigned long seq = checkedWooFPut(&blocked_nodes, node.addr, req_forward_woof, "h_find_successor", arg);
            if (seq == -2) {
                log_debug("the node has failed");
                return 1;
            }
            if (WooFInvalid(seq)) {
                --arg->query_count;
                ++arg->failure_count;
                log_warn("failed to forward find_successor request to finger[%d] %s, ACTION: %d",
                         i,
                         req_forward_woof,
                         arg->action);
                log_warn("invalidating all fingers with same hash to finger %s", finger_replicas_leader);
                if (invalidate_fingers(finger.hash) < 0) {
                    log_error("failed to invalidate fingers");
                    exit(1);
                }
                continue;
            }
            log_debug("forwarded find_succesor request to finger[%d]: %s", i, finger_replicas_leader);
            return 1;
        }
    }
    for (i = 0; i < DHT_SUCCESSOR_LIST_R; ++i) {
        if (!is_empty(successor.hash[i]) && in_range(successor.hash[i], node.hash, arg->hashed_key)) {
            sprintf(req_forward_woof, "%s/%s", successor_addr(&successor, i), DHT_FIND_SUCCESSOR_WOOF);
            ++arg->query_count;
            ++arg->message_count;
            unsigned long seq = WooFPut(req_forward_woof, "h_find_successor", arg);
            if (seq == -2) {
                log_debug("the node has failed");
                return 1;
            }
            if (WooFInvalid(seq)) {
                --arg->query_count;
                ++arg->failure_count;
                log_warn("failed to forward find_successor request to successor %s, ACTION: %d",
                         req_forward_woof,
                         arg->action);
#ifdef USE_RAFT
                DHT_TRY_REPLICAS_ARG try_replicas_arg;
                seq = monitor_put(DHT_MONITOR_NAME, DHT_TRY_REPLICAS_WOOF, "r_try_replicas", &try_replicas_arg, 1);
                if (WooFInvalid(seq)) {
                    log_error("failed to invoke r_try_replicas");
                    exit(1);
                }
                log_debug("trying successor replicas, forward query to the next successor in line");
#else
                DHT_SHIFT_SUCCESSOR_ARG shift_successor_arg;
                unsigned long seq = monitor_put(
                    DHT_MONITOR_NAME, DHT_SHIFT_SUCCESSOR_WOOF, "h_shift_successor", &shift_successor_arg, 0);
                if (WooFInvalid(seq)) {
                    log_error("failed to shift successor");
                    exit(1);
                }
                log_warn("called h_shift_successor to use the next successor in line");
#endif
            } else {
                log_debug("forwarded find_succesor request to successor: %s", successor_addr(&successor, i));
                return 1;
            }
        }
    }

    // return n.find_succesor(id)
    log_warn("closest_preceding_node not found in finger table");
    usleep(self_forward_delay * 1000);
    unsigned long seq = WooFPut(DHT_FIND_SUCCESSOR_WOOF, "h_find_successor", arg);
    if (WooFInvalid(seq)) {
        log_error("failed to put woof to self");
        exit(1);
    }
    log_debug("forwarded find_succesor request to self");
    return 1;
}
