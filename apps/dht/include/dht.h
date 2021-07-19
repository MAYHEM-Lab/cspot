#ifdef __cplusplus
extern "C" {
#endif

#ifndef DHT_H
#define DHT_H

#include "raft.h"
#include "woofc.h"

#include <openssl/sha.h>
#include <stdint.h>

#define DHT_CHECK_PREDECESSOR_WOOF "dht_check_predecessor.woof"
#define DHT_DAEMON_WOOF "dht_daemon.woof"
#define DHT_FIND_NODE_RESULT_WOOF "dht_find_node_result.woof"
#define DHT_FIND_SUCCESSOR_ROUTINE_WOOF "dht_find_successor_routine.woof"
#define DHT_FIND_SUCCESSOR_WOOF "dht_find_successor.woof"
#define DHT_FIX_FINGER_WOOF "dht_fix_finger.woof"
#define DHT_FIX_FINGER_CALLBACK_WOOF "dht_fix_finger_callback.woof"
#define DHT_GET_PREDECESSOR_WOOF "dht_get_predecessor.woof"
#define DHT_INVALIDATE_FINGERS_WOOF "dht_invalidate_fingers.woof"
#define DHT_INVOCATION_WOOF "dht_invocation.woof"
#define DHT_JOIN_WOOF "dht_join.woof"
#define DHT_NOTIFY_CALLBACK_WOOF "dht_notify_callback.woof"
#define DHT_NOTIFY_WOOF "dht_notify.woof"
#define DHT_REGISTER_TOPIC_WOOF "dht_register_topic.woof"
#define DHT_SHIFT_SUCCESSOR_WOOF "dht_shift_successor.woof"
#define DHT_STABILIZE_WOOF "dht_stabilize.woof"
#define DHT_STABILIZE_CALLBACK_WOOF "dht_stabilize_callback.woof"
#define DHT_SUBSCRIBE_WOOF "dht_subscribe.woof"
#define DHT_TRIGGER_WOOF "dht_trigger.woof"
#define DHT_NODE_INFO_WOOF "dht_node_info.woof"
#define DHT_PREDECESSOR_INFO_WOOF "dht_predecessor_info.woof"
#define DHT_SUCCESSOR_INFO_WOOF "dht_successor_info.woof"
#define DHT_FINGER_INFO_WOOF "dht_finger_info.woof"
#define DHT_TRY_REPLICAS_WOOF "dht_try_replicas.woof"
#define DHT_SUBSCRIPTION_LIST_WOOF "subscription_list.woof"
#define DHT_TOPIC_REGISTRATION_WOOF "topic_registaration.woof"
#define DHT_SERVER_PUBLISH_DATA_WOOF "dht_server_publish_data.woof"
#define DHT_SERVER_PUBLISH_TRIGGER_WOOF "dht_server_publish_trigger.woof"
#define DHT_SERVER_PUBLISH_ELEMENT_WOOF "dht_server_publish_element.woof"
#define DHT_SERVER_LOOP_ROUTINE_WOOF "dht_server_loop_routine.woof"
#define DHT_TOPIC_CACHE_WOOF "dht_topic_cache.woof"
#define DHT_REGISTRY_CACHE_WOOF "dht_registry_cache.woof"
#define DHT_INVALIDATE_CACHE_WOOF "dht_invalidate_cache.woof"
#define DHT_MONITOR_NAME "dht"

#define DHT_NAME_LENGTH WOOFNAMESIZE
#define DHT_CACHE_SIZE 32
#define DHT_HISTORY_LENGTH_SHORT 32
#define DHT_HISTORY_LENGTH_LONG 512
#define DHT_HISTORY_LENGTH_EXTRA_LONG 32768
#define DHT_MAX_SUBSCRIPTIONS 8
#define DHT_SUCCESSOR_LIST_R 3
#define DHT_REPLICA_NUMBER 5
#define DHT_REGISTER_TOPIC_REPLICA 3 // can't be greater than DHT_SUCCESSOR_LIST_R

#define DHT_ACTION_NONE 0
#define DHT_ACTION_FIND_NODE 1
#define DHT_ACTION_JOIN 2
#define DHT_ACTION_FIX_FINGER 3
#define DHT_ACTION_REGISTER_TOPIC 4
#define DHT_ACTION_SUBSCRIBE 5
#define DHT_ACTION_PUBLISH 6

extern char dht_error_msg[256];

typedef struct dht_node_info {
    char name[DHT_NAME_LENGTH];
    unsigned char hash[SHA_DIGEST_LENGTH];
    char addr[DHT_NAME_LENGTH];
    char replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH];
    int32_t replica_id;
    int32_t leader_id;
} DHT_NODE_INFO;

