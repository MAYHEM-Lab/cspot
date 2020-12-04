#include "dht.h"
#include "dht_utils.h"
#include "monitor.h"
#include "woofc.h"

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

int h_trigger(WOOF* wf, unsigned long seq_no, void* ptr) {
    DHT_TRIGGER_ARG* arg = (DHT_TRIGGER_ARG*)ptr;

    log_set_tag("h_trigger");
    log_set_level(DHT_LOG_INFO);
    // log_set_level(DHT_LOG_DEBUG);
    log_set_output(stdout);

#ifdef USE_RAFT
    log_debug("topic: %s, raft: %s, index: %lu", arg->topic_name, arg->element_woof, arg->element_seqno);
#else
    log_debug("topic: %s, woof: %s, seqno: %lu", arg->topic_name, arg->element_woof, arg->element_seqno);
#endif
    uint64_t begin = get_milliseconds();
    // uint64_t tmp;
    DHT_NODE_INFO node = {0};
    if (get_latest_node_info(&node) < 0) {
        log_error("couldn't get latest node info: %s", dht_error_msg);
        exit(1);
    }
    // tmp = get_milliseconds();
    DHT_SUBSCRIPTION_LIST list = {0};
    if (get_latest_element(arg->subscription_woof, &list) < 0) {
        log_error("failed to get the latest subscription list of %s: %s", arg->topic_name, dht_error_msg);
        exit(1);
    }
    // log_debug("list: %lu, %s", get_milliseconds() - tmp, arg->subscription_woof);
    BLOCKED_NODES blocked_nodes = {0};
    if (get_latest_element(BLOCKED_NODES_WOOF, &blocked_nodes) < 0) {
        log_error("failed to get blocked nodes");
    }

    int i, j, k;
    for (i = 0; i < list.size; ++i) {
        DHT_INVOCATION_ARG invocation_arg = {0};
        strcpy(invocation_arg.woof_name, arg->element_woof);
        strcpy(invocation_arg.topic_name, arg->topic_name);
        invocation_arg.seq_no = arg->element_seqno;
        // log_debug("woof_name: %s, topic_name: %s, seq_no: %lu",
        //           invocation_arg.woof_name,
        //           invocation_arg.topic_name,
        //           invocation_arg.seq_no);
        for (j = 0; j < DHT_REPLICA_NUMBER; ++j) {
            k = (list.last_successful_replica[i] + j) % DHT_REPLICA_NUMBER;
            if (list.replica_namespaces[i][k][0] == 0) {
                continue;
            }
            char invocation_woof[DHT_NAME_LENGTH];
            sprintf(invocation_woof, "%s/%s", list.replica_namespaces[i][k], DHT_INVOCATION_WOOF);
            // tmp = get_milliseconds();
            unsigned long seq =
                checkedWooFPut(&blocked_nodes, node.addr, invocation_woof, list.handlers[i], &invocation_arg);
            if (WooFInvalid(seq)) {
                log_error("failed to trigger handler %s in %s", list.handlers[i], list.replica_namespaces[i][k]);
            } else {
                log_debug("triggered handler %s/%s", list.replica_namespaces[i][k], list.handlers[i]);
                if (k != list.last_successful_replica[i]) {
                    list.last_successful_replica[i] = k;
                    seq = WooFPut(arg->subscription_woof, NULL, &list);
                    if (WooFInvalid(seq)) {
                        log_error("failed to update last_successful_replica[%d] to %d", i, k);
                    } else {
                        log_debug("updated last_successful_replica[%d] to %d", i, k);
                    }
                }
                break;
            }
        }
    }
    uint64_t end = get_milliseconds();
    log_debug("in h_trigger: %" PRIu64 " ms", end - begin);
    return 1;
}
