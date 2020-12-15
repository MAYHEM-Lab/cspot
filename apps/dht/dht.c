#include "dht.h"

#include "dht_utils.h"
#include "monitor.h"
#include "woofc.h"

#include <openssl/sha.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

char DHT_WOOF_TO_CREATE[][DHT_NAME_LENGTH] = {DHT_CHECK_PREDECESSOR_WOOF,
                                              DHT_DAEMON_WOOF,
                                              DHT_FIND_NODE_RESULT_WOOF,
                                              DHT_FIND_SUCCESSOR_ROUTINE_WOOF,
                                              DHT_FIND_SUCCESSOR_WOOF,
                                              DHT_FIX_FINGER_WOOF,
                                              DHT_FIX_FINGER_CALLBACK_WOOF,
                                              DHT_GET_PREDECESSOR_WOOF,
                                              DHT_INVALIDATE_FINGERS_WOOF,
                                              DHT_INVOCATION_WOOF,
                                              DHT_JOIN_WOOF,
                                              DHT_NOTIFY_CALLBACK_WOOF,
                                              DHT_NOTIFY_WOOF,
                                              DHT_REGISTER_TOPIC_WOOF,
                                              DHT_SHIFT_SUCCESSOR_WOOF,
                                              DHT_STABILIZE_WOOF,
                                              DHT_STABILIZE_CALLBACK_WOOF,
                                              DHT_SUBSCRIBE_WOOF,
                                              DHT_TRIGGER_ROUTINE_WOOF,
                                              DHT_PARTIAL_TRIGGER_WOOF,
                                              DHT_TRIGGER_WOOF,
                                              DHT_NODE_INFO_WOOF,
                                              DHT_PREDECESSOR_INFO_WOOF,
                                              DHT_SUCCESSOR_INFO_WOOF,
                                              DHT_SERVER_PUBLISH_FIND_WOOF,
                                              DHT_SERVER_PUBLISH_DATA_WOOF,
                                              DHT_SERVER_PUBLISH_TRIGGER_WOOF,
                                              DHT_SERVER_LOOP_ROUTINE_WOOF,
                                              DHT_TRY_REPLICAS_WOOF};

unsigned long DHT_WOOF_ELEMENT_SIZE[] = {sizeof(DHT_CHECK_PREDECESSOR_ARG),
                                         sizeof(DHT_DAEMON_ARG),
                                         sizeof(DHT_FIND_NODE_RESULT),
                                         sizeof(DHT_LOOP_ROUTINE_ARG),
                                         sizeof(DHT_FIND_SUCCESSOR_ARG),
                                         sizeof(DHT_FIX_FINGER_ARG),
                                         sizeof(DHT_FIX_FINGER_CALLBACK_ARG),
                                         sizeof(DHT_GET_PREDECESSOR_ARG),
                                         sizeof(DHT_INVALIDATE_FINGERS_ARG),
                                         sizeof(DHT_INVOCATION_ARG),
                                         sizeof(DHT_JOIN_ARG),
                                         sizeof(DHT_NOTIFY_CALLBACK_ARG),
                                         sizeof(DHT_NOTIFY_ARG),
                                         sizeof(DHT_REGISTER_TOPIC_ARG),
                                         sizeof(DHT_SHIFT_SUCCESSOR_ARG),
                                         sizeof(DHT_STABILIZE_ARG),
                                         sizeof(DHT_STABILIZE_CALLBACK_ARG),
                                         sizeof(DHT_SUBSCRIBE_ARG),
                                         sizeof(DHT_LOOP_ROUTINE_ARG),
                                         sizeof(DHT_TRIGGER_ARG),
                                         sizeof(DHT_TRIGGER_ARG),
                                         sizeof(DHT_NODE_INFO),
                                         sizeof(DHT_PREDECESSOR_INFO),
                                         sizeof(DHT_SUCCESSOR_INFO),
                                         sizeof(DHT_SERVER_PUBLISH_FIND_ARG),
                                         sizeof(DHT_SERVER_PUBLISH_DATA_ARG),
                                         sizeof(DHT_SERVER_PUBLISH_TRIGGER_ARG),
                                         sizeof(DHT_LOOP_ROUTINE_ARG),
                                         sizeof(DHT_TRY_REPLICAS_WOOF)};

