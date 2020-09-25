#include "dht.h"
#include "dht_utils.h"
#include "monitor.h"
#include "woofc.h"

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

    DHT_NODE_INFO node = {0};
    if (get_latest_node_info(&node) < 0) {
        log_error("couldn't get latest node info: %s", dht_error_msg);
        exit(1);
    }
    DHT_SUBSCRIPTION_LIST list = {0};
    if (get_latest_element(arg->subscription_woof, &list) < 0) {
        log_error("failed to get the latest subscription list of %s: %s", arg->topic_name, dht_error_msg);
        exit(1);
    }
    BLOCKED_NODES blocked_nodes = {0};
    if (get_latest_element(BLOCKED_NODES_WOOF, &blocked_nodes) < 0) {
        log_error("failed to get blocked nodes");
    }
    FAILURE_RATE failure_rate = {0};
    if (get_latest_element(FAILURE_RATE_WOOF, &failure_rate) < 0) {
        log_error("failed to get failure rate");
    }

    int i;
    for (i = 0; i < list.size; ++i) {
        DHT_INVOCATION_ARG invocation_arg = {0};
        strcpy(invocation_arg.woof_name, arg->element_woof);
        strcpy(invocation_arg.topic_name, arg->topic_name);
        invocation_arg.seq_no = arg->element_seqno;
        log_debug("woof_name: %s, topic_name: %s, seq_no: %lu",
                  invocation_arg.woof_name,
                  invocation_arg.topic_name,
                  invocation_arg.seq_no);

        char invocation_woof[DHT_NAME_LENGTH];
        sprintf(invocation_woof, "%s/%s", list.handler_namespace[i], DHT_INVOCATION_WOOF);
        unsigned long seq = -1;
        if (!is_blocked(invocation_woof, node.addr, blocked_nodes, failure_rate)) {
            seq = WooFPut(invocation_woof, list.handlers[i], &invocation_arg);
        }
        if (WooFInvalid(seq)) {
            log_error("failed to trigger handler %s in %s", list.handlers[i], list.handler_namespace[i]);
        } else {
            log_debug("triggered handler %s/%s", list.handler_namespace[i], list.handlers[i]);
        }
    }

    return 1;
}
