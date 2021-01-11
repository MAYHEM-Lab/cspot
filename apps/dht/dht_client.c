#include "dht_client.h"

#include "dht.h"
#include "dht_utils.h"
#include "monitor.h"
#include "raft.h"
#include "raft_client.h"
#include "woofc-access.h"
#include "woofc.h"

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
    if (element_size > RAFT_DATA_TYPE_SIZE) {
        sprintf(dht_error_msg, "element_size exceeds RAFT_DATA_TYPE_SIZE");
        return -1;
    }

    return 0;
    // when using raft, there's no need to create topic woof since the data is stored in raft log
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
    DHT_NODE_INFO node_info = {0};
    if (get_latest_node_info(&node_info) < 0) {
        char err_msg[DHT_NAME_LENGTH] = {0};
        strcpy(err_msg, dht_error_msg);
        sprintf(dht_error_msg, "couldn't get latest node info: %s", err_msg);
        return -1;
    }

    memcpy(register_topic_arg.topic_replicas, node_info.replicas, sizeof(register_topic_arg.topic_replicas));
    register_topic_arg.topic_leader = node_info.leader_id;

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
    char find_sucessor_woof[DHT_NAME_LENGTH] = {0};
    if (node_info.leader_id != node_info.replica_id) {
        sprintf(find_sucessor_woof, "%s/%s", node_info.replicas[node_info.leader_id], DHT_FIND_SUCCESSOR_WOOF);
    } else {
        strcpy(find_sucessor_woof, DHT_FIND_SUCCESSOR_WOOF);
    }
    unsigned long seq_no = WooFPut(find_sucessor_woof, NULL, &arg);
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
    if (WooFGet(RAFT_SERVER_STATE_WOOF, &raft_server_state, 0) < 0) {
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

    DHT_NODE_INFO node_info = {0};
    if (get_latest_node_info(&node_info) < 0) {
        char err_msg[DHT_NAME_LENGTH] = {0};
        strcpy(err_msg, dht_error_msg);
        sprintf(dht_error_msg, "couldn't get latest node info: %s", err_msg);
        return -1;
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
    char find_sucessor_woof[DHT_NAME_LENGTH] = {0};
    if (node_info.leader_id != node_info.replica_id) {
        sprintf(find_sucessor_woof, "%s/%s", node_info.replicas[node_info.leader_id], DHT_FIND_SUCCESSOR_WOOF);
    } else {
        strcpy(find_sucessor_woof, DHT_FIND_SUCCESSOR_WOOF);
    }
    unsigned long seq = WooFPut(find_sucessor_woof, NULL, &arg);
    if (WooFInvalid(seq)) {
        sprintf(dht_error_msg, "failed to invoke h_find_successor");
        exit(1);
    }

    return 0;
}

int dht_publish(char* topic_name, void* element, uint64_t element_size) {
    if (element_size > sizeof(RAFT_DATA_TYPE)) {
        sprintf(dht_error_msg, "element size %lu exceeds the maximum size %lu", element_size, sizeof(RAFT_DATA_TYPE));
        return -1;
    }
    RAFT_DATA_TYPE publish_element = {0};
    memcpy(&publish_element, element, element_size);

    unsigned long seq = WooFPut(DHT_SERVER_PUBLISH_ELEMENT_WOOF, NULL, &publish_element);
    if (WooFInvalid(seq)) {
        sprintf(dht_error_msg, "failed put element to %s", DHT_SERVER_PUBLISH_ELEMENT_WOOF);
        return -1;
    }

    // see if the topic is in the cache
    unsigned long i = WooFGetLatestSeqno(DHT_TOPIC_CACHE_WOOF);
    if (WooFInvalid(i)) {
        sprintf(dht_error_msg, "failed to get the latest seqno of topic cache");
        return -1;
    }
    DHT_TOPIC_CACHE cache = {0};
    while (i != 0) {
        if (WooFGet(DHT_TOPIC_CACHE_WOOF, &cache, i) < 0) {
            sprintf(dht_error_msg, "failed to get the topic cache at %lu", i);
            return -1;
        }
        if (strcmp(cache.topic_name, topic_name) == 0) {
            if (cache.node_leader == -1) { // cache invalidated
                break;
            }
            DHT_SERVER_PUBLISH_DATA_ARG publish_data_arg = {0};
            publish_data_arg.element_seqno = seq;
            publish_data_arg.node_leader = cache.node_leader;
            memcpy(publish_data_arg.node_replicas, cache.node_replicas, sizeof(publish_data_arg.node_replicas));
            strcpy(publish_data_arg.topic_name, topic_name);
            publish_data_arg.ts_created = get_milliseconds();
            publish_data_arg.ts_found = publish_data_arg.ts_created;
            publish_data_arg.update_cache = 0;
            unsigned long publish_data_seq = WooFPut(DHT_SERVER_PUBLISH_DATA_WOOF, NULL, &publish_data_arg);
            if (WooFInvalid(publish_data_seq)) {
                sprintf(dht_error_msg, "failed to invoke server_publish_data");
                return -1;
            }
            return 0;
        }
        --i;
    }

    // not found in cache
    DHT_NODE_INFO node_info = {0};
    if (WooFGet(DHT_NODE_INFO_WOOF, &node_info, 0) < 0) {
        sprintf(dht_error_msg, "failed to get node info");
        return -1;
    }
    char hashed_key[SHA_DIGEST_LENGTH];
    dht_hash(hashed_key, topic_name);
    DHT_FIND_SUCCESSOR_ARG arg = {0};
    dht_init_find_arg(&arg, topic_name, hashed_key, node_info.addr);
    arg.action_seqno = seq;
    arg.action = DHT_ACTION_PUBLISH;
    arg.ts_created = get_milliseconds();

    char find_sucessor_woof[DHT_NAME_LENGTH] = {0};
    if (node_info.leader_id != node_info.replica_id) {
        sprintf(find_sucessor_woof, "%s/%s", node_info.replicas[node_info.leader_id], DHT_FIND_SUCCESSOR_WOOF);
    } else {
        strcpy(find_sucessor_woof, DHT_FIND_SUCCESSOR_WOOF);
    }
    seq = WooFPut(find_sucessor_woof, NULL, &arg);
    if (WooFInvalid(seq)) {
        log_error("failed to invoke h_find_successor");
        return -1;
    }
    return 0;
}