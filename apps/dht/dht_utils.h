#ifdef __cplusplus
extern "C" {
#endif

#ifndef DHT_UTILS_H
#define DHT_UTILS_H

#include "dht.h"

#include <stdio.h>

#define DHT_LOG_DEBUG 0
#define DHT_LOG_INFO 1
#define DHT_LOG_WARN 2
#define DHT_LOG_ERROR 3

uint64_t get_milliseconds();
void node_woof_namespace(char* woof_namespace);
void log_set_level(int level);
void log_set_output(FILE* file);
void log_set_tag(const char* tag);
void log_debug(const char* message, ...);
void log_info(const char* message, ...);
void log_warn(const char* message, ...);
void log_error(const char* message, ...);

int get_latest_element(char* woof_name, void* element);
int read_raft_config(FILE* fp, char* name, int* len, char replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH]);
int read_dht_config(FILE* fp,
                    int* stabilize_freq,
                    int* chk_predecessor_freq,
                    int* fix_finger_freq,
                    int* update_leader_freq,
                    int* daemon_wakeup_freq);
void serialize_dht_config(char* dst,
                          int stabilize_freq,
                          int chk_predecessor_freq,
                          int fix_finger_freq,
                          int update_leader_freq,
                          int daemon_wakeup_freq);
void deserialize_dht_config(char* src,
                            int* stabilize_freq,
                            int* chk_predecessor_freq,
                            int* fix_finger_freq,
                            int* update_leader_freq,
                            int* daemon_wakeup_freq);
int dht_init(unsigned char* node_hash,
             char* node_name,
             char* node_addr,
             char node_replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH]);
void dht_init_find_arg(DHT_FIND_SUCCESSOR_ARG* arg, char* key, char* hashed_key, char* callback_namespace);
char* dht_hash(unsigned char* dst, char* src);
void print_node_hash(char* dst, const unsigned char* id_hash);
int is_empty(char hash[SHA_DIGEST_LENGTH]);
int in_range(unsigned char* n, unsigned char* lower, unsigned char* upper);
int get_latest_node_info(DHT_NODE_INFO* element);
int get_latest_predecessor_info(DHT_PREDECESSOR_INFO* element);
int get_latest_successor_info(DHT_SUCCESSOR_INFO* element);
int get_latest_finger_info(int finger_id, DHT_FINGER_INFO* element);
unsigned long set_finger_info(int finger_id, DHT_FINGER_INFO* element);
char* predecessor_addr(DHT_PREDECESSOR_INFO* info);
char* successor_addr(DHT_SUCCESSOR_INFO* info, int r);
char* finger_addr(DHT_FINGER_INFO* info);
int raft_leader_id();
int try_successor_replicas();
int shift_successor();
int invalidate_fingers(char hash[SHA_DIGEST_LENGTH]);
#define TRY_SUCCESSOR_REPLICAS_TIMEOUT 10000

#endif

#ifdef __cplusplus
}
#endif
