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

int dht_find_node(char* topic_name,
                  char result_node_replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH],
                  int* result_node_leader,
                  int timeout) {
    int query_count;
    int message_count;
    int failure_count;
    // char path[4096];
    return dht_find_node_debug(
        topic_name, result_node_replicas, result_node_leader, &query_count, &message_count, &failure_count, timeout);
    // return dht_find_node_debug(
    //     topic_name, result_node_replicas, result_node_leader, &query_count, &message_count, &failure_count, timeout);
}

int dht_find_node_debug(char* topic_name,
                        char result_node_replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH],
                        int* result_node_leader,
                        int* query_count,
                        int* message_count,
                        int* failure_count,
                        int timeout) {
    uint64_t begin = get_milliseconds();

    char local_namespace[DHT_NAME_LENGTH] = {0};
    node_woof_namespace(local_namespace);
    char callback_namespace[DHT_NAME_LENGTH] = {0};
    if (client_ip[0] == 0) {
        char ip[DHT_NAME_LENGTH] = {0};
        if (WooFLocalIP(ip, sizeof(ip)) < 0) {
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
        if (timeout > 0 && get_milliseconds() - begin > timeout) {
            sprintf(dht_error_msg, "timeout after %d ms", timeout);
            return -1;
        }

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
                *query_count = result.find_successor_query_count;
                *message_count = result.find_successor_message_count;
                *failure_count = result.find_successor_failure_count;
                // strcpy(path, result.path);
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
    // initial raft index mapping woof
    DHT_MAP_RAFT_INDEX_ARG map_raft_index_arg = {0};
    strcpy(map_raft_index_arg.topic_name, topic_name);
    unsigned long mapping_index = raft_sessionless_put_handler(node_info.replicas[node_info.leader_id],
                                                               "r_map_raft_index",
                                                               &map_raft_index_arg,
                                                               sizeof(DHT_MAP_RAFT_INDEX_ARG),
                                                               1,
                                                               timeout);
    if (raft_is_error(mapping_index)) {
        sprintf(dht_error_msg, "failed to put to raft: %s", raft_error_msg);
        return -1;
    }
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

    unsigned long seq = WooFPut(DHT_FIND_SUCCESSOR_WOOF, "h_find_successor", &arg);
    if (WooFInvalid(seq)) {
        sprintf(dht_error_msg, "failed to invoke h_find_successor");
        exit(1);
    }

    return 0;
}

unsigned long dht_publish(char* topic_name, void* element, unsigned long element_size, int timeout) {
    BLOCKED_NODES blocked_nodes = {0};
    if (get_latest_element(BLOCKED_NODES_WOOF, &blocked_nodes) < 0) {
        log_error("failed to get blocked nodes");
    }
#ifdef USE_RAFT
    if (element_size > RAFT_DATA_TYPE_SIZE) {
        sprintf(dht_error_msg, "element size %lu exceeds RAFT_DATA_TYPE_SIZE %lu", element_size, RAFT_DATA_TYPE_SIZE);
        return -1;
    }
#endif
    uint64_t called = get_milliseconds();
    char node_replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH] = {0};
    int node_leader;

    int query_count;
    int message_count;
    int failure_count;
    // char path[4096];
    if (dht_find_node_debug(
            topic_name, node_replicas, &node_leader, &query_count, &message_count, &failure_count, timeout) < 0) {
        // if (dht_find_node(topic_name, node_replicas, &node_leader, timeout) < 0) {
        char err_msg[DHT_NAME_LENGTH] = {0};
        strcpy(err_msg, dht_error_msg);
        sprintf(dht_error_msg, "failed to find node hosting the topic: %s", err_msg);
        return -1;
    }
    // printf("query_count: %d\n", query_count);
    // printf("message_count: %d\n", message_count);
    // printf("failure_count: %d\n", failure_count);
    // printf("path: %s\n", path);
    uint64_t found = get_milliseconds();
    char registration_woof[DHT_NAME_LENGTH] = {0};
    char registration_monitor[DHT_NAME_LENGTH] = {0};
    DHT_TOPIC_REGISTRY topic_entry = {0};
    char* node_addr;
    int i, j;
    for (i = 0; i < DHT_REPLICA_NUMBER; ++i) {
        node_addr = node_replicas[(node_leader + i) % DHT_REPLICA_NUMBER];
        // get the topic namespace
        sprintf(registration_woof, "%s/%s_%s", node_addr, topic_name, DHT_TOPIC_REGISTRATION_WOOF);
        sprintf(registration_monitor, "%s/%s", node_addr, DHT_MONITOR_NAME);
        if (is_blocked(registration_woof, "client", &blocked_nodes) != 0) {
            continue;
        }
        if (get_latest_element(registration_woof, &topic_entry) >= 0) {
            // found a working replica
            break;
        }
    }
    if (i == DHT_REPLICA_NUMBER) {
        char err_msg[DHT_NAME_LENGTH] = {0};
        strcpy(err_msg, dht_error_msg);
        sprintf(dht_error_msg, "failed to get topic registration info from %s: %s", node_addr, err_msg);
        return -1;
    }
    uint64_t registry = get_milliseconds();
    uint64_t redirect = registry;

    DHT_TRIGGER_ARG trigger_arg = {0};
    strcpy(trigger_arg.topic_name, topic_name);
    sprintf(trigger_arg.subscription_woof, "%s/%s_%s", node_addr, topic_name, DHT_SUBSCRIPTION_LIST_WOOF);
    char trigger_woof[DHT_NAME_LENGTH] = {0};
#ifdef USE_RAFT
    raft_set_client_result_delay(10);
    int working_replica_id = -1;
    unsigned long index;
    char* topic_replica;
    for (i = 0; i < DHT_REPLICA_NUMBER; ++i) {
        topic_replica = topic_entry.topic_replicas[(topic_entry.last_leader + i) % DHT_REPLICA_NUMBER];
#ifdef DEBUG
        printf("putting data to raft: %s\n", topic_replica);
#endif
        if (is_blocked(topic_replica, "client", &blocked_nodes) != 0) {
            sprintf(raft_error_msg, "replica is blocked");
            continue;
        }
        raft_set_client_leader(topic_replica);
        RAFT_DATA_TYPE raft_data = {0};
        memcpy(raft_data.val, element, element_size);
        index = raft_sync_put(&raft_data, timeout);
        while (index == RAFT_REDIRECTED) {
            redirect = get_milliseconds();
            if (timeout > 0 && get_milliseconds() - called > timeout) {
                sprintf(dht_error_msg, "failed to put data to raft, timeout after %d ms", timeout);
                return -1;
            }
#ifdef DEBUG
            printf("redirected to %s\n", raft_client_leader);
#endif
            index = raft_sync_put(&raft_data, timeout);
        }
        if (!raft_is_error(index)) {
            sprintf(trigger_arg.element_woof, "%s", raft_get_client_leader());
            // found a working replica
            working_replica_id = i;
            // update last_leader so next time we'll use it on the first attempt
            for (j = 0; j < DHT_REPLICA_NUMBER; ++j) {
                if (strcmp(raft_get_client_leader(), topic_entry.topic_replicas[j]) == 0) {
                    break;
                }
            }
            if (j != topic_entry.last_leader) {
                topic_entry.last_leader = j;
                DHT_REGISTER_TOPIC_ARG action_arg = {0};
                strcpy(action_arg.topic_name, topic_entry.topic_name);
                strcpy(action_arg.topic_namespace, topic_entry.topic_namespace);
                memcpy(action_arg.topic_replicas, topic_entry.topic_replicas, sizeof(action_arg.topic_replicas));
                action_arg.topic_leader = topic_entry.last_leader;
                index = checkedMonitorRemotePut(&blocked_nodes,
                                                "client",
                                                registration_monitor,
                                                registration_woof,
                                                "r_register_topic",
                                                &action_arg,
                                                0);
                if (WooFInvalid(index)) {
                    fprintf(stderr, "couldn't update topic registry last seen leader at %s\n", registration_woof);
                } else {
                    printf("updated topic registry last seen leader at %s\n", registration_woof);
                }
            }
            break;
        }
    }
    if (i == DHT_REPLICA_NUMBER) {
        sprintf(dht_error_msg, "failed to put to raft: %s", "none of replica is working leader");
        return -1;
    }
    uint64_t committed = get_milliseconds();
    uint64_t redirect2 = committed;

    DHT_MAP_RAFT_INDEX_ARG map_raft_index_arg = {0};
    strcpy(map_raft_index_arg.topic_name, topic_name);
    map_raft_index_arg.raft_index = index;
#ifdef DEBUG
    printf("map_raft_index_arg.raft_index: %" PRIu64 "\n", map_raft_index_arg.raft_index);
#endif
    map_raft_index_arg.timestamp = get_milliseconds();
    unsigned long mapping_index =
        raft_put_handler("r_map_raft_index", &map_raft_index_arg, sizeof(DHT_MAP_RAFT_INDEX_ARG), 1, timeout);
    while (mapping_index == RAFT_REDIRECTED) {
        redirect2 = get_milliseconds();
        if (timeout > 0 && get_milliseconds() - called > timeout) {
            sprintf(dht_error_msg, "failed to map data to raft, timeout after %d ms", timeout);
            return -1;
        }
#ifdef DEBUG
        printf("redirected to %s\n", raft_client_leader);
#endif
        mapping_index =
            raft_put_handler("r_map_raft_index", &map_raft_index_arg, sizeof(DHT_MAP_RAFT_INDEX_ARG), 1, timeout);
    }
    if (raft_is_error(mapping_index)) {
        sprintf(dht_error_msg, "failed to put to raft: %s", raft_error_msg);
        return -1;
    }

    uint64_t mapped = get_milliseconds();
    trigger_arg.element_seqno = index;
    sprintf(trigger_woof, "%s/%s", topic_entry.topic_replicas[working_replica_id], DHT_TRIGGER_WOOF);
#else
    sprintf(trigger_arg.element_woof, "%s/%s", topic_entry.topic_namespace, topic_name);
#ifdef DEBUG
    printf("using woof: %s\n", trigger_arg.element_woof);
#endif
    trigger_arg.element_seqno = WooFPut(trigger_arg.element_woof, NULL, element);
    if (WooFInvalid(trigger_arg.element_seqno)) {
        sprintf(dht_error_msg, "failed to put element to woof");
        return -1;
    }
    uint64_t committed = get_milliseconds();
    sprintf(trigger_woof, "%s/%s", topic_entry.topic_namespace, DHT_TRIGGER_WOOF);
#endif
    unsigned long seq = WooFPut(trigger_woof, "h_trigger", &trigger_arg);
    if (WooFInvalid(seq)) {
        sprintf(dht_error_msg, "failed to invoke h_trigger");
        return -1;
    }
    uint64_t triggered = get_milliseconds();
    // #ifdef DEBUG
    printf("called: %" PRIu64 "\n", called);
    printf("found: %" PRIu64 " %" PRIu64 "\n", found, (found - called));
    printf("registry: %" PRIu64 " %" PRIu64 "\n", registry, (registry - found));
    printf("committed: %" PRIu64 " %" PRIu64 " %" PRIu64 " (%" PRIu64 ")\n",
           committed,
           (committed - called),
           (committed - registry),
           (redirect - registry));
    printf("mapped: %" PRIu64 " %" PRIu64 " %" PRIu64 " (%" PRIu64 ")\n",
           mapped,
           (mapped - called),
           (mapped - committed),
           (redirect2 - committed));
    printf("h_trigger: %" PRIu64 " %" PRIu64 " %" PRIu64 "\n", triggered, (triggered - called), (triggered - mapped));
    fflush(stdout);
    // #endif
    return trigger_arg.element_seqno;
}

#ifdef USE_RAFT
int dht_topic_is_empty(unsigned long seqno) {
    return seqno <= 1 || seqno == -1;
}

unsigned long dht_topic_latest_seqno(char* topic_name, int timeout) {
    char node_replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH] = {0};
    int node_leader;
    if (dht_find_node(topic_name, node_replicas, &node_leader, timeout) < 0) {
        sprintf(dht_error_msg, "failed to find node hosting the topic");
        return -1;
    }
    char* node_addr = node_replicas[node_leader];
    char registration_woof[DHT_NAME_LENGTH] = {0};
    sprintf(registration_woof, "%s/%s_%s", node_addr, topic_name, DHT_TOPIC_REGISTRATION_WOOF);
    DHT_TOPIC_REGISTRY topic_entry = {0};
    if (get_latest_element(registration_woof, &topic_entry) < 0) {
        char err_msg[DHT_NAME_LENGTH] = {0};
        strcpy(err_msg, dht_error_msg);
        sprintf(dht_error_msg, "failed to get topic registration info from %s: %s", registration_woof, err_msg);
        return -1;
    }

    return dht_remote_topic_latest_seqno(topic_entry.topic_replicas[0], topic_name);
}

