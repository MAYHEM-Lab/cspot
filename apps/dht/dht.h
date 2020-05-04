#ifndef DHT_H
#define DHT_H

#include <stdint.h>
#include <openssl/sha.h>

#define DHT_NAME_LENGTH WOOFNAMESIZE
#define DHT_HISTORY_LENGTH 65536
#define DHT_TABLE_WOOF "dht_table"
#define DHT_FIND_SUCCESSOR_ARG_WOOF "dht_find_successor_arg"
#define DHT_FIND_SUCCESSOR_RESULT_WOOF "dht_find_successor_result"
#define DHT_GET_PREDECESSOR_ARG_WOOF "dht_get_predecessor_arg"
#define DHT_GET_PREDECESSOR_RESULT_WOOF "dht_get_predecessor_result"
#define DHT_NOTIFY_ARG_WOOF "dht_notify_arg"
#define DHT_NOTIFY_RESULT_WOOF "dht_notify_result"
#define DHT_INIT_TOPIC_ARG_WOOF "dht_init_topic_arg"
#define DHT_SUBSCRIPTION_ARG_WOOF "dht_subscription_arg"
#define DHT_SUBSCRIPTION_LIST_WOOF "dht_subscription_list"
#define DHT_TRIGGER_ARG_WOOF "dht_trigger_arg"
#define DHT_SUCCESSOR_LIST_R 3
#define DHT_SUCRIPTION_MAX_HANDLERS 32
#define LOG_DEBUG 0
#define LOG_INFO 1
#define LOG_WARN 2
#define LOG_ERROR 3

FILE *dht_log_output;
int dht_log_level;

typedef struct tbl_stc {
	unsigned char node_hash[SHA_DIGEST_LENGTH];
	char node_addr[DHT_NAME_LENGTH];
	char finger_addr[SHA_DIGEST_LENGTH * 8 + 1][DHT_NAME_LENGTH];
	unsigned char finger_hash[SHA_DIGEST_LENGTH * 8 + 1][SHA_DIGEST_LENGTH];
	char successor_addr[DHT_SUCCESSOR_LIST_R][DHT_NAME_LENGTH];
	unsigned char successor_hash[DHT_SUCCESSOR_LIST_R][SHA_DIGEST_LENGTH];
	char predecessor_addr[DHT_NAME_LENGTH];
	unsigned char predecessor_hash[SHA_DIGEST_LENGTH];
} DHT_TABLE_EL;

typedef struct find_successor_arg_stc {
	unsigned long request_seq_no; // the first resuest's seq_no
	int finger_index; // only used by fix_fingers
	int hops;
	unsigned char id_hash[SHA_DIGEST_LENGTH];
	char callback_woof[DHT_NAME_LENGTH];
	char callback_handler[DHT_NAME_LENGTH];
} FIND_SUCESSOR_ARG;

typedef struct find_result_stc {
	unsigned long request_seq_no; // the first resuest's seq_no
	int finger_index;
	int hops;
	unsigned char target_hash[SHA_DIGEST_LENGTH];
	unsigned char node_hash[SHA_DIGEST_LENGTH];
	unsigned char node_addr[DHT_NAME_LENGTH];
} FIND_SUCESSOR_RESULT;

typedef struct get_predecessor_arg_stc {
	char callback_woof[DHT_NAME_LENGTH];
	char callback_handler[DHT_NAME_LENGTH];
} GET_PREDECESSOR_ARG;

typedef struct get_predecessor_result_stc {
	unsigned char predecessor_hash[SHA_DIGEST_LENGTH];
	unsigned char predecessor_addr[DHT_NAME_LENGTH];
} GET_PREDECESSOR_RESULT;

typedef struct notify_arg_stc {
	unsigned char node_hash[SHA_DIGEST_LENGTH];
	unsigned char node_addr[DHT_NAME_LENGTH];
	char callback_woof[DHT_NAME_LENGTH];
	char callback_handler[DHT_NAME_LENGTH];
} NOTIFY_ARG;

typedef struct notify_result_stc {
	char successor_addr[DHT_SUCCESSOR_LIST_R][DHT_NAME_LENGTH];
	unsigned char successor_hash[DHT_SUCCESSOR_LIST_R][SHA_DIGEST_LENGTH];
} NOTIFY_RESULT;

typedef struct init_topic_arg_stc {
	char topic[DHT_NAME_LENGTH];
	unsigned long element_size;
	unsigned long history_size;
} INIT_TOPIC_ARG;

typedef struct subscription_list_stc {
	int size;
	char handlers[DHT_SUCRIPTION_MAX_HANDLERS][DHT_NAME_LENGTH];
} SUBSCRIPTION_LIST;

typedef struct subscription_arg_stc {
	char topic[DHT_NAME_LENGTH];
	char handler[DHT_NAME_LENGTH];
} SUBSCRIPTION_ARG;

typedef struct trigger_arg_stc {
	char woof_name[DHT_NAME_LENGTH];
	unsigned long seqno;
} TRIGGER_ARG;

void dht_init(unsigned char *node_hash, char *node_addr, DHT_TABLE_EL *el);
void find_init(char *node_addr, char *callback, char *topic, FIND_SUCESSOR_ARG *arg);
unsigned long get_milliseconds();
int node_woof_name(char *node_woof);
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

typedef struct test_stc {
	char msg[256];
} TEST_EL;

#endif
