#include "dht.h"
#include "dht_utils.h"
#include "raft_client.h"
#include "woofc.h"

#include <stdlib.h>
#include <string.h>

#define MAX_PUBLISH_SIZE 512
#define PUBLISH_TIMEOUT 5000

typedef struct resolve_thread_arg {
    char node_addr[DHT_NAME_LENGTH];
    unsigned long seq_no;
} THREAD_ARG;

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

void* resolve(void* ptr) {
    THREAD_ARG* thread_arg = (THREAD_ARG*)ptr;
    DHT_SERVER_PUBLISH_FIND_ARG find_arg = {0};
    if (WooFGet(DHT_SERVER_PUBLISH_FIND_WOOF, &find_arg, thread_arg->seq_no) < 0) {
        log_error("failed to get find_arg at %lu", thread_arg->seq_no);
        return;
    }
    if (find_arg.element_size > RAFT_DATA_TYPE_SIZE) {
        log_error("element_size %lu exceeds RAFT_DATA_TYPE_SIZE %lu", find_arg.element_size, RAFT_DATA_TYPE_SIZE);
        return;
    }

    uint64_t begin = get_milliseconds();

    char hashed_key[SHA_DIGEST_LENGTH];
    dht_hash(hashed_key, find_arg.topic_name);
    DHT_FIND_SUCCESSOR_ARG arg = {0};
    dht_init_find_arg(&arg, find_arg.topic_name, hashed_key, thread_arg->node_addr);
    arg.action = DHT_ACTION_FIND_NODE;

    unsigned long last_checked_result = WooFGetLatestSeqno(DHT_FIND_NODE_RESULT_WOOF);
    unsigned long seq_no = WooFPut(DHT_FIND_SUCCESSOR_WOOF, NULL, &arg);
    if (WooFInvalid(seq_no)) {
        log_error("failed to invoke h_find_successor");
        return;
    }

    char result_replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH];
    int result_leader = -1;
    int query_count;
    int message_count;
    int failure_count;

    while (result_leader == -1) {
        if (PUBLISH_TIMEOUT > 0 && get_milliseconds() - begin > PUBLISH_TIMEOUT) {
            log_error("timeout after %d ms", PUBLISH_TIMEOUT);
            return;
        }

        unsigned long latest_result = WooFGetLatestSeqno(DHT_FIND_NODE_RESULT_WOOF);
        unsigned long i;
        for (i = last_checked_result + 1; i <= latest_result; ++i) {
            DHT_FIND_NODE_RESULT result = {0};
            if (WooFGet(DHT_FIND_NODE_RESULT_WOOF, &result, i) < 0) {
                log_error("failed to get result at %lu from %s", i, DHT_FIND_NODE_RESULT_WOOF);
                return;
            }
            if (memcmp(result.topic, find_arg.topic_name, SHA_DIGEST_LENGTH) == 0) {
                memcpy(result_replicas, result.node_replicas, sizeof(result.node_replicas));
                result_leader = result.node_leader;
                break;
            }
        }
        last_checked_result = latest_result;
        usleep(50 * 1000);
    }

    // get the topic registry
    DHT_TOPIC_REGISTRY topic_entry = {0};
    char registration_woof[DHT_NAME_LENGTH] = {0};
    char* node_addr;
    int i;
    for (i = 0; i < DHT_REPLICA_NUMBER; ++i) {
        node_addr = result_replicas[(result_leader + i) % DHT_REPLICA_NUMBER];
        // get the topic namespace
        sprintf(registration_woof, "%s/%s_%s", node_addr, find_arg.topic_name, DHT_TOPIC_REGISTRATION_WOOF);
        if (get_latest_element(registration_woof, &topic_entry) < 0) {
            log_error("failed to get topic registration info from %s: %s", registration_woof, dht_error_msg);
            continue;
        }
        break;
    }

    DHT_TRIGGER_ARG trigger_arg = {0};
    trigger_arg.requested = find_arg.requested_ts;
    trigger_arg.found = get_milliseconds();
    strcpy(trigger_arg.topic_name, find_arg.topic_name);
    sprintf(trigger_arg.subscription_woof, "%s/%s_%s", node_addr, find_arg.topic_name, DHT_SUBSCRIPTION_LIST_WOOF);

    // put data to raft
    char* topic_replica;
    unsigned long raft_put_seqno;
    for (i = 0; i < DHT_REPLICA_NUMBER; ++i) {
        topic_replica = topic_entry.topic_replicas[(topic_entry.last_leader + i) % DHT_REPLICA_NUMBER];
        sprintf(trigger_arg.element_woof, "%s", topic_replica);
        RAFT_DATA_TYPE raft_data = {0};
        memcpy(raft_data.val, find_arg.element, find_arg.element_size);
        raft_put_seqno = raft_put(topic_replica, &raft_data, NULL);
        if (raft_is_error(raft_put_seqno)) {
            log_error("failed to put data to raft: %s", raft_error_msg);
            continue;
        }
        log_debug("requested to put data to raft, client_put seqno: %lu", raft_put_seqno);
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
        memcpy(register_arg.topic_namespace, topic_entry.topic_namespace, sizeof(register_arg.topic_namespace));
        memcpy(register_arg.topic_replicas, topic_entry.topic_replicas, sizeof(register_arg.topic_replicas));
        register_arg.topic_leader = topic_entry.last_leader + i;
        if (update_topic_entry(registration_woof, &register_arg) < 0) {
            log_error("failed to update registry's last leader to %d", topic_entry.last_leader + i);
        } else {
            log_debug("updated registry's last leader to %d", topic_entry.last_leader + i);
        }
    }
    // wait for raft_put result
    RAFT_CLIENT_PUT_RESULT raft_put_result = {0};
    while (raft_client_put_result(topic_replica, &raft_put_result, raft_put_seqno) == RAFT_WAITING_RESULT) {
        usleep(10 * 1000);
    }

    trigger_arg.data = get_milliseconds();
    trigger_arg.element_seqno = raft_put_result.index;

    unsigned long seq = WooFPut(DHT_TRIGGER_WOOF, NULL, &trigger_arg);
    if (WooFInvalid(seq)) {
        log_error("failed to queue trigger to %s", DHT_TRIGGER_WOOF);
        return;
    }
    return;
}

