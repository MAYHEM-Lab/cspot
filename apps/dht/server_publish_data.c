#include "dht.h"
#include "dht_utils.h"
#include "raft_client.h"
#include "string.h"
#include "woofc.h"

#define MAX_PUBLISH_SIZE 64
#define REGISTRY_CACHE_SIZE 8

typedef struct registry_cache {
    char woof_name[DHT_NAME_LENGTH];
    DHT_TOPIC_REGISTRY topic_entry;
} REGISTRY_CACHE;

REGISTRY_CACHE registry_cache[REGISTRY_CACHE_SIZE];
pthread_mutex_t cache_lock;

int get_registry(char* woof_name) {
    int i;
    for (i = 0; i < REGISTRY_CACHE_SIZE && registry_cache[i].woof_name[0] != 0; ++i) {
        if (strcmp(registry_cache[i].woof_name, woof_name) == 0) {
            return i;
        }
    }
    return -1;
}

int get_free_cache() {
    int i;
    while (i < REGISTRY_CACHE_SIZE && registry_cache[i].woof_name[0] != 0) {
        ++i;
    }
    if (i == REGISTRY_CACHE_SIZE) {
        return -1;
    }
    return i;
}

void put_registry(int cache_id, char* woof_name, DHT_TOPIC_REGISTRY* registry) {
    memcpy(&registry_cache[cache_id].woof_name, woof_name, sizeof(registry_cache[cache_id].woof_name));
    memcpy(&registry_cache[cache_id].topic_entry, registry, sizeof(DHT_TOPIC_REGISTRY));
}

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

    DHT_SERVER_PUBLISH_FIND_ARG find_arg = {0};
    if (WooFGet(DHT_SERVER_PUBLISH_FIND_WOOF, &find_arg, data_arg.find_arg_seqno) < 0) {
        log_error("failed to get server_publish_find_arg at %lu", data_arg.find_arg_seqno);
        return;
    }

    DHT_TRIGGER_ARG trigger_arg = {0};
    trigger_arg.requested = find_arg.ts_a;
    trigger_arg.ts_e = get_milliseconds();

    char registration_woof[DHT_NAME_LENGTH] = {0};
    DHT_TOPIC_REGISTRY topic_entry = {0};
    char* node_addr;
    int i, j;
    for (i = 0; i < DHT_REPLICA_NUMBER; ++i) {
        node_addr = data_arg.node_replicas[(data_arg.node_leader + i) % DHT_REPLICA_NUMBER];
        if (node_addr[0] == 0) {
            continue;
        }
        // get the topic namespace
        sprintf(registration_woof, "%s/%s_%s", node_addr, find_arg.topic_name, DHT_TOPIC_REGISTRATION_WOOF);
        pthread_mutex_lock(&cache_lock);
        int cache_id = get_registry(registration_woof);
        if (cache_id != -1) {
            memcpy(&topic_entry, &registry_cache[cache_id].topic_entry, sizeof(DHT_TOPIC_REGISTRY));
            log_debug("found registry in cache[%d] %s", cache_id, registration_woof);
            pthread_mutex_unlock(&cache_lock);
            break;
        } else {
            cache_id = get_free_cache();
            if (WooFGet(registration_woof, &topic_entry, 0) < 0) {
                log_warn("%s not working: %s", registration_woof, dht_error_msg);
                pthread_mutex_unlock(&cache_lock);
            } else if (cache_id != -1) {
                // found a working replica
                // log_debug("%s works", registration_woof);
                put_registry(cache_id, registration_woof, &topic_entry);
                log_debug("put registry into cache[%d] %s", cache_id, registration_woof);
                pthread_mutex_unlock(&cache_lock);
                break;
            }
        }
    }
    if (i == DHT_REPLICA_NUMBER) {
        char err_msg[DHT_NAME_LENGTH] = {0};
        strcpy(err_msg, dht_error_msg);
        log_error("failed to get topic registration info from %s: %s", node_addr, err_msg);
        return;
    }

    trigger_arg.ts_f = get_milliseconds();
    trigger_arg.found = get_milliseconds();
    strcpy(trigger_arg.topic_name, data_arg.topic_name);
    sprintf(trigger_arg.subscription_woof, "%s/%s_%s", node_addr, data_arg.topic_name, DHT_SUBSCRIPTION_LIST_WOOF);

    unsigned long trigger_seq = WooFPut(DHT_PARTIAL_TRIGGER_WOOF, NULL, &trigger_arg);
    if (WooFInvalid(trigger_seq)) {
        log_error("failed to store partial trigger to %s", DHT_PARTIAL_TRIGGER_WOOF);
        return;
    }
    // log_warn("[%lu] trigger: %lu", thread_arg->seq_no, get_milliseconds() - t); t = get_milliseconds();


    printf("FOUND_PROFILE a->b: %lu, b->c: %lu, c->d: %lu, d->e: %lu, e->f: %lu, total: %lu\n",
           data_arg.ts_b - data_arg.ts_a,
           data_arg.ts_c - data_arg.ts_b,
           data_arg.ts_d - data_arg.ts_c,
           trigger_arg.ts_e - data_arg.ts_d,
           trigger_arg.ts_f - trigger_arg.ts_e,
           trigger_arg.ts_f - data_arg.ts_a);


    // put data to raft
    char* topic_replica;
    for (i = 0; i < DHT_REPLICA_NUMBER; ++i) {
        topic_replica = topic_entry.topic_replicas[(topic_entry.last_leader + i) % DHT_REPLICA_NUMBER];
        sprintf(trigger_arg.element_woof, "%s", topic_replica);
        RAFT_DATA_TYPE raft_data = {0};
        memcpy(raft_data.val, find_arg.element, find_arg.element_size);

        RAFT_CLIENT_PUT_OPTION opt = {0};
        sprintf(opt.callback_woof, "%s/%s", thread_arg->node_addr, DHT_SERVER_PUBLISH_TRIGGER_WOOF);
        sprintf(opt.extra_woof, "%s", DHT_PARTIAL_TRIGGER_WOOF);
        opt.extra_seqno = trigger_seq;
        unsigned long seq = raft_put(topic_replica, &raft_data, &opt);
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
        register_arg.topic_leader = topic_entry.last_leader + i;
        if (update_topic_entry(registration_woof, &register_arg) < 0) {
            log_error("failed to update registry's last leader to %d", topic_entry.last_leader + i);
        } else {
            log_debug("updated registry's last leader to %d", topic_entry.last_leader + i);
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
    memset(registry_cache, 0, sizeof(REGISTRY_CACHE) * REGISTRY_CACHE_SIZE);
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

    threads_join(count, thread_id);
    if (count != 0) {
        if (get_milliseconds() - begin > 200)
            log_debug("took %lu ms to process %lu publish_data", get_milliseconds() - begin, count);
    }
    // printf("handler server_publish_data took %lu\n", get_milliseconds() - begin);
    WooFMsgCacheShutdown();
    return 1;
}