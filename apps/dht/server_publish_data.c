#include "dht.h"
#include "dht_client.h"
#include "dht_utils.h"
#include "raft_client.h"
#include "string.h"
#include "woofc.h"

#define MAX_PUBLISH_SIZE 64
#define REGISTRY_CACHE_SIZE 8

pthread_mutex_t cache_lock;

typedef struct resolve_thread_arg {
    char node_addr[DHT_NAME_LENGTH];
    unsigned long seq_no;
} RESOLVE_THREAD_ARG;

int update_topic_entry(char* registration_woof, DHT_REGISTER_TOPIC_ARG* register_arg) {
    char reg_ipaddr[DHT_NAME_LENGTH] = {0};
    if (WooFIPAddrFromURI(registration_woof, reg_ipaddr, DHT_NAME_LENGTH) < 0) {
        log_error("failed to extract woof ip address from %s", registration_woof);
        return -1;
    }
    int reg_port = 0;
    WooFPortFromURI(registration_woof, &reg_port);
    char reg_namespace[DHT_NAME_LENGTH] = {0};
    if (WooFNameSpaceFromURI(registration_woof, reg_namespace, DHT_NAME_LENGTH) < 0) {
        log_error("failed to extract woof namespace from %s", registration_woof);
        return -1;
    }
    char reg_monitor[DHT_NAME_LENGTH] = {0};
    char reg_woof[DHT_NAME_LENGTH] = {0};
    if (reg_port > 0) {
        sprintf(reg_monitor, "woof://%s:%d%s/%s", reg_ipaddr, reg_port, reg_namespace, DHT_MONITOR_NAME);
        sprintf(reg_woof, "woof://%s:%d%s/%s", reg_ipaddr, reg_port, reg_namespace, DHT_REGISTER_TOPIC_WOOF);
    } else {
        sprintf(reg_monitor, "woof://%s%s/%s", reg_ipaddr, reg_namespace, DHT_MONITOR_NAME);
        sprintf(reg_woof, "woof://%s%s/%s", reg_ipaddr, reg_namespace, DHT_REGISTER_TOPIC_WOOF);
    }
    unsigned long seq = monitor_remote_put(reg_monitor, reg_woof, "r_register_topic", register_arg, 0);
    if (WooFInvalid(seq)) {
        return -1;
    }
    return 0;
}

