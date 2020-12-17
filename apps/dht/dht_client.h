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
unsigned long dht_publish(DHT_SERVER_PUBLISH_FIND_ARG* arg);

#endif

#ifdef __cplusplus
}
#endif
