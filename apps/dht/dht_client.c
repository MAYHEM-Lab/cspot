#include "dht_client.h"

#include "dht.h"
#include "dht_utils.h"
#include "monitor.h"
#include "woofc-access.h"
#include "woofc.h"
#ifdef USE_RAFT
#include "raft.h"
#include "raft_client.h"
#endif

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

char client_ip[DHT_NAME_LENGTH] = {0};

void dht_set_client_ip(char* ip) {
    strcpy(client_ip, ip);
}

int dht_find_node(char* topic_name,
                  char result_node_replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH],
                  int* result_node_leader,
                  int* hops) {
    char local_namespace[DHT_NAME_LENGTH] = {0};
    node_woof_namespace(local_namespace);
    char callback_namespace[DHT_NAME_LENGTH] = {0};
    if (client_ip[0] == 0) {
        char ip[DHT_NAME_LENGTH] = {0};
        if (WooFLocalIP(ip, sizeof(ip) < 0)) {
            sprintf(dht_error_msg, "failed to get local IP");
            return -1;
        }
        sprintf(callback_namespace, "woof://%s%s", ip, local_namespace);
    } else {
        sprintf(callback_namespace, "woof://%s%s", client_ip, local_namespace);
    }

    char hashed_key[SHA_DIGEST_LENGTH];
    dht_hash(hashed_key, topic_name);
    DHT_FIND_SUCCESSOR_ARG arg = {0};
    dht_init_find_arg(&arg, topic_name, hashed_key, callback_namespace);
    arg.action = DHT_ACTION_FIND_NODE;

    unsigned long last_checked_result = WooFGetLatestSeqno(DHT_FIND_NODE_RESULT_WOOF);

    unsigned long seq_no = WooFPut(DHT_FIND_SUCCESSOR_WOOF, "h_find_successor", &arg);
    if (WooFInvalid(seq_no)) {
        sprintf(dht_error_msg, "failed to invoke h_find_successor");
        return -1;
    }

    while (1) {
        unsigned long latest_result = WooFGetLatestSeqno(DHT_FIND_NODE_RESULT_WOOF);
        unsigned long i;
        for (i = last_checked_result + 1; i <= latest_result; ++i) {
            DHT_FIND_NODE_RESULT result = {0};
            if (WooFGet(DHT_FIND_NODE_RESULT_WOOF, &result, i) < 0) {
                sprintf(dht_error_msg, "failed to get result at %lu from %s", i, DHT_FIND_NODE_RESULT_WOOF);
                return -1;
            }
            if (memcmp(result.topic, topic_name, SHA_DIGEST_LENGTH) == 0) {
                memcpy(result_node_replicas, result.node_replicas, sizeof(result.node_replicas));
                *result_node_leader = result.node_leader;
                *hops = result.hops;
                return 0;
            }
        }
        last_checked_result = latest_result;
        usleep(10 * 1000);
    }
}

// call this on the node hosting the topic
int dht_create_topic(char* topic_name, unsigned long element_size, unsigned long history_size) {
#ifdef USE_RAFT
    if (element_size > RAFT_DATA_TYPE_SIZE) {
        sprintf(dht_error_msg, "element_size exceeds RAFT_DATA_TYPE_SIZE");
        return -1;
    }
    return 1;
    // when using raft, there's no need to create topic woof since the data is stored in raft log
#else
    return WooFCreate(topic_name, element_size, history_size);
#endif
}

int dht_register_topic(char* topic_name) {
    char local_namespace[DHT_NAME_LENGTH] = {0};
    node_woof_namespace(local_namespace);
    char topic_namespace[DHT_NAME_LENGTH] = {0};
    if (client_ip[0] == 0) {
        char ip[DHT_NAME_LENGTH] = {0};
        if (WooFLocalIP(ip, sizeof(ip) < 0)) {
            sprintf(dht_error_msg, "failed to get local IP");
            return -1;
        }
        sprintf(topic_namespace, "woof://%s%s", ip, local_namespace);
    } else {
        sprintf(topic_namespace, "woof://%s%s", client_ip, local_namespace);
    }

    DHT_REGISTER_TOPIC_ARG register_topic_arg = {0};
    strcpy(register_topic_arg.topic_name, topic_name);
#ifdef USE_RAFT
    DHT_NODE_INFO node_info = {0};
    if (get_latest_node_info(&node_info) < 0) {
        sprintf(dht_error_msg, "couldn't get latest node info: %s", dht_error_msg);
        return -1;
    }
    memcpy(register_topic_arg.topic_replicas, node_info.replicas, sizeof(register_topic_arg.topic_replicas));
#else
    strcpy(register_topic_arg.topic_namespace, local_namespace);
#endif
    unsigned long action_seqno = WooFPut(DHT_REGISTER_TOPIC_WOOF, NULL, &register_topic_arg);
    if (WooFInvalid(action_seqno)) {
        sprintf(dht_error_msg, "failed to store subscribe arg");
        return -1;
    }

    char hashed_key[SHA_DIGEST_LENGTH];
    dht_hash(hashed_key, topic_name);
    DHT_FIND_SUCCESSOR_ARG arg = {0};
    dht_init_find_arg(&arg, topic_name, hashed_key, ""); // namespace doesn't matter, acted in target namespace
    arg.action = DHT_ACTION_REGISTER_TOPIC;
    strcpy(arg.action_namespace, local_namespace);
    arg.action_seqno = action_seqno;

    unsigned long seq_no = WooFPut(DHT_FIND_SUCCESSOR_WOOF, "h_find_successor", &arg);
    if (WooFInvalid(seq_no)) {
        sprintf(dht_error_msg, "failed to invoke h_find_successor");
        return -1;
    }
    return 0;
}