unsigned long dht_local_topic_latest_seqno(char* topic_name) {
    return dht_remote_topic_latest_seqno(NULL, topic_name);
}

unsigned long dht_remote_topic_latest_seqno(char* remote_woof, char* topic_name) {
    char index_woof[DHT_NAME_LENGTH] = {0};
    if (remote_woof != NULL) {
        sprintf(index_woof, "%s/%s_%s", remote_woof, topic_name, DHT_MAP_RAFT_INDEX_WOOF_SUFFIX);
    } else {
        sprintf(index_woof, "%s_%s", topic_name, DHT_MAP_RAFT_INDEX_WOOF_SUFFIX);
    }
    return WooFGetLatestSeqno(index_woof);
}

int dht_topic_get(char* topic_name, void* element, unsigned long element_size, unsigned long seqno, int timeout) {
    char node_replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH] = {0};
    int node_leader;
    if (dht_find_node(topic_name, node_replicas, &node_leader, timeout) < 0) {
        sprintf(dht_error_msg, "failed to find node hosting the topic");
        return -1;
    }
    char* node_addr = node_replicas[node_leader];
    char registration_woof[DHT_NAME_LENGTH] = {0};
    sprintf(registration_woof, "%s/%s_%s", node_addr, topic_name, DHT_TOPIC_REGISTRATION_WOOF);
    DHT_TOPIC_REGISTRY topic_entry = {0};
    if (get_latest_element(registration_woof, &topic_entry) < 0) {
        char err_msg[DHT_NAME_LENGTH] = {0};
        strcpy(err_msg, dht_error_msg);
        sprintf(dht_error_msg, "failed to get topic registration info from %s: %s", registration_woof, err_msg);
        return -1;
    }
    return dht_remote_topic_get(topic_entry.topic_replicas[0], topic_name, element, element_size, seqno);
}