typedef struct dht_predecessor_info {
    unsigned char hash[SHA_DIGEST_LENGTH];
    char replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH];
    int32_t leader;
} DHT_PREDECESSOR_INFO;

typedef struct dht_successor_info {
    unsigned char hash[DHT_SUCCESSOR_LIST_R][SHA_DIGEST_LENGTH];
    char replicas[DHT_SUCCESSOR_LIST_R][DHT_REPLICA_NUMBER][DHT_NAME_LENGTH];
    int32_t leader[DHT_SUCCESSOR_LIST_R];
} DHT_SUCCESSOR_INFO;

typedef struct dht_finger_info {
    unsigned char hash[SHA_DIGEST_LENGTH];
    char replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH];
    int32_t leader;
} DHT_FINGER_INFO;

typedef struct dht_find_successor_arg {
    char key[DHT_NAME_LENGTH];
    unsigned char hashed_key[SHA_DIGEST_LENGTH];
    int32_t action;
    char action_namespace[DHT_NAME_LENGTH]; // if action == DHT_ACTION_JOIN, this serves as serialized dht_config
    uint64_t action_seqno;                  // if action == DHT_ACTION_FIX_FINGER, this serves as finger_index
                           // if action == DHT_ACTION_PUBLISH_FIND, this servers as publish request seqno
    char callback_namespace[DHT_NAME_LENGTH];
    uint64_t ts_created;
    uint64_t ts_found;
} DHT_FIND_SUCCESSOR_ARG;

typedef struct dht_find_node_result {
    char topic[DHT_NAME_LENGTH];
    unsigned char node_hash[SHA_DIGEST_LENGTH];
    char node_replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH];
    int32_t node_leader;
} DHT_FIND_NODE_RESULT;

typedef struct dht_join_arg {
    unsigned char node_hash[SHA_DIGEST_LENGTH];
    char node_replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH];
    int32_t node_leader;
    unsigned char replier_hash[SHA_DIGEST_LENGTH];
    char replier_replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH];
    int32_t replier_leader;
    char serialized_config[DHT_NAME_LENGTH];
} DHT_JOIN_ARG;

typedef struct dht_fix_finger_arg {

} DHT_FIX_FINGER_ARG;

typedef struct dht_fix_finger_callback_arg {
    int32_t finger_index;
    unsigned char node_hash[SHA_DIGEST_LENGTH];
    char node_replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH];
    int32_t node_leader;
} DHT_FIX_FINGER_CALLBACK_ARG;

typedef struct dht_get_predecessor_arg {
    char callback_woof[DHT_NAME_LENGTH];
    char callback_handler[DHT_NAME_LENGTH];
} DHT_GET_PREDECESSOR_ARG;

typedef struct dht_stabilize_callback_arg {
    unsigned char predecessor_hash[SHA_DIGEST_LENGTH];
    char predecessor_replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH];
    int32_t predecessor_leader;
    int32_t successor_leader_id;
} DHT_STABILIZE_CALLBACK_ARG;

typedef struct dht_notify_arg {
    unsigned char node_hash[SHA_DIGEST_LENGTH];
    char node_replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH];
    int32_t node_leader;
    char callback_woof[DHT_NAME_LENGTH];
    char callback_handler[DHT_NAME_LENGTH];
} DHT_NOTIFY_ARG;

typedef struct dht_notify_callback_arg {
    unsigned char successor_hash[DHT_SUCCESSOR_LIST_R][SHA_DIGEST_LENGTH];
    char successor_replicas[DHT_SUCCESSOR_LIST_R][DHT_REPLICA_NUMBER][DHT_NAME_LENGTH];
    int32_t successor_leader[DHT_SUCCESSOR_LIST_R];
} DHT_NOTIFY_CALLBACK_ARG;

typedef struct dht_register_topic_arg {
    char topic_name[DHT_NAME_LENGTH];
    char topic_namespace[DHT_NAME_LENGTH];
    char topic_replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH];
    int topic_leader;
} DHT_REGISTER_TOPIC_ARG;

typedef struct dht_create_index_map_arg {
    char topic_name[DHT_NAME_LENGTH];
} DHT_CREATE_INDEX_MAP_ARG;

typedef struct dht_shift_successor_arg {

} DHT_SHIFT_SUCCESSOR_ARG;

typedef struct dht_subscription_list {
    int32_t size;
    char handlers[DHT_MAX_SUBSCRIPTIONS][DHT_NAME_LENGTH];
    char replica_namespaces[DHT_MAX_SUBSCRIPTIONS][DHT_REPLICA_NUMBER][DHT_NAME_LENGTH];
    int32_t last_successful_replica[DHT_MAX_SUBSCRIPTIONS];
} DHT_SUBSCRIPTION_LIST;

