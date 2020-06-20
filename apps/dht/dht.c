#include "dht.h"

#include "dht_utils.h"
#include "woofc.h"

#include <openssl/sha.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

char DHT_WOOF_TO_CREATE[][DHT_NAME_LENGTH] = {
    DHT_CHECK_PREDECESSOR_WOOF,
    DHT_DAEMON_WOOF,
    DHT_FIND_NODE_RESULT_WOOF,
    DHT_FIND_SUCCESSOR_WOOF,
    DHT_FIX_FINGER_WOOF,
    DHT_FIX_FINGER_CALLBACK_WOOF,
    DHT_GET_PREDECESSOR_WOOF,
    DHT_INVOCATION_WOOF,
    DHT_JOIN_WOOF,
    DHT_NOTIFY_CALLBACK_WOOF,
    DHT_NOTIFY_WOOF,
    DHT_REGISTER_TOPIC_WOOF,
    DHT_STABILIZE_WOOF,
    DHT_STABILIZE_CALLBACK_WOOF,
    DHT_SUBSCRIBE_WOOF,
    DHT_TRIGGER_WOOF,
    DHT_NODE_INFO_WOOF,
    DHT_PREDECESSOR_INFO_WOOF,
    DHT_SUCCESSOR_INFO_WOOF,
#ifdef USE_RAFT
    DHT_REPLICATE_STATE_WOOF,
    DHT_TRY_REPLICAS_WOOF,
#endif
};

unsigned long DHT_WOOF_ELEMENT_SIZE[] = {
    sizeof(DHT_CHECK_PREDECESSOR_ARG),
    sizeof(DHT_DAEMON_ARG),
    sizeof(DHT_FIND_NODE_RESULT),
    sizeof(DHT_FIND_SUCCESSOR_ARG),
    sizeof(DHT_FIX_FINGER_ARG),
    sizeof(DHT_FIX_FINGER_CALLBACK_ARG),
    sizeof(DHT_GET_PREDECESSOR_ARG),
    sizeof(DHT_INVOCATION_ARG),
    sizeof(DHT_JOIN_ARG),
    sizeof(DHT_NOTIFY_CALLBACK_ARG),
    sizeof(DHT_NOTIFY_ARG),
    sizeof(DHT_REGISTER_TOPIC_ARG),
    sizeof(DHT_STABILIZE_ARG),
    sizeof(DHT_STABILIZE_CALLBACK_ARG),
    sizeof(DHT_SUBSCRIBE_ARG),
    sizeof(DHT_TRIGGER_ARG),
    sizeof(DHT_NODE_INFO),
    sizeof(DHT_PREDECESSOR_INFO),
    sizeof(DHT_SUCCESSOR_INFO),
#ifdef USE_RAFT
    sizeof(DHT_REPLICATE_STATE_WOOF),
    sizeof(DHT_TRY_REPLICAS_WOOF),
#endif
};

