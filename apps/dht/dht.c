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
                                              DHT_TRIGGER_WOOF,
                                              DHT_NODE_INFO_WOOF,
                                              DHT_PREDECESSOR_INFO_WOOF,
                                              DHT_SUCCESSOR_INFO_WOOF,
                                              DHT_SERVER_PUBLISH_DATA_WOOF,
                                              DHT_SERVER_PUBLISH_TRIGGER_WOOF,
                                              DHT_SERVER_PUBLISH_ELEMENT_WOOF,
                                              DHT_SERVER_LOOP_ROUTINE_WOOF,
                                              DHT_TOPIC_CACHE_WOOF,
                                              DHT_REGISTRY_CACHE_WOOF,
                                              DHT_INVALIDATE_CACHE_WOOF,
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
                                         sizeof(DHT_TRIGGER_ARG),
                                         sizeof(DHT_JOIN_ARG),
                                         sizeof(DHT_NOTIFY_CALLBACK_ARG),
                                         sizeof(DHT_NOTIFY_ARG),
                                         sizeof(DHT_REGISTER_TOPIC_ARG),
                                         sizeof(DHT_SHIFT_SUCCESSOR_ARG),
                                         sizeof(DHT_STABILIZE_ARG),
                                         sizeof(DHT_STABILIZE_CALLBACK_ARG),
                                         sizeof(DHT_SUBSCRIBE_ARG),
                                         sizeof(DHT_TRIGGER_ARG),
                                         sizeof(DHT_NODE_INFO),
                                         sizeof(DHT_PREDECESSOR_INFO),
                                         sizeof(DHT_SUCCESSOR_INFO),
                                         sizeof(DHT_SERVER_PUBLISH_DATA_ARG),
                                         sizeof(DHT_SERVER_PUBLISH_TRIGGER_ARG),
                                         sizeof(RAFT_DATA_TYPE),
                                         sizeof(DHT_LOOP_ROUTINE_ARG),
                                         sizeof(DHT_TOPIC_CACHE),
                                         sizeof(DHT_REGISTRY_CACHE),
                                         sizeof(DHT_INVALIDATE_CACHE_ARG),
                                         sizeof(DHT_TRY_REPLICAS_WOOF)};

unsigned long DHT_ELEMENT_SIZE[] = {
    DHT_HISTORY_LENGTH_SHORT,      // DHT_CHECK_PREDECESSOR_WOOF,
    DHT_HISTORY_LENGTH_SHORT,      // DHT_DAEMON_WOOF,
    DHT_HISTORY_LENGTH_LONG,       // DHT_FIND_NODE_RESULT_WOOF,
    DHT_HISTORY_LENGTH_SHORT,      // DHT_FIND_SUCCESSOR_ROUTINE_WOOF,
    DHT_HISTORY_LENGTH_EXTRA_LONG, // DHT_FIND_SUCCESSOR_WOOF,
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
    DHT_HISTORY_LENGTH_EXTRA_LONG, // DHT_TRIGGER_WOOF,
    DHT_HISTORY_LENGTH_SHORT,      // DHT_NODE_INFO_WOOF,
    DHT_HISTORY_LENGTH_SHORT,      // DHT_PREDECESSOR_INFO_WOOF,
    DHT_HISTORY_LENGTH_SHORT,      // DHT_SUCCESSOR_INFO_WOOF,
    DHT_HISTORY_LENGTH_EXTRA_LONG, // DHT_SERVER_PUBLISH_DATA_WOOF
    DHT_HISTORY_LENGTH_EXTRA_LONG, // DHT_SERVER_PUBLISH_TRIGGER_ARG
    DHT_HISTORY_LENGTH_EXTRA_LONG, // DHT_SERVER_PUBLISH_ELEMENT_WOOF
    DHT_HISTORY_LENGTH_LONG,       // DHT_SERVER_LOOP_ROUTINE_WOOF,
    DHT_CACHE_SIZE,                // DHT_TOPIC_CACHE_WOOF,
    DHT_CACHE_SIZE,                // DHT_REGISTRY_CACHE_WOOF,
    DHT_HISTORY_LENGTH_SHORT,      // DHT_INVALIDATE_CACHE_WOOF
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
        sprintf(finger_woof_name, "%s_%d", DHT_FINGER_INFO_WOOF, i);
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
    unsigned long seq = WooFPut(DHT_SERVER_LOOP_ROUTINE_WOOF, "server_publish_data", &routine_arg);
    if (WooFInvalid(seq)) {
        fprintf(stderr, "failed to start server_publish_data\n");
        return -1;
    }
    // seq = WooFPut(DHT_SERVER_LOOP_ROUTINE_WOOF, "server_publish_trigger", &routine_arg);
    // if (WooFInvalid(seq)) {
    //     fprintf(stderr, "failed to start server_publish_trigger\n");
    //     return -1;
    // }
}