unsigned long DHT_ELEMENT_SIZE[] = {
    DHT_HISTORY_LENGTH_SHORT,      // DHT_CHECK_PREDECESSOR_WOOF,
    DHT_HISTORY_LENGTH_SHORT,      // DHT_DAEMON_WOOF,
    DHT_HISTORY_LENGTH_LONG,       // DHT_FIND_NODE_RESULT_WOOF,
    DHT_HISTORY_LENGTH_SHORT,      // DHT_FIND_SUCCESSOR_ROUTINE_WOOF,
    DHT_HISTORY_LENGTH_LONG,       // DHT_FIND_SUCCESSOR_WOOF,
    DHT_HISTORY_LENGTH_LONG,       // DHT_FIX_FINGER_WOOF,
    DHT_HISTORY_LENGTH_LONG,       // DHT_FIX_FINGER_CALLBACK_WOOF,
    DHT_HISTORY_LENGTH_LONG,       // DHT_GET_PREDECESSOR_WOOF,
    DHT_HISTORY_LENGTH_LONG,       // DHT_INVALIDATE_FINGER_WOOF,
    DHT_HISTORY_LENGTH_LONG,       // DHT_INVOCATION_WOOF,
    DHT_HISTORY_LENGTH_SHORT,      // DHT_JOIN_WOOF,
    DHT_HISTORY_LENGTH_LONG,       // DHT_NOTIFY_CALLBACK_WOOF,
    DHT_HISTORY_LENGTH_LONG,       // DHT_NOTIFY_WOOF,
    DHT_HISTORY_LENGTH_LONG,       // DHT_REGISTER_TOPIC_WOOF,
    DHT_HISTORY_LENGTH_SHORT,      // DHT_SHIFT_SUCCESSOR_WOOF,
    DHT_HISTORY_LENGTH_SHORT,      // DHT_STABILIZE_WOOF,
    DHT_HISTORY_LENGTH_SHORT,      // DHT_STABILIZE_CALLBACK_WOOF,
    DHT_HISTORY_LENGTH_LONG,       // DHT_SUBSCRIBE_WOOF,
    DHT_HISTORY_LENGTH_LONG,       // DHT_TRIGGER_ROUTINE_WOOF,
    DHT_HISTORY_LENGTH_LONG,       // DHT_PARTIAL_TRIGGER_WOOF,
    DHT_HISTORY_LENGTH_LONG,       // DHT_TRIGGER_WOOF,
    DHT_HISTORY_LENGTH_SHORT,      // DHT_NODE_INFO_WOOF,
    DHT_HISTORY_LENGTH_SHORT,      // DHT_PREDECESSOR_INFO_WOOF,
    DHT_HISTORY_LENGTH_SHORT,      // DHT_SUCCESSOR_INFO_WOOF,
    DHT_HISTORY_LENGTH_EXTRA_LONG, // DHT_SERVER_PUBLISH_FIND_WOOF
    DHT_HISTORY_LENGTH_EXTRA_LONG, // DHT_SERVER_PUBLISH_DATA_WOOF
    DHT_HISTORY_LENGTH_EXTRA_LONG, // DHT_SERVER_PUBLISH_TRIGGER_ARG
    DHT_HISTORY_LENGTH_LONG,       // DHT_SERVER_LOOP_ROUTINE_WOOF,
    DHT_HISTORY_LENGTH_SHORT       // DHT_TRY_REPLICAS_WOOF,
};