void* resolve_thread(void* arg) {
    RESOLVE_THREAD_ARG* thread_arg = (RESOLVE_THREAD_ARG*)arg;
    DHT_SERVER_PUBLISH_DATA_ARG data_arg = {0};
    if (WooFGet(DHT_SERVER_PUBLISH_DATA_WOOF, &data_arg, thread_arg->seq_no) < 0) {
        log_error("failed to get server_data_find_arg at %lu", thread_arg->seq_no);
        return;
    }

    if (data_arg.update_cache == 1) {
        if (dht_cache_node_put(data_arg.topic_name, data_arg.node_leader, data_arg.node_replicas) < 0) {
            log_error("failed to update topic cache of %s", data_arg.topic_name);
        } else {
            log_debug("updated topic cache of %s", data_arg.topic_name);
        }
    }

    RAFT_DATA_TYPE element = {0};
    if (WooFGet(DHT_SERVER_PUBLISH_ELEMENT_WOOF, &element, data_arg.element_seqno) < 0) {
        log_error(
            "failed to get stored element at %lu from %s", data_arg.element_seqno, DHT_SERVER_PUBLISH_ELEMENT_WOOF);
        return;
    }

    DHT_TRIGGER_ARG trigger_arg = {0};
    trigger_arg.ts_created = data_arg.ts_created;
    trigger_arg.ts_found = data_arg.ts_found;
    trigger_arg.ts_returned = get_milliseconds();

    char registration_woof[DHT_NAME_LENGTH] = {0};
    DHT_TOPIC_REGISTRY topic_entry = {0};
    DHT_REGISTRY_CACHE cache = {0};
    pthread_mutex_lock(&cache_lock);
    if (dht_cache_registry_get(data_arg.topic_name, &cache) > 0) {
        log_debug("found registry cache of %s: %s", data_arg.topic_name, dht_error_msg);
        memcpy(&topic_entry, &cache.registry, sizeof(topic_entry));
        strcpy(registration_woof, cache.registry_woof);
        pthread_mutex_unlock(&cache_lock);
    } else {
        if (dht_get_registry(data_arg.topic_name, &topic_entry, registration_woof) < 0) {
            pthread_mutex_unlock(&cache_lock);
            log_error("failed to get topic registration info %s", dht_error_msg);
            // invalidate topic cache
            if (data_arg.update_cache == 0) { // the topic cache only exists when update_cache == 0
                if (dht_cache_node_invalidate(data_arg.topic_name)) {
                    log_error("failed to invalidate topic cache of %s", data_arg.topic_name);
                } else {
                    log_debug("invalidated topic cache of %s", data_arg.topic_name);
                }

                // find the topic again
                char hashed_key[SHA_DIGEST_LENGTH];
                dht_hash(hashed_key, data_arg.topic_name);
                DHT_FIND_SUCCESSOR_ARG arg = {0};
                dht_init_find_arg(&arg, data_arg.topic_name, hashed_key, thread_arg->node_addr);
                arg.action_seqno = data_arg.element_seqno;
                arg.action = DHT_ACTION_PUBLISH;
                arg.ts_created = data_arg.ts_created;

                unsigned long seq = WooFPut(DHT_FIND_SUCCESSOR_WOOF, NULL, &arg);
                if (WooFInvalid(seq)) {
                    log_error("failed to invoke h_find_successor");
                    return;
                }
            }
            return;
        }
        if (dht_cache_registry_put(data_arg.topic_name, &topic_entry, registration_woof) < 0) {
            log_error("failed to update registry cache of %s: %s", data_arg.topic_name, dht_error_msg);
        }
        pthread_mutex_unlock(&cache_lock);
    }

    memcpy(trigger_arg.element_woof, topic_entry.topic_replicas, sizeof(trigger_arg.element_woof));
    trigger_arg.leader_id = topic_entry.last_leader;
    trigger_arg.ts_registry = get_milliseconds();
    strcpy(trigger_arg.topic_name, data_arg.topic_name);
    sprintf(
        trigger_arg.subscription_woof, "%s/%s_%s", registration_woof, data_arg.topic_name, DHT_SUBSCRIPTION_LIST_WOOF);

    unsigned long trigger_seq = WooFPut(DHT_TRIGGER_WOOF, NULL, &trigger_arg);
    if (WooFInvalid(trigger_seq)) {
        log_error("failed to store partial trigger to %s", DHT_TRIGGER_WOOF);
        return;
    }

#ifdef PROFILING
    printf("FOUND_PROFILE created->found: %lu found->returned: %lu returned->registry: %lu total: %lu\n",
           trigger_arg.ts_found - trigger_arg.ts_created,
           trigger_arg.ts_returned - trigger_arg.ts_found,
           trigger_arg.ts_registry - trigger_arg.ts_returned,
           trigger_arg.ts_registry - trigger_arg.ts_created);
#endif

    // put data to raft
    char* topic_replica;
    int i;
    for (i = 0; i < DHT_REPLICA_NUMBER; ++i) {
        topic_replica = topic_entry.topic_replicas[(topic_entry.last_leader + i) % DHT_REPLICA_NUMBER];

        RAFT_CLIENT_PUT_OPTION opt = {0};
        strcpy(opt.topic_name, data_arg.topic_name);
        sprintf(opt.callback_woof, "%s/%s", thread_arg->node_addr, DHT_SERVER_PUBLISH_TRIGGER_WOOF);
        sprintf(opt.extra_woof, "%s", DHT_TRIGGER_WOOF);
        opt.extra_seqno = trigger_seq;
        unsigned long seq = raft_put(topic_replica, &element, &opt);
        if (raft_is_error(seq)) {
            log_error("failed to put data to raft: %s", raft_error_msg);
            continue;
        }
        // log_debug("requested to put data to raft, client_put seqno: %lu", seq);
        break;
    }
    if (i == DHT_REPLICA_NUMBER) {
        log_error("failed to put data to raft: %s", "none of replicas is working");
        return;
    }
    if (i != 0) {
        // update last_leader
        DHT_REGISTER_TOPIC_ARG register_arg = {0};
        memcpy(register_arg.topic_name, topic_entry.topic_name, sizeof(register_arg.topic_name));
        memcpy(register_arg.topic_replicas, topic_entry.topic_replicas, sizeof(register_arg.topic_replicas));
        register_arg.topic_leader = (topic_entry.last_leader + i) % DHT_REPLICA_NUMBER;
        if (update_topic_entry(registration_woof, &register_arg) < 0) {
            log_error("failed to update registry's last leader to %d", (topic_entry.last_leader + i) % DHT_REPLICA_NUMBER);
        } else {
            log_debug("updated registry's last leader to %d", (topic_entry.last_leader + i) % DHT_REPLICA_NUMBER);
        }
    }
}

