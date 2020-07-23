#ifndef DHT_CLIENT_H
#define DHT_CLIENT_H

#include "dht.h"

void dht_set_client_ip(char* ip);
int dht_find_node(char* topic_name,
                  char result_node_replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH],
                  int* result_node_leader,
                  int* hops);
int dht_create_topic(char* topic_name, unsigned long element_size, unsigned long history_size);
int dht_register_topic(char* topic_name);
int dht_subscribe(char* topic_name, char* handler);
unsigned long dht_publish(char* topic_name, void* element);

#ifdef USE_RAFT
unsigned long dht_topic_latest_seqno(char* topic_name);
unsigned long dht_remote_topic_latest_seqno(char* remote_woof, char* topic_name);
int dht_topic_get(char* topic_name, void* element, unsigned long element_size, unsigned long seqno);
int dht_remote_topic_get(
    char* remote_woof, char* topic_name, void* element, unsigned long element_size, unsigned long seqno);
int dht_topic_get_range(char* topic_name,
                        void* element,
                        unsigned long element_size,
                        unsigned long seqno,
                        unsigned long lower_ts,
                        unsigned long upper_ts);
int dht_remote_topic_get_range(char* remote_woof,
                               char* topic_name,
                               void* element,
                               unsigned long element_size,
                               unsigned long seqno,
                               unsigned long lower_ts,
                               unsigned long upper_ts);
#endif

#endif