int dht_create_woofs() {
    int num_woofs = sizeof(DHT_WOOF_TO_CREATE) / DHT_NAME_LENGTH;
    if (num_woofs != sizeof(DHT_WOOF_ELEMENT_SIZE) / sizeof(unsigned long) ||
        sizeof(DHT_WOOF_ELEMENT_SIZE) != sizeof(DHT_ELEMENT_SIZE)) {
        sprintf(dht_error_msg, "inconsistent woof size");
        return -1;
    }
    int i;
    for (i = 0; i < num_woofs; ++i) {
        if (WooFCreate(DHT_WOOF_TO_CREATE[i], DHT_WOOF_ELEMENT_SIZE[i], DHT_ELEMENT_SIZE[i]) < 0) {
            sprintf(dht_error_msg, "failed to create %s", DHT_WOOF_TO_CREATE[i]);
            return -1;
        }
    }
    char finger_woof_name[DHT_NAME_LENGTH] = {0};
    for (i = 1; i < SHA_DIGEST_LENGTH * 8 + 1; ++i) {
        sprintf(finger_woof_name, "%s%d", DHT_FINGER_INFO_WOOF, i);
        if (WooFCreate(finger_woof_name, sizeof(DHT_FINGER_INFO), DHT_HISTORY_LENGTH_LONG) < 0) {
            sprintf(dht_error_msg, "failed to create %s", finger_woof_name);
            return -1;
        }
    }

    return 0;
}

int dht_start_app_server() {
    DHT_LOOP_ROUTINE_ARG routine_arg = {0};
    routine_arg.last_seqno = 0;
    unsigned long seq = WooFPut(DHT_TRIGGER_ROUTINE_WOOF, "h_trigger", &routine_arg);
    if (WooFInvalid(seq)) {
        fprintf(stderr, "failed to start h_trigger\n");
        return -1;
    }
    seq = WooFPut(DHT_SERVER_LOOP_ROUTINE_WOOF, "server_publish_find", &routine_arg);
    if (WooFInvalid(seq)) {
        fprintf(stderr, "failed to start server_publish_find\n");
        return -1;
    }
    seq = WooFPut(DHT_SERVER_LOOP_ROUTINE_WOOF, "server_publish_data", &routine_arg);
    if (WooFInvalid(seq)) {
        fprintf(stderr, "failed to start server_publish_data\n");
        return -1;
    }
    seq = WooFPut(DHT_SERVER_LOOP_ROUTINE_WOOF, "server_publish_trigger", &routine_arg);
    if (WooFInvalid(seq)) {
        fprintf(stderr, "failed to start server_publish_trigger\n");
        return -1;
    }
}

int dht_start_daemon(
    int stabilize_freq, int chk_predecessor_freq, int fix_finger_freq, int update_leader_freq, int daemon_wakeup_freq) {
    DHT_DAEMON_ARG arg = {0};
    arg.last_stabilize = 0;
    arg.last_check_predecessor = 0;
    arg.last_fix_finger = 0;
#ifdef USE_RAFT
    arg.last_update_leader_id = 0;
    arg.last_replicate_state = 0;
#endif
    arg.stabilize_freq = stabilize_freq;
    arg.chk_predecessor_freq = chk_predecessor_freq;
    arg.fix_finger_freq = fix_finger_freq;
    arg.update_leader_freq = update_leader_freq;
    arg.daemon_wakeup_freq = daemon_wakeup_freq;
    unsigned long seq = WooFPut(DHT_DAEMON_WOOF, "d_daemon", &arg);
    if (WooFInvalid(seq)) {
        fprintf(stderr, "failed to start d_daemon\n");
        return -1;
    }
    DHT_LOOP_ROUTINE_ARG find_routine_arg = {0};
    find_routine_arg.last_seqno = 0;
    seq = WooFPut(DHT_FIND_SUCCESSOR_ROUTINE_WOOF, "h_find_successor", &find_routine_arg);
    if (WooFInvalid(seq)) {
        fprintf(stderr, "failed to start h_find_successor\n");
        return -1;
    }
    if (dht_start_app_server() < 0) {
        fprintf(stderr, "failed to start app server\n");
        return -1;
    }
    return 0;
}