int dht_local_topic_get(char* topic_name, void* element, unsigned long element_size, unsigned long seqno) {
    return dht_remote_topic_get(NULL, topic_name, element, element_size, seqno);
}

int dht_remote_topic_get(
    char* remote_woof, char* topic_name, void* element, unsigned long element_size, unsigned long seqno) {
    return dht_remote_topic_get_range(remote_woof, topic_name, element, element_size, seqno, 0, 0);
}

int dht_topic_get_range(char* topic_name,
                        void* element,
                        unsigned long element_size,
                        unsigned long seqno,
                        uint64_t lower_ts,
                        uint64_t upper_ts,
                        int timeout) {
    char node_replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH] = {0};
    int node_leader;
    if (dht_find_node(topic_name, node_replicas, &node_leader, timeout) < 0) {
        sprintf(dht_error_msg, "failed to find node hosting the topic");
        return -1;
    }
    char* node_addr = node_replicas[node_leader];
    char registration_woof[DHT_NAME_LENGTH] = {0};
    sprintf(registration_woof, "%s/%s_%s", node_addr, topic_name, DHT_TOPIC_REGISTRATION_WOOF);
    DHT_TOPIC_REGISTRY topic_entry = {0};
    if (get_latest_element(registration_woof, &topic_entry) < 0) {
        char err_msg[DHT_NAME_LENGTH] = {0};
        strcpy(err_msg, dht_error_msg);
        sprintf(dht_error_msg, "failed to get topic registration info from %s: %s", registration_woof, err_msg);
        return -1;
    }
    return dht_remote_topic_get_range(
        topic_entry.topic_replicas[0], topic_name, element, element_size, seqno, lower_ts, upper_ts);
}

