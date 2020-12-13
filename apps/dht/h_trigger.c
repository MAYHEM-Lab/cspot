#include "dht.h"
#include "dht_utils.h"
#include "monitor.h"
#include "woofc.h"

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

void* resolve_thread(void* arg) {
    DHT_TRIGGER_ARG* thrigger_arg = (DHT_TRIGGER_ARG*)arg;
    DHT_SUBSCRIPTION_LIST list = {0};
    if (get_latest_element(thrigger_arg->subscription_woof, &list) < 0) {
        log_error("failed to get the latest subscription list of %s: %s", thrigger_arg->topic_name, dht_error_msg);
        exit(1);
    }

    DHT_INVOCATION_ARG invocation_arg = {0};
    strcpy(invocation_arg.woof_name, thrigger_arg->element_woof);
    strcpy(invocation_arg.topic_name, thrigger_arg->topic_name);
    invocation_arg.seq_no = thrigger_arg->element_seqno;

    int i, j, k;
    for (i = 0; i < list.size; ++i) {
        for (j = 0; j < DHT_REPLICA_NUMBER; ++j) {
            k = (list.last_successful_replica[i] + j) % DHT_REPLICA_NUMBER;
            if (list.replica_namespaces[i][k][0] == 0) {
                continue;
            }
            char invocation_woof[DHT_NAME_LENGTH];
            sprintf(invocation_woof, "%s/%s", list.replica_namespaces[i][k], DHT_INVOCATION_WOOF);
            // tmp = get_milliseconds();
            unsigned long seq = WooFPut(invocation_woof, list.handlers[i], &invocation_arg);
            if (WooFInvalid(seq)) {
                log_error("failed to trigger handler %s in %s", list.handlers[i], list.replica_namespaces[i][k]);
            } else {
                log_debug("triggered handler %s/%s", list.replica_namespaces[i][k], list.handlers[i]);
                if (k != list.last_successful_replica[i]) {
                    list.last_successful_replica[i] = k;
                    seq = WooFPut(thrigger_arg->subscription_woof, NULL, &list);
                    if (WooFInvalid(seq)) {
                        log_error("failed to update last_successful_replica[%d] to %d", i, k);
                    } else {
                        log_debug("updated last_successful_replica[%d] to %d", i, k);
                    }
                }
                // log_debug("found: %lu", thrigger_arg->found - thrigger_arg->requested);
                // log_debug("data: %lu", thrigger_arg->data - thrigger_arg->found);
                // log_debug("mapped: %lu", thrigger_arg->mapped - thrigger_arg->data);
                // log_debug("triggered: %lu", get_milliseconds() - thrigger_arg->mapped);
                // log_debug("total: %lu", get_milliseconds() - thrigger_arg->requested);
                uint64_t found = thrigger_arg->found - thrigger_arg->requested;
                uint64_t data = thrigger_arg->data - thrigger_arg->found;
                uint64_t mapped = thrigger_arg->mapped - thrigger_arg->data;
                uint64_t triggered = get_milliseconds() - thrigger_arg->mapped;
                uint64_t total = get_milliseconds() - thrigger_arg->requested;
                printf("TRIGGERED: found: %lu data: %lu mapped: %lu triggered: %lu total: %lu\n",
                       found,
                       data,
                       mapped,
                       triggered,
                       total);
                break;
            }
        }
    }
    return 0;
}

int h_trigger(WOOF* wf, unsigned long seq_no, void* ptr) {
    DHT_LOOP_ROUTINE_ARG* routine_arg = (DHT_LOOP_ROUTINE_ARG*)ptr;

    log_set_tag("h_trigger");
    log_set_level(DHT_LOG_INFO);
    // log_set_level(DHT_LOG_DEBUG);
    log_set_output(stdout);

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

    zsys_init();
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
    return 1;
}