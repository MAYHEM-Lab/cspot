#ifndef DHT_H
#define DHT_H

#include <stdint.h>
#include <openssl/sha.h>

#define DHT_TABLE_WOOF "dht_table"
#define DHT_FIND_SUCESSOR_ARG_WOOF "dht_find_successor_arg"
#define DHT_FIND_SUCESSOR_RESULT_WOOF "dht_find_successor_result"
#define DHT_GET_PREDECESSOR_ARG_WOOF "dht_get_predecessor_arg"
#define DHT_GET_PREDECESSOR_RESULT_WOOF "dht_get_predecessor_result"
#define DHT_NOTIFY_ARG_WOOF "dht_notify_arg"
#define DHT_INIT_TOPIC_ARG_WOOF "dht_init_topic_arg"
#define DHT_SUBSCRIPTION_ARG_WOOF "dht_subscription_arg"
#define DHT_SUBSCRIPTION_LIST_WOOF "dht_subscription_list"
#define DHT_TRIGGER_ARG_WOOF "dht_trigger_arg"
#define LOG_DEBUG 0
#define LOG_INFO 1
#define LOG_WARN 2
#define LOG_ERROR 3

FILE *dht_log_output;
int dht_log_level;

struct tbl_stc
{
	unsigned char node_hash[SHA_DIGEST_LENGTH];
	char node_addr[256];
	char finger_addr[SHA_DIGEST_LENGTH * 8 + 1][256];
	unsigned char finger_hash[SHA_DIGEST_LENGTH * 8 + 1][SHA_DIGEST_LENGTH];
	char predecessor_addr[256];
	unsigned char predecessor_hash[SHA_DIGEST_LENGTH];
};
typedef struct tbl_stc DHT_TABLE_EL;

struct find_successor_arg_stc
{
	unsigned long request_seq_no; // the first resuest's seq_no
	int finger_index; // only used by fix_fingers
	int hops;
	unsigned char id_hash[SHA_DIGEST_LENGTH];
	char callback_woof[256];
	char callback_handler[256];
};
typedef struct find_successor_arg_stc FIND_SUCESSOR_ARG;

struct find_result_stc
{
	unsigned long request_seq_no; // the first resuest's seq_no
	int finger_index;
	int hops;
	unsigned char target_hash[SHA_DIGEST_LENGTH];
	unsigned char node_hash[SHA_DIGEST_LENGTH];
	unsigned char node_addr[256];
};
typedef struct find_result_stc FIND_SUCESSOR_RESULT;

struct get_predecessor_arg_stc
{
	char callback_woof[256];
	char callback_handler[256];
};
typedef struct get_predecessor_arg_stc GET_PREDECESSOR_ARG;

struct get_predecessor_result_stc
{
	unsigned char predecessor_hash[SHA_DIGEST_LENGTH];
	unsigned char predecessor_addr[256];
};
typedef struct get_predecessor_result_stc GET_PREDECESSOR_RESULT;

struct notify_arg_stc
{
	unsigned char node_hash[SHA_DIGEST_LENGTH];
	unsigned char node_addr[256];
};
typedef struct notify_arg_stc NOTIFY_ARG;

struct init_topic_arg_stc
{
	char topic[256];
	unsigned long element_size;
	unsigned long history_size;
};
typedef struct init_topic_arg_stc INIT_TOPIC_ARG;

struct subscription_list_stc
{
	int size;
	char handlers[64 * 256];
};
typedef struct subscription_list_stc SUBSCRIPTION_LIST;

struct subscription_arg_stc
{
	char topic[256];
	char handler[64];
};
typedef struct subscription_arg_stc SUBSCRIPTION_ARG;

struct trigger_arg_stc
{
	char woof_name[256];
	unsigned long seqno;
};
typedef struct trigger_arg_stc TRIGGER_ARG;

void dht_init(unsigned char *node_hash, char *node_addr, DHT_TABLE_EL *el);
void find_init(char *node_addr, char *callback, char *topic, FIND_SUCESSOR_ARG *arg);
int node_woof_name(char *node_woof);
void print_node_hash(char *dst, const unsigned char *id_hash);
int in_range(unsigned char *n, unsigned char *lower, unsigned char *upper);
void log_set_level(int level);
void log_set_output(FILE *file);
void log_info(const char *tag, const char *message);
void log_warn(const char *tag, const char *message);
void log_error(const char *tag, const char *message);
void log_debug(const char *tag, const char *message);

struct test_stc
{
	char msg[256];
};
typedef struct test_stc TEST_EL;

#endif