typedef struct dht_subscribe_arg {
    char topic_name[DHT_NAME_LENGTH];
    char handler[DHT_NAME_LENGTH];
    char replica_namespaces[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH];
    int32_t replica_leader;
} DHT_SUBSCRIBE_ARG;

typedef struct dht_trigger_arg {
    char topic_name[DHT_NAME_LENGTH];
    char element_woof[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH];
    int32_t leader_id;
    uint64_t element_seqno;
    char subscription_woof[DHT_NAME_LENGTH];
    uint64_t ts_created;
    uint64_t ts_found;
    uint64_t ts_returned;
    uint64_t ts_registry;
} DHT_TRIGGER_ARG;

typedef struct dht_topic_registry {
    char topic_name[DHT_NAME_LENGTH];
    char topic_replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH];
    int32_t last_leader;
} DHT_TOPIC_REGISTRY;

typedef struct dht_daemon_arg {
    uint64_t last_stabilize;
    uint64_t last_check_predecessor;
    uint64_t last_fix_finger;
    uint64_t last_update_leader_id;
    uint64_t last_replicate_state;
    int32_t stabilize_freq;
    int32_t chk_predecessor_freq;
    int32_t fix_finger_freq;
    int32_t update_leader_freq;
    int32_t daemon_wakeup_freq;
} DHT_DAEMON_ARG;

typedef struct dht_stabilize_arg {

} DHT_STABILIZE_ARG;

typedef struct dht_check_predecessor_arg {

} DHT_CHECK_PREDECESSOR_ARG;

typedef struct dht_set_finger_arg {
    int32_t finger_index;
    DHT_FINGER_INFO finger;
} DHT_SET_FINGER_ARG;

typedef struct dht_try_replicas_arg {
} DHT_TRY_REPLICAS_ARG;

typedef struct dht_invalidate_fingers_arg {
    unsigned char finger_hash[SHA_DIGEST_LENGTH];
} DHT_INVALIDATE_FINGERS_ARG;

typedef struct dht_server_publish_data_arg {
    char topic_name[DHT_NAME_LENGTH];
    char node_replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH];
    int32_t update_cache;
    int32_t node_leader;
    uint64_t element_seqno;
    uint64_t ts_created;
    uint64_t ts_found;
} DHT_SERVER_PUBLISH_DATA_ARG;

typedef struct dht_loop_routine_arg {
    uint64_t last_seqno;
} DHT_LOOP_ROUTINE_ARG;

typedef struct dht_topic_cache {
    char topic_name[DHT_NAME_LENGTH];
    char node_replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH];
    int32_t node_leader;
} DHT_TOPIC_CACHE;

typedef struct dht_registry_cache {
    char topic_name[DHT_NAME_LENGTH];
    DHT_TOPIC_REGISTRY registry;
    char registry_woof[DHT_NAME_LENGTH];
    int8_t invalidated;
} DHT_REGISTRY_CACHE;

typedef struct dht_invalidate_cache_arg {
    char topic_name[DHT_NAME_LENGTH];
} DHT_INVALIDATE_CACHE_ARG;

typedef RAFT_CLIENT_PUT_RESULT DHT_SERVER_PUBLISH_TRIGGER_ARG;

int dht_create_woofs();
int dht_start_app_server();
int dht_start_daemon(
    int stabilize_freq, int chk_predecessor_freq, int fix_finger_freq, int update_leader_freq, int daemon_wakeup_freq);
int dht_create_cluster(char* woof_name,
                       char* node_name,
                       char replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH],
                       int stabilize_freq,
                       int chk_predecessor_freq,
                       int fix_finger_freq,
                       int update_leader_freq,
                       int daemon_wakeup_freq);
int dht_join_cluster(char* node_woof,
                     char* woof_name,
                     char* node_name,
                     char replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH],
                     int rejoin,
                     int stabilize_freq,
                     int chk_predecessor_freq,
                     int fix_finger_freq,
                     int update_leader_freq,
                     int daemon_wakeup_freq);

int dht_cache_node_get(char* topic_name, DHT_TOPIC_CACHE* result);
int dht_cache_node_put(char* topic_name, int node_leader, char node_replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH]);
int dht_cache_node_invalidate(char* topic_name);
int dht_cache_registry_get(char* topic_name, DHT_REGISTRY_CACHE* result);
int dht_cache_registry_put(char* topic_name, DHT_TOPIC_REGISTRY* registry, char* registry_woof);
int dht_cache_registry_invalidate(char* topic_name);
#endif

#ifdef __cplusplus
}
#endif