int server_publish_data(WOOF* wf, unsigned long seq_no, void* ptr) {
    DHT_LOOP_ROUTINE_ARG* routine_arg = (DHT_LOOP_ROUTINE_ARG*)ptr;
    log_set_tag("server_publish_data");
    log_set_level(DHT_LOG_INFO);
    // log_set_level(DHT_LOG_DEBUG);
    log_set_output(stdout);
    WooFMsgCacheInit();
    zsys_init();

    uint64_t begin = get_milliseconds();
    DHT_NODE_INFO node = {0};
    if (get_latest_node_info(&node) < 0) {
        log_error("couldn't get latest node info: %s", dht_error_msg);
        WooFMsgCacheShutdown();
        exit(1);
    }

    unsigned long latest_seq = WooFGetLatestSeqno(DHT_SERVER_PUBLISH_DATA_WOOF);
    if (WooFInvalid(latest_seq)) {
        log_error("failed to get the latest seqno from %s", DHT_SERVER_PUBLISH_DATA_WOOF);
        WooFMsgCacheShutdown();
        exit(1);
    }

    int count = latest_seq - routine_arg->last_seqno;
    if (count > MAX_PUBLISH_SIZE) {
        log_warn("publish_data lag: %d", count);
        count = MAX_PUBLISH_SIZE;
    }
    if (count != 0) {
        log_debug("processing %d publish_data", count);
    }

    RESOLVE_THREAD_ARG thread_arg[count];
    pthread_t thread_id[count];
    if (pthread_mutex_init(&cache_lock, NULL) < 0) {
        log_error("failed to init mutex");
        WooFMsgCacheShutdown();
        exit(1);
    }

    int i;
    for (i = 0; i < count; ++i) {
        memcpy(thread_arg[i].node_addr, node.addr, sizeof(thread_arg[i].node_addr));
        thread_arg[i].seq_no = routine_arg->last_seqno + 1 + i;
        if (pthread_create(&thread_id[i], NULL, resolve_thread, (void*)&thread_arg[i]) < 0) {
            log_error("failed to create resolve_thread to process publish_data");
            WooFMsgCacheShutdown();
            exit(1);
        }
    }
    routine_arg->last_seqno += count;

    unsigned long seq = WooFPut(DHT_SERVER_LOOP_ROUTINE_WOOF, "server_publish_data", routine_arg);
    if (WooFInvalid(seq)) {
        log_error("failed to queue the next server_publish_data");
        WooFMsgCacheShutdown();
        exit(1);
    }

    uint64_t join_begin = get_milliseconds();
    threads_join(count, thread_id);
    if (get_milliseconds() - join_begin > 5000) {
        log_warn("join tooks %lu ms", get_milliseconds() - join_begin);
    }
    if (count != 0) {
        if (get_milliseconds() - begin > 200)
            log_debug("took %lu ms to process %lu publish_data", get_milliseconds() - begin, count);
    }
    // printf("handler server_publish_data took %lu\n", get_milliseconds() - begin);
    WooFMsgCacheShutdown();
    return 1;
}