int dht_create_cluster(char* woof_name,
                       char* node_name,
                       char replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH],
                       int stabilize_freq,
                       int chk_predecessor_freq,
                       int fix_finger_freq,
                       int update_leader_freq,
                       int daemon_wakeup_freq) {
    int replica_id;
    for (replica_id = 0; replica_id < DHT_REPLICA_NUMBER; ++replica_id) {
        if (strcmp(woof_name, replicas[replica_id]) == 0) {
            break;
        }
    }
    if (replica_id == DHT_REPLICA_NUMBER) {
        sprintf(dht_error_msg, "%s is not in replicas", woof_name);
        return -1;
    }

    if (monitor_create(DHT_MONITOR_NAME) < 0) {
        sprintf(dht_error_msg, "failed to create and start the handler monitor\n");
        return -1;
    }

    unsigned char node_hash[SHA_DIGEST_LENGTH];
    dht_hash(node_hash, node_name);

    if (dht_init(node_hash, node_name, woof_name, replicas) < 0) {
        sprintf(dht_error_msg, "failed to initialize DHT: %s", dht_error_msg);
        return -1;
    }

    if (dht_start_daemon(
            stabilize_freq, chk_predecessor_freq, fix_finger_freq, update_leader_freq, daemon_wakeup_freq) < 0) {
        sprintf(dht_error_msg, "failed to start daemon");
        return -1;
    }
    printf("stabilize_freq: %d\n", stabilize_freq);
    printf("chk_predecessor_freq: %d\n", chk_predecessor_freq);
    printf("fix_finger_freq: %d\n", fix_finger_freq);
    printf("update_leader_freq: %d\n", update_leader_freq);
    printf("daemon_wakeup_freq: %d\n", daemon_wakeup_freq);
    char hash_str[DHT_NAME_LENGTH] = {0};
    print_node_hash(hash_str, node_hash);
    printf("node_name: %s\nnode_hash: %s\nnode_addr: %s\nid: %d\n", node_name, hash_str, woof_name, replica_id);
    int i;
    for (i = 0; i < DHT_REPLICA_NUMBER; ++i) {
        if (replicas[i][0] == 0) {
            break;
        }
        printf("replica: %s\n", replicas[i]);
    }
    return 0;
}

int dht_join_cluster(char* node_woof,
                     char* woof_name,
                     char* node_name,
                     char replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH],
                     int rejoin,
                     int stabilize_freq,
                     int chk_predecessor_freq,
                     int fix_finger_freq,
                     int update_leader_freq,
                     int daemon_wakeup_freq) {
    int replica_id;
    for (replica_id = 0; replica_id < DHT_REPLICA_NUMBER; ++replica_id) {
        if (strcmp(woof_name, replicas[replica_id]) == 0) {
            break;
        }
    }
    if (replica_id == DHT_REPLICA_NUMBER) {
        sprintf(dht_error_msg, "%s is not in replicas", woof_name);
        return -1;
    }

    unsigned char node_hash[SHA_DIGEST_LENGTH];
    dht_hash(node_hash, node_name);

    if (rejoin == 0) {
        if (monitor_create(DHT_MONITOR_NAME) < 0) {
            sprintf(dht_error_msg, "failed to create and start the handler monitor\n");
            return -1;
        }
        if (dht_init(node_hash, node_name, woof_name, replicas) < 0) {
            sprintf(dht_error_msg, "failed to initialize DHT: %s", dht_error_msg);
            return -1;
        }
    }

    DHT_FIND_SUCCESSOR_ARG arg;
    dht_init_find_arg(&arg, woof_name, node_hash, woof_name);
    serialize_dht_config(arg.action_namespace,
                         stabilize_freq,
                         chk_predecessor_freq,
                         fix_finger_freq,
                         update_leader_freq,
                         daemon_wakeup_freq);
    // printf("stabilize_freq: %d\n", stabilize_freq);
    // printf("chk_predecessor_freq: %d\n", chk_predecessor_freq);
    // printf("fix_finger_freq: %d\n", fix_finger_freq);
    // printf("update_leader_freq: %d\n", update_leader_freq);
    // printf("daemon_wakeup_freq: %d\n", daemon_wakeup_freq);
    arg.action = DHT_ACTION_JOIN;
    if (node_woof[strlen(node_woof) - 1] == '/') {
        sprintf(node_woof, "%s%s", node_woof, DHT_FIND_SUCCESSOR_WOOF);
    } else {
        sprintf(node_woof, "%s/%s", node_woof, DHT_FIND_SUCCESSOR_WOOF);
    }
    unsigned long seq = WooFPut(node_woof, NULL, &arg);
    if (WooFInvalid(seq)) {
        sprintf(dht_error_msg, "failed to invoke find_successor on %s", node_woof);
        return -1;
    }

    return 0;
}

