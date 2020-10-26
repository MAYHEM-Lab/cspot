#include "dht.h"
#include "dht_utils.h"
#include "monitor.h"
#include "woofc.h"

#include <stdlib.h>
#include <string.h>

int h_subscribe(WOOF* wf, unsigned long seq_no, void* ptr) {
    log_set_tag("h_subscribe");
    log_set_level(DHT_LOG_INFO);
    log_set_level(DHT_LOG_DEBUG);
    log_set_output(stdout);

    DHT_SUBSCRIBE_ARG arg = {0};
    if (monitor_cast(ptr, &arg, sizeof(DHT_SUBSCRIBE_ARG)) < 0) {
        log_error("failed to call monitor_cast");
        monitor_exit(ptr);
        exit(1);
    }

    char subscription_woof[DHT_NAME_LENGTH];
    sprintf(subscription_woof, "%s_%s", arg.topic_name, DHT_SUBSCRIPTION_LIST_WOOF);
    if (!WooFExist(subscription_woof)) {
        log_error("topic %s doesn't exist", arg.topic_name);
        monitor_exit(ptr);
        exit(1);
    }
    DHT_SUBSCRIPTION_LIST list = {0};
    if (get_latest_element(subscription_woof, &list) < 0) {
        log_error("failed to get latest subscription list of %s: %s", subscription_woof, dht_error_msg);
        monitor_exit(ptr);
        exit(1);
    }

    if (list.size == DHT_MAX_SUBSCRIPTIONS) {
        log_error("maximum number of subscriptions %d has been reached", list.size);
        monitor_exit(ptr);
        return 1;
    }

    int i, j;
    for (i = 0; i < list.size; ++i) {
        if (strcmp(list.handlers[i], arg.handler) == 0) {
            log_info("%s has already subscribed to topic %s", arg.handler, arg.topic_name);
            monitor_exit(ptr);
            return 1;
        }
    }

    memcpy(list.handlers[list.size], arg.handler, sizeof(list.handlers[list.size]));
    memcpy(list.replica_namespaces[list.size], arg.replica_namespaces, sizeof(list.replica_namespaces[list.size]));
    list.last_successful_replica[list.size] = 0;
    list.size += 1;
    log_debug("number of subscription: %d", list.size);
    for (i = 0; i < list.size; ++i) {
        for (j = 0; j < DHT_REPLICA_NUMBER; ++j) {
            log_debug("%s[%d] %s", list.handlers[i], i, list.replica_namespaces[i][j]);
        }
    }

    unsigned long seq = WooFPut(subscription_woof, NULL, &list);
    if (WooFInvalid(seq)) {
        log_error("failed to update subscription list %s", arg.topic_name);
        monitor_exit(ptr);
        exit(1);
    }
    log_info("%s subscribed to topic %s", arg.handler, arg.topic_name);

    monitor_exit(ptr);
    return 1;
}