int dht_subscribe(char* topic_name, char* handler) {
    if (access(handler, F_OK) == -1) {
        sprintf(dht_error_msg, "handler %s doesn't exist", handler);
        return -1;
    }

    char local_namespace[DHT_NAME_LENGTH] = {0};
    node_woof_namespace(local_namespace);
    char handler_namespace[DHT_NAME_LENGTH] = {0};
    if (client_ip[0] == 0) {
        char ip[DHT_NAME_LENGTH] = {0};
        if (WooFLocalIP(ip, sizeof(ip) < 0)) {
            sprintf(dht_error_msg, "failed to get local IP");
            return -1;
        }
        sprintf(handler_namespace, "woof://%s%s", ip, local_namespace);
    } else {
        sprintf(handler_namespace, "woof://%s%s", client_ip, local_namespace);
    }

    DHT_SUBSCRIBE_ARG subscribe_arg = {0};
    strcpy(subscribe_arg.topic_name, topic_name);
    strcpy(subscribe_arg.handler, handler);
    strcpy(subscribe_arg.handler_namespace, handler_namespace);
    unsigned long action_seqno = WooFPut(DHT_SUBSCRIBE_WOOF, NULL, &subscribe_arg);
    if (WooFInvalid(action_seqno)) {
        sprintf(dht_error_msg, "failed to store subscribe arg");
        return -1;
    }

    char hashed_key[SHA_DIGEST_LENGTH];
    dht_hash(hashed_key, topic_name);
    DHT_FIND_SUCCESSOR_ARG arg = {0};
    dht_init_find_arg(&arg, topic_name, hashed_key, ""); // namespace doesn't matter, acted in target namespace
    arg.action = DHT_ACTION_SUBSCRIBE;
    strcpy(arg.action_namespace, local_namespace);
    arg.action_seqno = action_seqno;

    unsigned long seq = WooFPut(DHT_FIND_SUCCESSOR_WOOF, "h_find_successor", &arg);
    if (WooFInvalid(seq)) {
        sprintf(dht_error_msg, "failed to invoke h_find_successor");
        exit(1);
    }

    return 0;
}

unsigned long dht_publish(char* topic_name, void* element) {
unsigned long called = get_milliseconds(); 
    char node_replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH] = {0};
    int node_leader;
    int hops;
    if (dht_find_node(topic_name, node_replicas, &node_leader, &hops) < 0) {
        sprintf(dht_error_msg, "failed to find node hosting the topic");
        return -1;
    }
    char* node_addr = node_replicas[node_leader];
unsigned long found = get_milliseconds(); 
    // TODO: if failed use the next replica
    // get the topic namespace
    char registration_woof[DHT_NAME_LENGTH] = {0};
    sprintf(registration_woof, "%s/%s_%s", node_addr, topic_name, DHT_TOPIC_REGISTRATION_WOOF);

    DHT_TOPIC_REGISTRY topic_entry = {0};
    if (get_latest_element(registration_woof, &topic_entry) < 0) {
        sprintf(dht_error_msg, "failed to get topic registration info from %s: %s", registration_woof, dht_error_msg);
        return -1;
    }

    DHT_TRIGGER_ARG trigger_arg = {0};
    strcpy(trigger_arg.topic_name, topic_name);
    sprintf(trigger_arg.subscription_woof, "%s/%s_%s", node_addr, topic_name, DHT_SUBSCRIPTION_LIST_WOOF);
    char trigger_woof[DHT_NAME_LENGTH] = {0};
    // TODO: use other replicas if the first one failed
#ifdef USE_RAFT
    printf("using raft, leader: %s\n", topic_entry.topic_replicas[0]);
    raft_set_client_leader(topic_entry.topic_replicas[0]);
    raft_set_client_result_delay(0);
    RAFT_DATA_TYPE raft_data = {0};
    memcpy(raft_data.val, element, sizeof(raft_data.val));
    sprintf(trigger_arg.element_woof, "%s", topic_entry.topic_replicas[0]);
    unsigned long index = raft_sync_put(&raft_data, 0);
    while (index == RAFT_REDIRECTED) {
        printf("redirected to %s\n", raft_client_leader);
        index = raft_sync_put(&raft_data, 0);
    }
    if (raft_is_error(index)) {
        sprintf(dht_error_msg, "failed to put to raft: %s", raft_error_msg);
        return -1;
    }
unsigned long committed = get_milliseconds(); 
    trigger_arg.element_seqno = index;
    sprintf(trigger_woof, "%s/%s", topic_entry.topic_replicas[0], DHT_TRIGGER_WOOF);
#else
    sprintf(trigger_arg.element_woof, "%s/%s", topic_entry.topic_namespace, topic_name);
    printf("using woof: %s\n", trigger_arg.element_woof);
    trigger_arg.element_seqno = WooFPut(trigger_arg.element_woof, NULL, element);
    if (WooFInvalid(trigger_arg.element_seqno)) {
        sprintf(dht_error_msg, "failed to put element to woof");
        return -1;
    }
unsigned long committed = get_milliseconds(); 
    sprintf(trigger_woof, "%s/%s", topic_entry.topic_namespace, DHT_TRIGGER_WOOF);
#endif
    unsigned long seq = WooFPut(trigger_woof, "h_trigger", &trigger_arg);
unsigned long triggered = get_milliseconds(); 
    printf("called: %lu\n", called);
    printf("found: %lu %lu\n", found, found - called);
    printf("committed: %lu %lu %lu\n", committed, committed - called, committed - found);
    fflush(stdout);
    return seq;
}