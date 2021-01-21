#ifdef __cplusplus
extern "C" {
#endif

#ifndef DHT_CLIENT_H
#define DHT_CLIENT_H

#include "dht.h"

void dht_set_client_ip(char* ip);
int dht_create_topic(char* topic_name, unsigned long element_size, unsigned long history_size);
int dht_register_topic(char* topic_name, int timeout);
int dht_subscribe(char* topic_name, char* handler);
int dht_publish(char* topic_name, void* element, uint64_t element_size);
int dht_find_node(char* topic_name, int* node_leader, char node_replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH]);
int dht_get_registry(char* topic_name, DHT_TOPIC_REGISTRY* topic_entry, char* registry_woof);
unsigned long dht_latest_index(char* topic_name);
unsigned long dht_latest_earlier_index(char* topic_name, unsigned long element_seqno);
int dht_get(char* topic_name, RAFT_DATA_TYPE* data, unsigned long index);

#endif

#ifdef __cplusplus
}
#endif
