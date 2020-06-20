#ifndef DHT_H
#define DHT_H

#include "woofc.h"

#include <openssl/sha.h>
#include <stdint.h>

#define DHT_CHECK_PREDECESSOR_WOOF "dht_check_predecessor.woof"
#define DHT_DAEMON_WOOF "dht_daemon.woof"
#define DHT_FIND_NODE_RESULT_WOOF "dht_find_node_result.woof"
#define DHT_FIND_SUCCESSOR_WOOF "dht_find_successor.woof"
#define DHT_FIX_FINGER_WOOF "dht_fix_finger.woof"
#define DHT_FIX_FINGER_CALLBACK_WOOF "dht_fix_finger_callback.woof"
#define DHT_GET_PREDECESSOR_WOOF "dht_get_predecessor.woof"
#define DHT_INVOCATION_WOOF "dht_invocation.woof"
#define DHT_JOIN_WOOF "dht_join.woof"
#define DHT_NOTIFY_CALLBACK_WOOF "dht_notify_callback.woof"
#define DHT_NOTIFY_WOOF "dht_notify.woof"
#define DHT_REGISTER_TOPIC_WOOF "dht_register_topic.woof"
#define DHT_STABILIZE_WOOF "dht_stabilize.woof"
#define DHT_STABILIZE_CALLBACK_WOOF "dht_stabilize_callback.woof"
#define DHT_SUBSCRIBE_WOOF "dht_subscribe.woof"
#define DHT_TRIGGER_WOOF "dht_trigger.woof"
#define DHT_NODE_INFO_WOOF "dht_node_info.woof"
#define DHT_PREDECESSOR_INFO_WOOF "dht_predecessor_info.woof"
#define DHT_SUCCESSOR_INFO_WOOF "dht_successor_info.woof"
#define DHT_FINGER_INFO_WOOF "dht_finger_info.woof"
#define DHT_REPLICATE_STATE_WOOF "dht_replicate_state.woof"
#define DHT_TRY_REPLICAS_WOOF "dht_try_replicas.woof"
#define DHT_SUBSCRIPTION_LIST_WOOF "subscription_list.woof"
#define DHT_TOPIC_REGISTRATION_WOOF "topic_registaration.woof"

#define DHT_NAME_LENGTH WOOFNAMESIZE
#define DHT_HISTORY_LENGTH_LONG 256
#define DHT_HISTORY_LENGTH_SHORT 4
#define DHT_MAX_SUBSCRIPTIONS 8
#define DHT_STABILIZE_FREQUENCY 1000
#define DHT_CHECK_PREDECESSOR_FRQUENCY 1000
#define DHT_FIX_FINGER_FRQUENCY 100
#define DHT_SUCCESSOR_LIST_R 3
#define DHT_REPLICA_NUMBER 3
#define DHT_REPLICATE_STATE_FREQUENCY 5000

#define DHT_ACTION_NONE 0
#define DHT_ACTION_FIND_NODE 1
#define DHT_ACTION_JOIN 2
#define DHT_ACTION_FIX_FINGER 3
#define DHT_ACTION_REGISTER_TOPIC 4
#define DHT_ACTION_SUBSCRIBE 5

#define DHT_TRY_PREDECESSOR 0
#define DHT_TRY_SUCCESSOR 1
#define DHT_TRY_FINGER 2

char dht_error_msg[256];

typedef struct dht_node_info {
    unsigned char hash[SHA_DIGEST_LENGTH];
    char addr[DHT_NAME_LENGTH];
    char replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH];
    int replica_id;
    int leader_id;
} DHT_NODE_INFO;

typedef struct dht_predecessor_info {
    unsigned char hash[SHA_DIGEST_LENGTH];
    char replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH];
    int leader;
} DHT_PREDECESSOR_INFO;

typedef struct dht_successor_info {
    unsigned char hash[DHT_SUCCESSOR_LIST_R][SHA_DIGEST_LENGTH];
    char replicas[DHT_SUCCESSOR_LIST_R][DHT_REPLICA_NUMBER][DHT_NAME_LENGTH];
    int leader[DHT_SUCCESSOR_LIST_R];
} DHT_SUCCESSOR_INFO;

typedef struct dht_finger_info {
    unsigned char hash[SHA_DIGEST_LENGTH];
    char replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH];
    int leader;
} DHT_FINGER_INFO;

typedef struct dht_find_successor_arg {
    int hops;
    char key[DHT_NAME_LENGTH];
    unsigned char hashed_key[SHA_DIGEST_LENGTH];
    int action;
    char action_namespace[DHT_NAME_LENGTH];
    unsigned long action_seqno; // if action == DHT_ACTION_FIX_FINGER, this serves as finger_index
    char callback_namespace[DHT_NAME_LENGTH];
} DHT_FIND_SUCCESSOR_ARG;

