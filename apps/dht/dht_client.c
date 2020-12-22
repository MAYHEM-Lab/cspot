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

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

char client_ip[DHT_NAME_LENGTH] = {0};

void dht_set_client_ip(char* ip) {
    strcpy(client_ip, ip);
}

// call this on the node hosting the topic
int dht_create_topic(char* topic_name, unsigned long element_size, unsigned long history_size) {
#ifdef USE_RAFT
    if (element_size > RAFT_DATA_TYPE_SIZE) {
        sprintf(dht_error_msg, "element_size exceeds RAFT_DATA_TYPE_SIZE");
        return -1;
    }

    return 0;
    // when using raft, there's no need to create topic woof since the data is stored in raft log
#else
    return WooFCreate(topic_name, element_size, history_size);
#endif
}

int dht_register_topic(char* topic_name, int timeout) {
    char local_namespace[DHT_NAME_LENGTH] = {0};
    node_woof_namespace(local_namespace);
    char topic_namespace[DHT_NAME_LENGTH] = {0};
    if (client_ip[0] == 0) {
        char ip[DHT_NAME_LENGTH] = {0};
        if (WooFLocalIP(ip, sizeof(ip)) < 0) {
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
        char err_msg[DHT_NAME_LENGTH] = {0};
        strcpy(err_msg, dht_error_msg);
        sprintf(dht_error_msg, "couldn't get latest node info: %s", err_msg);
        return -1;
    }
    memcpy(register_topic_arg.topic_replicas, node_info.replicas, sizeof(register_topic_arg.topic_replicas));
    register_topic_arg.topic_leader = node_info.leader_id;
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
    strcpy(arg.action_namespace, topic_namespace);
    arg.action_seqno = action_seqno;
    unsigned long seq_no = WooFPut(DHT_FIND_SUCCESSOR_WOOF, NULL, &arg);
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

    RAFT_SERVER_STATE raft_server_state;
    if (get_server_state(&raft_server_state) < 0) {
        sprintf(dht_error_msg, "failed to get the latest RAFT server state");
        return -1;
    }
    char replica_namespaces[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH] = {0};
    int i;
    for (i = 0; i < DHT_REPLICA_NUMBER && i < raft_server_state.members; ++i) {
        strcpy(replica_namespaces[i], raft_server_state.member_woofs[i]);
    }

    char local_namespace[DHT_NAME_LENGTH] = {0};
    node_woof_namespace(local_namespace);
    char action_namespace[DHT_NAME_LENGTH] = {0};
    if (client_ip[0] == 0) {
        char ip[DHT_NAME_LENGTH] = {0};
        if (WooFLocalIP(ip, sizeof(ip)) < 0) {
            sprintf(dht_error_msg, "failed to get local IP");
            return -1;
        }
        sprintf(action_namespace, "woof://%s%s", ip, local_namespace);
    } else {
        sprintf(action_namespace, "woof://%s%s", client_ip, local_namespace);
    }

    DHT_SUBSCRIBE_ARG subscribe_arg = {0};
    strcpy(subscribe_arg.topic_name, topic_name);
    strcpy(subscribe_arg.handler, handler);
    memcpy(subscribe_arg.replica_namespaces, replica_namespaces, sizeof(subscribe_arg.replica_namespaces));
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
    strcpy(arg.action_namespace, action_namespace);
    arg.action_seqno = action_seqno;

    unsigned long seq = WooFPut(DHT_FIND_SUCCESSOR_WOOF, NULL, &arg);
    if (WooFInvalid(seq)) {
        sprintf(dht_error_msg, "failed to invoke h_find_successor");
        exit(1);
    }

    return 0;
}

unsigned long dht_publish(DHT_SERVER_PUBLISH_FIND_ARG* arg) {
    arg->ts_a = get_milliseconds();
    unsigned long seq = WooFPut(DHT_SERVER_PUBLISH_FIND_WOOF, NULL, arg);
    if (WooFInvalid(seq)) {
        sprintf(dht_error_msg, "failed put publish request to %s", DHT_SERVER_PUBLISH_FIND_WOOF);
        return -1;
    }
    return seq;
}