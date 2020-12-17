#include "dht.h"
#include "dht_utils.h"
#include "monitor.h"
#include "woofc.h"

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

int update_subscription_list(char* subscription_woof, DHT_SUBSCRIBE_ARG* subscribe_arg) {
    char sub_ipaddr[DHT_NAME_LENGTH] = {0};
    if (WooFIPAddrFromURI(subscription_woof, sub_ipaddr, DHT_NAME_LENGTH) < 0) {
        log_error("failed to extract woof ip address from %s", subscription_woof);
        return -1;
    }
    int sub_port = 0;
    WooFPortFromURI(subscription_woof, &sub_port);
    char sub_namespace[DHT_NAME_LENGTH] = {0};
    if (WooFNameSpaceFromURI(subscription_woof, sub_namespace, DHT_NAME_LENGTH) < 0) {
        log_error("failed to extract woof namespace from %s", subscription_woof);
        return -1;
    }
    char sub_monitor[DHT_NAME_LENGTH] = {0};
    char sub_woof[DHT_NAME_LENGTH] = {0};
    if (sub_port > 0) {
        sprintf(sub_monitor, "woof://%s:%d%s/%s", sub_ipaddr, sub_port, sub_namespace, DHT_MONITOR_NAME);
        sprintf(sub_woof, "woof://%s:%d%s/%s", sub_ipaddr, sub_port, sub_namespace, DHT_SUBSCRIBE_WOOF);
    } else {
        sprintf(sub_monitor, "woof://%s%s/%s", sub_ipaddr, sub_namespace, DHT_MONITOR_NAME);
        sprintf(sub_woof, "woof://%s%s/%s", sub_ipaddr, sub_namespace, DHT_SUBSCRIBE_WOOF);
    }
    unsigned long seq = monitor_remote_put(sub_monitor, sub_woof, "r_subscribe", subscribe_arg, 0);
    if (WooFInvalid(seq)) {
        return -1;
    }
    return 0;
}

void* resolve_thread(void* arg) {
    DHT_TRIGGER_ARG* trigger_arg = (DHT_TRIGGER_ARG*)arg;
    DHT_SUBSCRIPTION_LIST list = {0};
    if (get_latest_element(trigger_arg->subscription_woof, &list) < 0) {
        log_error("failed to get the latest subscription list of %s: %s", trigger_arg->topic_name, dht_error_msg);
        return;
    }

    DHT_INVOCATION_ARG invocation_arg = {0};
    strcpy(invocation_arg.woof_name, trigger_arg->element_woof);
    strcpy(invocation_arg.topic_name, trigger_arg->topic_name);
    invocation_arg.seq_no = trigger_arg->element_seqno;

    int i, j, k;
    for (i = 0; i < list.size; ++i) {
        for (j = 0; j < DHT_REPLICA_NUMBER; ++j) {
            k = (list.last_successful_replica[i] + j) % DHT_REPLICA_NUMBER;
            if (list.replica_namespaces[i][k][0] == 0) {
                continue;
            }
            char invocation_woof[DHT_NAME_LENGTH];
            sprintf(invocation_woof, "%s/%s", list.replica_namespaces[i][k], DHT_INVOCATION_WOOF);
            unsigned long seq = WooFPut(invocation_woof, list.handlers[i], &invocation_arg);
            if (WooFInvalid(seq)) {
                log_error("failed to trigger handler %s in %s", list.handlers[i], list.replica_namespaces[i][k]);
            } else {
                log_debug("triggered handler %s/%s", list.replica_namespaces[i][k], list.handlers[i]);
                if (k != list.last_successful_replica[i]) {
                    DHT_SUBSCRIBE_ARG subscribe_arg = {0};
                    memcpy(subscribe_arg.handler, list.handlers[i], sizeof(subscribe_arg.handler));
                    subscribe_arg.replica_leader = k;
                    memcpy(subscribe_arg.replica_namespaces,
                           list.replica_namespaces[i],
                           sizeof(subscribe_arg.replica_namespaces));
                    memcpy(subscribe_arg.topic_name, invocation_arg.topic_name, sizeof(subscribe_arg.topic_name));
                    if (update_subscription_list(trigger_arg->subscription_woof, &subscribe_arg) < 0) {
                        log_error("failed to update last_successful_replica[%d] to %d", i, k);
                    } else {
                        log_debug("updated last_successful_replica[%d] to %d", i, k);
                    }
                }

                uint64_t found = trigger_arg->found - trigger_arg->requested;
                uint64_t data = trigger_arg->data - trigger_arg->found;
                uint64_t triggered = get_milliseconds() - trigger_arg->data;
                uint64_t total = get_milliseconds() - trigger_arg->requested;
                printf("TRIGGERED: found: %lu data: %lu triggered: %lu total: %lu\n", found, data, triggered, total);
                break;
            }
        }
    }
}

int h_trigger(WOOF* wf, unsigned long seq_no, void* ptr) {
    DHT_LOOP_ROUTINE_ARG* routine_arg = (DHT_LOOP_ROUTINE_ARG*)ptr;
    log_set_tag("h_trigger");
    log_set_level(DHT_LOG_INFO);
    // log_set_level(DHT_LOG_DEBUG);
    log_set_output(stdout);
    zsys_init();

    uint64_t begin = get_milliseconds();

    unsigned long latest_seq = WooFGetLatestSeqno(DHT_TRIGGER_WOOF);
    if (WooFInvalid(latest_seq)) {
        log_error("failed to get the latest seqno from %s", DHT_TRIGGER_WOOF);
        exit(1);
    }
    int count = latest_seq - routine_arg->last_seqno;
    if (count != 0) {
        log_debug("processing %d triggers", count);
    }

    DHT_TRIGGER_ARG thread_arg[count];
    pthread_t thread_id[count];

    int i;
    for (i = 0; i < count; ++i) {
        if (WooFGet(DHT_TRIGGER_WOOF, &thread_arg[i], routine_arg->last_seqno + 1 + i) < 0) {
            log_error("failed to get trigger_arg at %lu", i);
            exit(1);
        }
        if (pthread_create(&thread_id[i], NULL, resolve_thread, (void*)&thread_arg[i]) < 0) {
            log_error("failed to create resolve_thread to process triggers");
            exit(1);
        }
    }
    threads_join(count, thread_id);

    if (count != 0) {
        if (get_milliseconds() - begin > 200)
            log_debug("took %lu ms to process %lu triggers", get_milliseconds() - begin, count);
    }
    routine_arg->last_seqno = latest_seq;
    unsigned long seq = WooFPut(DHT_TRIGGER_ROUTINE_WOOF, "h_trigger", routine_arg);
    if (WooFInvalid(seq)) {
        log_error("failed to queue the next h_trigger");
        exit(1);
    }
    // printf("handler h_trigger took %lu\n", get_milliseconds() - begin);
    return 1;
}