int server_publish_single(WOOF* wf, unsigned long seq_no, void* ptr) {
    DHT_LOOP_ROUTINE_ARG* routine_arg = (DHT_LOOP_ROUTINE_ARG*)ptr;

    log_set_tag("server_publish_single");
    log_set_level(DHT_LOG_INFO);
    // log_set_level(DHT_LOG_DEBUG);
    log_set_output(stdout);

    uint64_t begin = get_milliseconds();

    DHT_NODE_INFO node = {0};
    if (get_latest_node_info(&node) < 0) {
        log_error("couldn't get latest node info: %s", dht_error_msg);
        exit(1);
    }

    unsigned long latest_seq = WooFGetLatestSeqno(DHT_SERVER_PUBLISH_FIND_WOOF);
    if (WooFInvalid(latest_seq)) {
        log_error("failed to get the latest seqno from %s", DHT_SERVER_PUBLISH_FIND_WOOF);
        exit(1);
    }
    int count = latest_seq - routine_arg->last_seqno;
    if (count > MAX_PUBLISH_SIZE) {
        log_warn("publish_find lag: %d", count);
        count = MAX_PUBLISH_SIZE;
    }
    if (count != 0) {
        log_debug("processing %lu publish_find", count);
    }

    THREAD_ARG thread_args[count];
    pthread_t tid[count];
    int i;
    for (i = 0; i < count; ++i) {
        memcpy(thread_args[i].node_addr, node.addr, DHT_NAME_LENGTH);
        thread_args[i].seq_no = routine_arg->last_seqno + 1 + i;
        if (pthread_create(&tid[i], NULL, resolve, &thread_args[i]) < 0) {
            log_error("failed to create thread to process publish_find at %lu", routine_arg->last_seqno + 1 + i);
            exit(1);
        }
    }
    threads_join(count, tid);
    routine_arg->last_seqno += count;
    unsigned long seq = WooFPut(DHT_SERVER_LOOP_ROUTINE_WOOF, "server_publish_single", routine_arg);
    if (WooFInvalid(seq)) {
        log_error("failed to queue the next server_publish_single");
        exit(1);
    }
    // printf("handler server_publish_single took %lu\n", get_milliseconds() - begin);
    return 1;
}
