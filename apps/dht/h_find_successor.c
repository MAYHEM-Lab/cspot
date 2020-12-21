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

static int SELF_FORWARD_DELAY = 1000;
static int MAX_RESOLVE_THREADS = 16;

typedef struct resolve_thread_arg {
    DHT_FIND_SUCCESSOR_ARG arg;
    DHT_NODE_INFO* node;
    DHT_SUCCESSOR_INFO* successor;
} RESOLVE_THREAD_ARG;

void* resolve_thread(void* ptr) {
    RESOLVE_THREAD_ARG* thread_arg = (RESOLVE_THREAD_ARG*)ptr;
    DHT_FIND_SUCCESSOR_ARG* arg = &thread_arg->arg;
    DHT_NODE_INFO* node = thread_arg->node;
    DHT_SUCCESSOR_INFO* successor = thread_arg->successor;
    char hash_str[2 * SHA_DIGEST_LENGTH + 1];
    print_node_hash(hash_str, arg->hashed_key);

    // if id in (n, successor]
    int i;
    for (i = 0; i < DHT_SUCCESSOR_LIST_R && !is_empty(successor->hash[i]); ++i) {
        unsigned char prev_hash[20] = {0};
        if (i == 0) {
            memcpy(prev_hash, node->hash, sizeof(prev_hash));
        } else {
            memcpy(prev_hash, successor->hash[i - 1], sizeof(prev_hash));
        }
        if (in_range(arg->hashed_key, prev_hash, successor->hash[i]) ||
            memcmp(arg->hashed_key, successor->hash[i], SHA_DIGEST_LENGTH) == 0) {
            // return successor
            char* replicas_leader = successor_addr(successor, i);
            switch (arg->action) {
            case DHT_ACTION_FIND_NODE: {
                DHT_FIND_NODE_RESULT action_result = {0};
                memcpy(action_result.topic, arg->key, sizeof(action_result.topic));
                memcpy(action_result.node_hash, successor->hash[i], sizeof(action_result.node_hash));
                memcpy(action_result.node_replicas, successor->replicas[i], sizeof(action_result.node_replicas));
                action_result.node_leader = successor->leader[i];
                // strcpy(action_result.path, arg->path);

                log_info("found_node[%d]: %s", i, successor->replicas[i][action_result.node_leader]);
                char tmp[32];
                print_node_hash(tmp, arg->hashed_key);
                log_info("key: %s", arg->key);
                char callback_woof[DHT_NAME_LENGTH] = {0};
                sprintf(callback_woof, "%s/%s", arg->callback_namespace, DHT_FIND_NODE_RESULT_WOOF);
                unsigned long seq = WooFPut(callback_woof, NULL, &action_result);
                if (WooFInvalid(seq)) {
                    log_error("failed to put find_node result on %s", callback_woof);
                    continue;
                }
                log_debug("found node_addr %s for action %s", replicas_leader, "FIND_NODE");
                break;
            }
            case DHT_ACTION_JOIN: {
                DHT_JOIN_ARG action_arg = {0};
                memcpy(action_arg.replier_hash, node->hash, sizeof(action_arg.node_hash));
                memcpy(action_arg.replier_replicas, node->replicas, sizeof(action_arg.node_replicas));
                action_arg.replier_leader = node->leader_id;
                memcpy(action_arg.node_hash, successor->hash[i], sizeof(action_arg.node_hash));
                memcpy(action_arg.node_replicas, successor->replicas[i], sizeof(action_arg.node_replicas));
                action_arg.node_leader = successor->leader[i];
                strcpy(action_arg.serialized_config, arg->action_namespace);

                char callback_monitor[DHT_NAME_LENGTH] = {0};
                sprintf(callback_monitor, "%s/%s", arg->callback_namespace, DHT_MONITOR_NAME);
                char callback_woof[DHT_NAME_LENGTH] = {0};
                sprintf(callback_woof, "%s/%s", arg->callback_namespace, DHT_JOIN_WOOF);
                unsigned long seq =
                    monitor_remote_put(callback_monitor, callback_woof, "h_join_callback", &action_arg, 0);
                if (WooFInvalid(seq)) {
                    log_error("failed to invoke h_join_callback on %s", callback_woof);
                    continue;
                }
                log_debug("found successor_addr %s for action %s", replicas_leader, "JOIN");
                break;
            }
            case DHT_ACTION_FIX_FINGER: {
                DHT_FIX_FINGER_CALLBACK_ARG action_arg = {0};
                memcpy(action_arg.node_hash, successor->hash[i], sizeof(action_arg.node_hash));
                memcpy(action_arg.node_replicas, successor->replicas[i], sizeof(action_arg.node_replicas));
                action_arg.node_leader = successor->leader[i];
                action_arg.finger_index = (int)arg->action_seqno;

                char callback_monitor[DHT_NAME_LENGTH] = {0};
                sprintf(callback_monitor, "%s/%s", arg->callback_namespace, DHT_MONITOR_NAME);
                char callback_woof[DHT_NAME_LENGTH] = {0};
                sprintf(callback_woof, "%s/%s", arg->callback_namespace, DHT_FIX_FINGER_CALLBACK_WOOF);
                unsigned long seq =
                    monitor_remote_put(callback_monitor, callback_woof, "h_fix_finger_callback", &action_arg, 0);
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
                if (WooFGet(action_arg_woof, &action_arg, arg->action_seqno) < 0) {
                    log_error("failed to get register_topic_arg from %s", action_arg_woof);
                    continue;
                }
                char* successor_leader = successor_addr(successor, i);
                log_info("calling r_register_topic on %s[%d]", successor_leader, i);
                char tmp[32];
                print_node_hash(tmp, arg->hashed_key);
                log_info("key: %s, hashed: %s", arg->key, tmp);
                char successor_monitor[DHT_NAME_LENGTH] = {0};
                sprintf(successor_monitor, "%s/%s", successor_leader, DHT_MONITOR_NAME);
                char successor_woof[DHT_NAME_LENGTH] = {0};
                sprintf(successor_woof, "%s/%s", successor_leader, DHT_REGISTER_TOPIC_WOOF);
                unsigned long seq =
                    monitor_remote_put(successor_monitor, successor_woof, "r_register_topic", &action_arg, 0);
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
                if (WooFGet(action_arg_woof, &action_arg, arg->action_seqno) < 0) {
                    log_error("failed to get subscribe_arg from %s", action_arg_woof);
                    continue;
                }
                char* successor_leader = successor_addr(successor, i);
                log_info("calling r_subscribe on %s[%d]", successor_leader, i);
                char tmp[32];
                print_node_hash(tmp, arg->hashed_key);
                log_info("key: %s, hashed: %s", arg->key, tmp);
                char successor_monitor[DHT_NAME_LENGTH] = {0};
                sprintf(successor_monitor, "%s/%s", successor_leader, DHT_MONITOR_NAME);
                char successor_woof[DHT_NAME_LENGTH] = {0};
                sprintf(successor_woof, "%s/%s", successor_leader, DHT_SUBSCRIBE_WOOF);
                unsigned long seq =
                    monitor_remote_put(successor_monitor, successor_woof, "r_subscribe", &action_arg, 0);
                if (WooFInvalid(seq)) {
                    log_warn("failed to invoke r_subscribe on %s", successor_woof);
                    continue;
                }
                log_debug("found successor_addr[%d] %s for action %s", i, successor_leader, "REGISTER_TOPIC");
                break;
            }
            case DHT_ACTION_PUBLISH_FIND: {
                DHT_SERVER_PUBLISH_DATA_ARG action_arg = {0};
                memcpy(action_arg.node_replicas, successor->replicas[i], sizeof(action_arg.node_replicas));
                action_arg.node_leader = successor->leader[i];
                action_arg.find_arg_seqno = arg->action_seqno;
                memcpy(action_arg.topic_name, arg->key, sizeof(action_arg.topic_name));
                uint64_t found_ts = get_milliseconds();
                printf("requested -> created: %lu\tcreated -> found: %lu\n",
                       arg->created_ts - arg->requested_ts,
                       found_ts - arg->created_ts);

                char callback_woof[DHT_NAME_LENGTH] = {0};
                sprintf(callback_woof, "%s/%s", arg->callback_namespace, DHT_SERVER_PUBLISH_DATA_WOOF);
                unsigned long seq = WooFPut(callback_woof, NULL, &action_arg);
                if (WooFInvalid(seq)) {
                    log_error("failed to invoke server_publish_data on %s", callback_woof);
                    continue;
                }
                log_debug("invoked server_publish_data on %s", callback_woof);
                break;
            }
            default: {
                break;
            }
            }
            return;
        }
    }
    // else closest_preceding_node(id)
    char req_forward_woof[DHT_NAME_LENGTH] = {0};
    for (i = SHA_DIGEST_LENGTH * 8; i > 0; --i) {
        DHT_FINGER_INFO finger = {0};
        if (get_latest_finger_info(i, &finger) < 0) {
            log_error("failed to get finger[%d]'s info: %s", i, dht_error_msg);
            return;
        }
        if (is_empty(finger.hash)) {
            continue;
        }
        // if finger[i] in (n, id)
        // return finger[i].find_succesor(id)
        if (in_range(finger.hash, node->hash, arg->hashed_key)) {
            char* finger_replicas_leader = finger_addr(&finger);
            if (finger_replicas_leader[0] == 0) {
                continue;
            }
            sprintf(req_forward_woof, "%s/%s", finger_replicas_leader, DHT_FIND_SUCCESSOR_WOOF);
            unsigned long seq = WooFPut(req_forward_woof, NULL, arg);
            if (seq == -2) {
                log_debug("the node has failed");
                return;
            }
            if (WooFInvalid(seq)) {
                log_warn("failed to forward find_successor request to finger[%d] %s, ACTION: %d",
                         i,
                         req_forward_woof,
                         arg->action);
                log_warn("invalidating all fingers with same hash to finger %s", finger_replicas_leader);
                if (invalidate_fingers(finger.hash) < 0) {
                    log_error("failed to invalidate fingers");
                    return;
                }
                continue;
            }
            log_debug("forwarded find_succesor request to finger[%d]: %s", i, finger_replicas_leader);
            return;
        }
    }
    for (i = 0; i < DHT_SUCCESSOR_LIST_R; ++i) {
        if (!is_empty(successor->hash[i]) && in_range(successor->hash[i], node->hash, arg->hashed_key)) {
            sprintf(req_forward_woof, "%s/%s", successor_addr(successor, i), DHT_FIND_SUCCESSOR_WOOF);
            unsigned long seq = WooFPut(req_forward_woof, NULL, arg);
            if (seq == -2) {
                log_debug("the node has failed");
                return;
            }
            if (WooFInvalid(seq)) {
                log_warn("failed to forward find_successor request to successor %s, ACTION: %d",
                         req_forward_woof,
                         arg->action);
#ifdef USE_RAFT
                DHT_TRY_REPLICAS_ARG try_replicas_arg;
                seq = monitor_put(DHT_MONITOR_NAME, DHT_TRY_REPLICAS_WOOF, "r_try_replicas", &try_replicas_arg, 1);
                if (WooFInvalid(seq)) {
                    log_error("failed to invoke r_try_replicas");
                    return;
                }
                log_debug("trying successor replicas, forward query to the next successor in line");
#else
                DHT_SHIFT_SUCCESSOR_ARG shift_successor_arg;
                unsigned long seq = monitor_put(
                    DHT_MONITOR_NAME, DHT_SHIFT_SUCCESSOR_WOOF, "h_shift_successor", &shift_successor_arg, 0);
                if (WooFInvalid(seq)) {
                    log_error("failed to shift successor");
                    return;
                }
                log_warn("called h_shift_successor to use the next successor in line");
#endif
            } else {
                log_debug("forwarded find_succesor request to successor: %s", successor_addr(successor, i));
                return;
            }
        }
    }

    // return n.find_succesor(id)
    log_warn("closest_preceding_node not found in finger table");
    usleep(SELF_FORWARD_DELAY * 1000);
    unsigned long seq = WooFPut(DHT_FIND_SUCCESSOR_WOOF, NULL, arg);
    if (WooFInvalid(seq)) {
        log_error("failed to put woof to self");
        return;
    }
    log_debug("forwarded find_succesor request to self");
}

