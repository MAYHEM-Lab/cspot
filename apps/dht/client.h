#ifndef CLIENT_H
#define CLIENT_H

#include "dht.h"

int dht_find_addr(char *node, char *topic_name, char *result_addr);
int dht_find_node(char *node, char *topic_name, char *result_node);
int dht_create_topic(char *topic_name, unsigned long element_size, unsigned long history_size);
int dht_register_topic(char *node, char *topic_name);
int dht_subscribe(char *node, char *topic_name, char *handler);
unsigned long dht_publish(char *node, char *topic_name, void *element);
unsigned long dht_remote_publish(char *node, char *topic_namespace, char *topic_name, void *element);
int dht_get(char *woof_name, char *topic_name, void *element, unsigned long seq_no);

#endif