// int is_blocked(char* target, char* self, BLOCKED_NODES* blocked_nodes) {
//     int i;
//     for (i = 0; i < 64; ++i) {
//         if (blocked_nodes->blocked_nodes[i][0] == 0) {
//             break;
//         }
//         if (strncmp(target, blocked_nodes->blocked_nodes[i], strlen(blocked_nodes->blocked_nodes[i])) == 0) {
//             if (rand() % 100 < blocked_nodes->failure_rate[i]) {
//                 usleep(blocked_nodes->timeout[i] * 1000);
//                 return -1;
//             }
//         }
//         if (strncmp(self, blocked_nodes->blocked_nodes[i], strlen(blocked_nodes->blocked_nodes[i])) == 0) {
//             if (rand() % 100 < blocked_nodes->failure_rate[i]) {
//                 return -2;
//             }
//         }
//     }
//     return 0;
// }

// int checkedWooFGet(BLOCKED_NODES* blocked_nodes, char* self, char* woof, void* ptr, unsigned long seq) {
//     int err = is_blocked(woof, self, blocked_nodes);
//     if (err < 0) {
//         return err;
//     }
//     return WooFGet(woof, ptr, seq);
// }

// unsigned long checkedWooFGetLatestSeq(BLOCKED_NODES* blocked_nodes, char* self, char* woof) {
//     int err = is_blocked(woof, self, blocked_nodes);
//     if (err < 0) {
//         return err;
//     }
//     return WooFGetLatestSeqno(woof);
// }

// unsigned long checkedWooFPut(BLOCKED_NODES* blocked_nodes, char* self, char* woof, char* handler, void* ptr) {
//     int err = is_blocked(woof, self, blocked_nodes);
//     if (err < 0) {
//         return err;
//     }
//     return WooFPut(woof, handler, ptr);
// }

// unsigned long checkedMonitorRemotePut(BLOCKED_NODES* blocked_nodes,
//                                       char* self,
//                                       char* callback_monitor,
//                                       char* callback_woof,
//                                       char* callback_handler,
//                                       void* ptr,
//                                       int idempotent) {
//     int err = is_blocked(callback_woof, self, blocked_nodes);
//     if (err < 0) {
//         return err;
//     }
//     return monitor_remote_put(callback_monitor, callback_woof, callback_handler, ptr, idempotent);
// }

// unsigned long checked_raft_sessionless_put_handler(BLOCKED_NODES* blocked_nodes,
//                                                    char* self,
//                                                    char* replica,
//                                                    char* handler,
//                                                    void* ptr,
//                                                    unsigned long size,
//                                                    int monitored,
//                                                    int timeout) {
//     int err = is_blocked(replica, self, blocked_nodes);
//     if (err < 0) {
//         return err;
//     }
//     return raft_sessionless_put_handler(replica, handler, ptr, size, monitored, timeout);
// }