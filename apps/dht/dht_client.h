#ifdef __cplusplus
extern "C" {
#endif

#ifndef DHT_CLIENT_H
#define DHT_CLIENT_H

#include "dht.h"

void dht_set_client_ip(char* ip);
int dht_find_node(char* topic_name,
                  char result_node_replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH],
                  int* result_node_leader,
                  int timeout);
int dht_find_node_debug(char* topic_name,
                        char result_node_replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH],
                        int* result_node_leader,
                        int* query_count,
                        int* message_count,
                        int* failure_count,
                        int timeout);
int dht_create_topic(char* topic_name, unsigned long element_size, unsigned long history_size);
int dht_register_topic(char* topic_name, int timeout);
int dht_subscribe(char* topic_name, char* handler);
unsigned long dht_publish(char* topic_name, void* element, unsigned long element_size, int timeout);

#ifdef USE_RAFT
int dht_topic_is_empty(unsigned long seqno);
unsigned long dht_topic_latest_seqno(char* topic_name, int timeout);
unsigned long dht_local_topic_latest_seqno(char* topic_name);
unsigned long dht_remote_topic_latest_seqno(char* remote_woof, char* topic_name);
int dht_topic_get(char* topic_name, void* element, unsigned long element_size, unsigned long seqno, int timeout);
int dht_local_topic_get(char* topic_name, void* element, unsigned long element_size, unsigned long seqno);
int dht_remote_topic_get(
    char* remote_woof, char* topic_name, void* element, unsigned long element_size, unsigned long seqno);
int dht_topic_get_range(char* topic_name,
                        void* element,
                        unsigned long element_size,
                        unsigned long seqno,
                        uint64_t lower_ts,
                        uint64_t upper_ts,
                        int timeout);
int dht_local_topic_get_range(char* topic_name,
                              void* element,
                              unsigned long element_size,
                              unsigned long seqno,
                              uint64_t lower_ts,
                              uint64_t upper_ts);
int dht_remote_topic_get_range(char* remote_woof,
                               char* topic_name,
                               void* element,
                               unsigned long element_size,
                               unsigned long seqno,
                               uint64_t lower_ts,
                               uint64_t upper_ts);
#endif

#endif

#ifdef __cplusplus
}
#endif
