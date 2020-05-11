#ifndef DHT_H
#define DHT_H

#include <stdint.h>
#include <openssl/sha.h>
#include "woofc.h"

#define DHT_NAME_LENGTH WOOFNAMESIZE
#define DHT_HISTORY_LENGTH_LONG 256
#define DHT_HISTORY_LENGTH_SHORT 4
#define DHT_TABLE_WOOF "dht_table"
#define DHT_FIND_ADDRESS_RESULT_WOOF "dht_find_address_result.woof"
#define DHT_FIND_NODE_RESULT_WOOF "dht_find_node_result.woof"
#define DHT_FIND_SUCCESSOR_WOOF "dht_find_successor.woof"
#define DHT_FIX_FINGER_WOOF "dht_fix_finger.woof"
#define DHT_GET_PREDECESSOR_WOOF "dht_get_predecessor.woof"
#define DHT_INVOCATION_WOOF "dht_invocation.woof"
#define DHT_JOIN_WOOF "dht_join.woof"
#define DHT_NOTIFY_CALLBACK_WOOF "dht_notify_callback.woof"
#define DHT_NOTIFY_WOOF "dht_notify.woof"
#define DHT_REGISTER_TOPIC_WOOF "dht_register_topic.woof"
#define DHT_STABLIZE_CALLBACK_WOOF "dht_stablize_callback.woof"
#define DHT_SUBSCRIBE_WOOF "dht_subscribe.woof"
#define DHT_TRIGGER_WOOF "dht_trigger.woof"
#define DHT_SUBSCRIPTION_LIST_WOOF "subscription_list.woof"
#define DHT_TOPIC_REGISTRATION_WOOF "topic_registaration.woof"

#define DHT_SUCCESSOR_LIST_R 3
#define DHT_MAX_SUBSCRIPTIONS 32

#define DHT_ACTION_NONE 0
#define DHT_ACTION_FIND_ADDRESS 1
#define DHT_ACTION_FIND_NODE 2
#define DHT_ACTION_JOIN 3
#define DHT_ACTION_FIX_FINGER 4
#define DHT_ACTION_REGISTER_TOPIC 5
#define DHT_ACTION_SUBSCRIBE 6
#define DHT_ACTION_TRIGGER 7

#define DHT_LOG_DEBUG 0
#define DHT_LOG_INFO 1
#define DHT_LOG_WARN 2
#define DHT_LOG_ERROR 3

typedef struct dht_table {
	unsigned char node_hash[SHA_DIGEST_LENGTH];
	char node_addr[DHT_NAME_LENGTH];
	char finger_addr[SHA_DIGEST_LENGTH * 8 + 1][DHT_NAME_LENGTH];
	unsigned char finger_hash[SHA_DIGEST_LENGTH * 8 + 1][SHA_DIGEST_LENGTH];
	char successor_addr[DHT_SUCCESSOR_LIST_R][DHT_NAME_LENGTH];
	unsigned char successor_hash[DHT_SUCCESSOR_LIST_R][SHA_DIGEST_LENGTH];
	char predecessor_addr[DHT_NAME_LENGTH];
	unsigned char predecessor_hash[SHA_DIGEST_LENGTH];
} DHT_TABLE;

typedef struct dht_find_successor_arg {
	int hops;
	char key[DHT_NAME_LENGTH];
	unsigned char hashed_key[SHA_DIGEST_LENGTH];
	int action;
	char action_namespace[DHT_NAME_LENGTH];
	unsigned long action_seqno; // if action == DHT_ACTION_FIX_FINGER, this serves as finger_index
	char callback_namespace[DHT_NAME_LENGTH];
} DHT_FIND_SUCCESSOR_ARG;

typedef struct dht_find_address_result {
	char topic[DHT_NAME_LENGTH];
	int hops;
	unsigned char topic_addr[DHT_NAME_LENGTH];
} DHT_FIND_ADDRESS_RESULT;

typedef struct dht_find_node_result {
	char topic[DHT_NAME_LENGTH];
	int hops;
	unsigned char node_hash[SHA_DIGEST_LENGTH];
	unsigned char node_addr[DHT_NAME_LENGTH];
} DHT_FIND_NODE_RESULT;

typedef struct dht_invocation_arg {
	char woof_name[DHT_NAME_LENGTH];
	unsigned long seqno;
} DHT_INVOCATION_ARG;

typedef struct dht_join_arg {
	unsigned char node_hash[SHA_DIGEST_LENGTH];
	unsigned char node_addr[DHT_NAME_LENGTH];
} DHT_JOIN_ARG;

typedef struct dht_fix_finger_arg {
	int finger_index;
	unsigned char node_hash[SHA_DIGEST_LENGTH];
	unsigned char node_addr[DHT_NAME_LENGTH];
} DHT_FIX_FINGER_ARG;

typedef struct dht_get_predecessor_arg {
	char callback_woof[DHT_NAME_LENGTH];
	char callback_handler[DHT_NAME_LENGTH];
} GET_PREDECESSOR_ARG;

typedef struct DHT_STABLIZE_CALLBACK {
	unsigned char predecessor_hash[SHA_DIGEST_LENGTH];
	unsigned char predecessor_addr[DHT_NAME_LENGTH];
} DHT_STABLIZE_CALLBACK;

typedef struct dht_notify_arg {
	unsigned char node_hash[SHA_DIGEST_LENGTH];
	unsigned char node_addr[DHT_NAME_LENGTH];
	char callback_woof[DHT_NAME_LENGTH];
	char callback_handler[DHT_NAME_LENGTH];
} NOTIFY_ARG;

typedef struct dht_notify_result {
	char successor_addr[DHT_SUCCESSOR_LIST_R][DHT_NAME_LENGTH];
	unsigned char successor_hash[DHT_SUCCESSOR_LIST_R][SHA_DIGEST_LENGTH];
} NOTIFY_RESULT;

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
	char woof_name[DHT_NAME_LENGTH];
	unsigned long seqno;
} DHT_TRIGGER_ARG;

typedef struct dht_topic_entry {
	char topic_name[DHT_NAME_LENGTH];
	char topic_namespace[DHT_NAME_LENGTH];
} DHT_TOPIC_ENTRY;

void dht_init(unsigned char *node_hash, char *node_addr, DHT_TABLE *dht_table);
void dht_init_find_arg(DHT_FIND_SUCCESSOR_ARG *arg, char *key, char *hashed_key, char *callback_namespace);
unsigned long get_milliseconds();
int node_woof_name(char *woof_name);
void print_node_hash(char *dst, const unsigned char *id_hash);
int in_range(unsigned char *n, unsigned char *lower, unsigned char *upper);
void shift_successor_list(char successor_addr[DHT_SUCCESSOR_LIST_R][DHT_NAME_LENGTH], unsigned char successor_hash[DHT_SUCCESSOR_LIST_R][SHA_DIGEST_LENGTH]);

char log_tag[DHT_NAME_LENGTH];
void log_set_level(int level);
void log_set_output(FILE *file);
void log_set_tag(const char *tag);
void log_debug(const char *message, ...);
void log_info(const char *message, ...);
void log_warn(const char *message, ...);
void log_error(const char *message, ...);

#endif