typedef struct dht_find_node_result {
    char topic[DHT_NAME_LENGTH];
    int hops;
    unsigned char node_hash[SHA_DIGEST_LENGTH];
    char node_replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH];
    int node_leader;
} DHT_FIND_NODE_RESULT;

typedef struct dht_invocation_arg {
    char woof_name[DHT_NAME_LENGTH];
    unsigned long seq_no;
} DHT_INVOCATION_ARG;

typedef struct dht_join_arg {
    unsigned char node_hash[SHA_DIGEST_LENGTH];
    char node_replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH];
    int node_leader;
} DHT_JOIN_ARG;

typedef struct dht_fix_finger_arg {
    int finger_index;
} DHT_FIX_FINGER_ARG;

typedef struct dht_fix_finger_callback_arg {
    int finger_index;
    unsigned char node_hash[SHA_DIGEST_LENGTH];
    char node_replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH];
    int node_leader;
} DHT_FIX_FINGER_CALLBACK_ARG;

typedef struct dht_get_predecessor_arg {
    char callback_woof[DHT_NAME_LENGTH];
    char callback_handler[DHT_NAME_LENGTH];
} DHT_GET_PREDECESSOR_ARG;

typedef struct dht_stabilize_callback_arg {
    unsigned char predecessor_hash[SHA_DIGEST_LENGTH];
    char predecessor_replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH];
    int predecessor_leader;
} DHT_STABILIZE_CALLBACK_ARG;

typedef struct dht_notify_arg {
    unsigned char node_hash[SHA_DIGEST_LENGTH];
    char node_replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH];
    int node_leader;
    char callback_woof[DHT_NAME_LENGTH];
    char callback_handler[DHT_NAME_LENGTH];
} DHT_NOTIFY_ARG;

typedef struct dht_notify_callback_arg {
    unsigned char successor_hash[DHT_SUCCESSOR_LIST_R][SHA_DIGEST_LENGTH];
    char successor_replicas[DHT_SUCCESSOR_LIST_R][DHT_REPLICA_NUMBER][DHT_NAME_LENGTH];
    int successor_leader[DHT_SUCCESSOR_LIST_R];
} DHT_NOTIFY_CALLBACK_ARG;

typedef struct dht_register_topic_arg {
    char topic_name[DHT_NAME_LENGTH];
    char topic_namespace[DHT_NAME_LENGTH];
} DHT_REGISTER_TOPIC_ARG;

typedef struct dht_subscription_list {
    int size;
    char handlers[DHT_MAX_SUBSCRIPTIONS][DHT_NAME_LENGTH];
    char namespace[DHT_MAX_SUBSCRIPTIONS][DHT_NAME_LENGTH];
} DHT_SUBSCRIPTION_LIST;

typedef struct dht_subscribe_arg {
    char topic_name[DHT_NAME_LENGTH];
    char handler[DHT_NAME_LENGTH];
    char handler_namespace[DHT_NAME_LENGTH];
} DHT_SUBSCRIBE_ARG;

typedef struct dht_trigger_arg {
    char topic_name[DHT_NAME_LENGTH];
    char element_woof[DHT_NAME_LENGTH];
    unsigned long element_seqno;
    char subscription_woof[DHT_NAME_LENGTH];
} DHT_TRIGGER_ARG;

typedef struct dht_topic_registry {
    char topic_name[DHT_NAME_LENGTH];
    char topic_namespace[DHT_NAME_LENGTH];
} DHT_TOPIC_REGISTRY;

typedef struct dht_daemon_arg {
    unsigned long last_stabilize;
    unsigned long last_check_predecessor;
    unsigned long last_fix_finger;
#ifdef USE_RAFT
    unsigned long last_replicate_state;
#endif
    int last_fixed_finger_index;
} DHT_DAEMON_ARG;

typedef struct dht_stabilize_arg {

} DHT_STABILIZE_ARG;

typedef struct dht_check_predecessor_arg {

} DHT_CHECK_PREDECESSOR_ARG;

typedef struct dht_set_finger_arg {
    int finger_index;
    DHT_FINGER_INFO finger;
} DHT_SET_FINGER_ARG;

typedef struct dht_replicate_state_arg {

} DHT_REPLICATE_STATE_ARG;

typedef struct dht_try_replicas_arg {
    int type;
    int finger_id;
    char woof_name[DHT_NAME_LENGTH];
    char handler_name[DHT_NAME_LENGTH];
    unsigned long seq_no;
} DHT_TRY_REPLICAS_ARG;

int dht_create_woofs();
int dht_start_daemon();
int dht_create_cluster(char* woof_name, char* node_name, char replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH]);
int dht_join_cluster(char* node_woof,
                     char* woof_name,
                     char* node_name,
                     char replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH]);

#endif