int h_find_successor(WOOF* wf, unsigned long seq_no, void* ptr) {
    DHT_LOOP_ROUTINE_ARG* routine_arg = (DHT_LOOP_ROUTINE_ARG*)ptr;
    log_set_tag("h_find_successor");
    log_set_level(DHT_LOG_INFO);
    // log_set_level(DHT_LOG_DEBUG);
    log_set_output(stdout);
    WooFMsgCacheInit();

    uint64_t begin = get_milliseconds();

    DHT_NODE_INFO node = {0};
    if (get_latest_node_info(&node) < 0) {
        log_error("couldn't get latest node info: %s", dht_error_msg);
        WooFMsgCacheShutdown();
        exit(1);
    }
    DHT_SUCCESSOR_INFO successor = {0};
    if (get_latest_successor_info(&successor) < 0) {
        log_error("couldn't get latest successor info: %s", dht_error_msg);
        WooFMsgCacheShutdown();
        exit(1);
    }

    unsigned long latest_seq = WooFGetLatestSeqno(DHT_FIND_SUCCESSOR_WOOF);
    if (WooFInvalid(latest_seq)) {
        log_error("couldn't get latest seqno of %s", DHT_FIND_SUCCESSOR_WOOF);
        WooFMsgCacheShutdown();
        exit(1);
    }
    int count = latest_seq - routine_arg->last_seqno;
    if (count != 0) {
        log_debug("%lu queued find_successor queries", count);
    }

    zsys_init();
    RESOLVE_THREAD_ARG thread_arg[count];
    pthread_t thread_id[count];

    int i;
    for (i = 0; i < count; ++i) {
        thread_arg[i].node = &node;
        thread_arg[i].successor = &successor;
        if (WooFGet(DHT_FIND_SUCCESSOR_WOOF, &thread_arg[i].arg, routine_arg->last_seqno + 1 + i) < 0) {
            log_error("couldn't get h_find_successor query at %lu", routine_arg->last_seqno + 1 + i);
            WooFMsgCacheShutdown();
            exit(1);
        }
        if (pthread_create(&thread_id[i], NULL, resolve_thread, (void*)&thread_arg[i]) < 0) {
            log_error("failed to create thread to process h_find_successor query at %lu",
                      routine_arg->last_seqno + 1 + i);
            WooFMsgCacheShutdown();
            exit(1);
        }
    }
    routine_arg->last_seqno = latest_seq;

    threads_join(count, thread_id);
    // usleep(wakeup_interval * 1000);
    unsigned long seq = WooFPut(DHT_FIND_SUCCESSOR_ROUTINE_WOOF, "h_find_successor", routine_arg);
    if (WooFInvalid(seq)) {
        log_error("failed to invoke next h_find_successor");
        WooFMsgCacheShutdown();
        exit(1);
    }
    monitor_join();
    // printf("handler h_find_successor took %lu\n", get_milliseconds() - begin);
    WooFMsgCacheShutdown();
    return 1;
}