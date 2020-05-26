#ifndef DHT_UTILS_H
#define DHT_UTILS_H

#include <stdio.h>
#include "dht.h"

#define DHT_LOG_DEBUG 0
#define DHT_LOG_INFO 1
#define DHT_LOG_WARN 2
#define DHT_LOG_ERROR 3

unsigned long get_milliseconds();
int node_woof_name(char *woof_name);
void log_set_level(int level);
void log_set_output(FILE *file);
void log_set_tag(const char *tag);
void log_debug(const char *message, ...);
void log_info(const char *message, ...);
void log_warn(const char *message, ...);
void log_error(const char *message, ...);

int get_latest_element(char *woof_name, void *element);
int read_config(FILE *fp, int *len, char replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH]);
void dht_init(unsigned char *node_hash, char *node_addr, char replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH], int replica_id, DHT_TABLE *dht_table);
void dht_init_find_arg(DHT_FIND_SUCCESSOR_ARG *arg, char *key, char *hashed_key, char *callback_namespace);
void print_node_hash(char *dst, const unsigned char *id_hash);
int is_empty(char hash[SHA_DIGEST_LENGTH]);
int in_range(unsigned char *n, unsigned char *lower, unsigned char *upper);
void shift_successor_list(unsigned char successor_hash[DHT_SUCCESSOR_LIST_R][SHA_DIGEST_LENGTH]);
int hmap_set(char *hash, DHT_HASHMAP_ENTRY *entry);
int hmap_get(char *hash, DHT_HASHMAP_ENTRY *entry);
int hmap_set_replicas(char *hash, char replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH], int leader);

#endif