unsigned long DHT_ELEMENT_SIZE[] = {
    DHT_HISTORY_LENGTH_SHORT, // DHT_CHECK_PREDECESSOR_WOOF,
    DHT_HISTORY_LENGTH_SHORT, // DHT_DAEMON_WOOF,
    DHT_HISTORY_LENGTH_LONG,  // DHT_FIND_NODE_RESULT_WOOF,
    DHT_HISTORY_LENGTH_LONG,  // DHT_FIND_SUCCESSOR_WOOF,
    DHT_HISTORY_LENGTH_LONG,  // DHT_FIX_FINGER_WOOF,
    DHT_HISTORY_LENGTH_LONG,  // DHT_FIX_FINGER_CALLBACK_WOOF,
    DHT_HISTORY_LENGTH_LONG,  // DHT_GET_PREDECESSOR_WOOF,
    DHT_HISTORY_LENGTH_LONG,  // DHT_INVOCATION_WOOF,
    DHT_HISTORY_LENGTH_SHORT, // DHT_JOIN_WOOF,
    DHT_HISTORY_LENGTH_LONG,  // DHT_NOTIFY_CALLBACK_WOOF,
    DHT_HISTORY_LENGTH_LONG,  // DHT_NOTIFY_WOOF,
    DHT_HISTORY_LENGTH_LONG,  // DHT_REGISTER_TOPIC_WOOF,
    DHT_HISTORY_LENGTH_SHORT, // DHT_STABILIZE_WOOF,
    DHT_HISTORY_LENGTH_SHORT, // DHT_STABILIZE_CALLBACK_WOOF,
    DHT_HISTORY_LENGTH_LONG,  // DHT_SUBSCRIBE_WOOF,
    DHT_HISTORY_LENGTH_LONG,  // DHT_TRIGGER_WOOF,
    DHT_HISTORY_LENGTH_SHORT, // DHT_NODE_INFO_WOOF,
    DHT_HISTORY_LENGTH_SHORT, // DHT_PREDECESSOR_INFO_WOOF,
    DHT_HISTORY_LENGTH_SHORT, // DHT_SUCCESSOR_INFO_WOOF,
#ifdef USE_RAFT
    DHT_HISTORY_LENGTH_SHORT, // DHT_REPLICATE_STATE_WOOF,
    DHT_HISTORY_LENGTH_SHORT, // DHT_TRY_REPLICAS_WOOF,
#endif
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
    char finger_woof_name[DHT_NAME_LENGTH];
    for (i = 1; i < SHA_DIGEST_LENGTH * 8 + 1; ++i) {
        sprintf(finger_woof_name, "%s%d", DHT_FINGER_INFO_WOOF, i);
        if (WooFCreate(finger_woof_name, sizeof(DHT_FINGER_INFO), DHT_HISTORY_LENGTH_LONG) < 0) {
            sprintf(dht_error_msg, "failed to create %s", finger_woof_name);
            return -1;
        }
    }
    return 0;
}

int dht_start_daemon() {
    DHT_DAEMON_ARG arg = {0};
    arg.last_stabilize = 0;
    arg.last_check_predecessor = 0;
    arg.last_fix_finger = 0;
#ifdef USE_RAFT
    arg.last_replicate_state = 0;
#endif
    arg.last_fixed_finger_index = 1;
    unsigned long seq = WooFPut(DHT_DAEMON_WOOF, "d_daemon", &arg);
    if (WooFInvalid(seq)) {
        return -1;
    }
    return 0;
}

int dht_create_cluster(char* woof_name, char* node_name, char replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH]) {
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

    if (dht_create_woofs() < 0) {
        sprintf(dht_error_msg, "can't create woofs");
        return -1;
    }

    unsigned char node_hash[SHA_DIGEST_LENGTH];
    dht_hash(node_hash, node_name);

    if (dht_init(node_hash, woof_name, replicas) < 0) {
        sprintf(dht_error_msg, "failed to initialize DHT: %s", dht_error_msg);
        return -1;
    }

    if (dht_start_daemon() < 0) {
        sprintf(dht_error_msg, "failed to start daemon");
        return -1;
    }
    char hash_str[DHT_NAME_LENGTH];
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
                     char replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH]) {
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

    if (dht_create_woofs() < 0) {
        sprintf(dht_error_msg, "can't create woofs");
        return -1;
    }

    unsigned char node_hash[SHA_DIGEST_LENGTH];
    dht_hash(node_hash, node_name);

    if (dht_init(node_hash, woof_name, replicas) < 0) {
        sprintf(dht_error_msg, "failed to initialize DHT: %s", dht_error_msg);
        return -1;
    }

    DHT_FIND_SUCCESSOR_ARG arg;
    dht_init_find_arg(&arg, woof_name, node_hash, woof_name);
    arg.action = DHT_ACTION_JOIN;
    if (node_woof[strlen(node_woof) - 1] == '/') {
        sprintf(node_woof, "%s%s", node_woof, DHT_FIND_SUCCESSOR_WOOF);
    } else {
        sprintf(node_woof, "%s/%s", node_woof, DHT_FIND_SUCCESSOR_WOOF);
    }
    unsigned long seq = WooFPut(node_woof, "h_find_successor", &arg);
    if (WooFInvalid(seq)) {
        sprintf(dht_error_msg, "failed to invoke find_successor on %s", node_woof);
        return -1;
    }

    return 0;
}