int dht_start_daemon(
    int stabilize_freq, int chk_predecessor_freq, int fix_finger_freq, int update_leader_freq, int daemon_wakeup_freq) {
    DHT_DAEMON_ARG arg = {0};
    arg.last_stabilize = 0;
    arg.last_check_predecessor = 0;
    arg.last_fix_finger = 0;
    arg.last_update_leader_id = 0;
    arg.last_replicate_state = 0;
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

int dht_cache_node_get(char* topic_name, DHT_TOPIC_CACHE* result) {
    unsigned long latest_seqno = WooFGetLatestSeqno(DHT_TOPIC_CACHE_WOOF);
    if (WooFInvalid(latest_seqno)) {
        sprintf(dht_error_msg, "failed to get the latest seqno of topic cache");
        return -1;
    }
    unsigned long i = latest_seqno;
    while (i != 0) {
        if (i <= latest_seqno - DHT_CACHE_SIZE) {
            break;
        }
        if (WooFGet(DHT_TOPIC_CACHE_WOOF, result, i) < 0) {
            sprintf(dht_error_msg, "failed to get the topic cache at %lu", i);
            return -1;
        }
        if (strcmp(result->topic_name, topic_name) == 0) {
            if (result->node_leader == -1) { // cache invalidated
                break;
            }
            return 1;
        }
        --i;
    }
    return 0;
}

int dht_cache_node_put(char* topic_name, int node_leader, char node_replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH]) {
    DHT_TOPIC_CACHE cache = {0};
    strcpy(cache.topic_name, topic_name);
    cache.node_leader = node_leader;
    memcpy(cache.node_replicas, node_replicas, sizeof(cache.node_replicas));
    unsigned long seq = WooFPut(DHT_TOPIC_CACHE_WOOF, NULL, &cache);
    if (WooFInvalid(seq)) {
        sprintf(dht_error_msg, "failed to put to %s", DHT_TOPIC_CACHE_WOOF);
        return -1;
    }
    return 0;
}

int dht_cache_node_invalidate(char* topic_name) {
    DHT_TOPIC_CACHE cache = {0};
    strcpy(cache.topic_name, topic_name);
    cache.node_leader = -1;
    unsigned long seq = WooFPut(DHT_TOPIC_CACHE_WOOF, NULL, &cache);
    if (WooFInvalid(seq)) {
        sprintf(dht_error_msg, "failed to put to %s", DHT_TOPIC_CACHE_WOOF);
        return -1;
    }
    return 0;
}

int dht_cache_registry_get(char* topic_name, DHT_REGISTRY_CACHE* result) {
    unsigned long latest_seqno = WooFGetLatestSeqno(DHT_REGISTRY_CACHE_WOOF);
    if (WooFInvalid(latest_seqno)) {
        sprintf(dht_error_msg, "failed to get the latest seqno of topic cache");
        return -1;
    }
    unsigned long i = latest_seqno;
    while (i != 0) {
        if (i <= latest_seqno - DHT_CACHE_SIZE) {
            break;
        }
        if (WooFGet(DHT_REGISTRY_CACHE_WOOF, result, i) < 0) {
            sprintf(dht_error_msg, "failed to get the registry cache at %lu", i);
            return -1;
        }
        if (strcmp(result->topic_name, topic_name) == 0) {
            if (result->invalidated) { // cache invalidated
                break;
            }
            return 1;
        }
        --i;
    }
    return 0;
}

int dht_cache_registry_put(char* topic_name, DHT_TOPIC_REGISTRY* registry, char* registry_woof) {
    DHT_REGISTRY_CACHE cache = {0};
    strcpy(cache.topic_name, topic_name);
    memcpy(&cache.registry, registry, sizeof(cache.registry));
    strcpy(cache.registry_woof, registry_woof);
    cache.invalidated = 0;
    unsigned long seq = WooFPut(DHT_REGISTRY_CACHE_WOOF, NULL, &cache);
    if (WooFInvalid(seq)) {
        sprintf(dht_error_msg, "failed to put to %s", DHT_REGISTRY_CACHE_WOOF);
        return -1;
    }
    return 0;
}

int dht_cache_registry_invalidate(char* topic_name) {
    DHT_REGISTRY_CACHE cache = {0};
    strcpy(cache.topic_name, topic_name);
    cache.invalidated = 1;
    unsigned long seq = WooFPut(DHT_REGISTRY_CACHE_WOOF, NULL, &cache);
    if (WooFInvalid(seq)) {
        sprintf(dht_error_msg, "failed to put to %s", DHT_REGISTRY_CACHE_WOOF);
        return -1;
    }
    return 0;
}