int dht_local_topic_get_range(char* topic_name,
                              void* element,
                              unsigned long element_size,
                              unsigned long seqno,
                              uint64_t lower_ts,
                              uint64_t upper_ts) {
    return dht_remote_topic_get_range(NULL, topic_name, element, element_size, seqno, lower_ts, upper_ts);
}

typedef struct count_el {
    int count;
} COUNT_EL;

int dht_remote_topic_get_range(char* remote_woof,
                               char* topic_name,
                               void* element,
                               unsigned long element_size,
                               unsigned long seqno,
                               uint64_t lower_ts,
                               uint64_t upper_ts) {
    char index_woof[DHT_NAME_LENGTH] = {0};
    if (remote_woof != NULL) {
        sprintf(index_woof, "%s/%s_%s", remote_woof, topic_name, DHT_MAP_RAFT_INDEX_WOOF_SUFFIX);
    } else {
        sprintf(index_woof, "%s_%s", topic_name, DHT_MAP_RAFT_INDEX_WOOF_SUFFIX);
    }
    DHT_MAP_RAFT_INDEX_ARG map_raft_index = {0};
    if (WooFGet(index_woof, &map_raft_index, seqno) < 0) {
        sprintf(dht_error_msg, "failed to get raft index mapping");
        return -1;
    }
    if (map_raft_index.timestamp < lower_ts || (upper_ts != 0 && map_raft_index.timestamp > upper_ts)) {
        sprintf(dht_error_msg, "mapping not in the time range");
        return -2;
    }
    raft_set_client_leader(remote_woof);
    RAFT_DATA_TYPE raft_data = {0};
    unsigned long result = raft_sync_get(&raft_data, map_raft_index.raft_index, 1);
    if (result == RAFT_ERROR) {
        sprintf(dht_error_msg, "failed to read element from raft: %s", raft_error_msg);
        return -1;
    }
    COUNT_EL* c = (COUNT_EL*)&raft_data;
    memcpy(element, raft_data.val, element_size);
    return 0;
}

#endif