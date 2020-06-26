#include "dht.h"
#include "dht_utils.h"
#include "monitor.h"
#include "woofc.h"

#include <stdlib.h>
#include <string.h>

int h_subscribe(WOOF* wf, unsigned long seq_no, void* ptr) {
    DHT_SUBSCRIBE_ARG* arg = (DHT_SUBSCRIBE_ARG*)ptr;

    log_set_tag("subscribe");
    log_set_level(DHT_LOG_DEBUG);
    // log_set_level(DHT_LOG_INFO);
    log_set_output(stdout);

    char subscription_woof[DHT_NAME_LENGTH];
    sprintf(subscription_woof, "%s_%s", arg->topic_name, DHT_SUBSCRIPTION_LIST_WOOF);
    if (!WooFExist(subscription_woof)) {
        log_error("topic %s doesn't exist", arg->topic_name);
        exit(1);
    }
    DHT_SUBSCRIPTION_LIST list = {0};
    if (get_latest_element(subscription_woof, &list) < 0) {
        log_error("failed to get latest subscription list of %s: %s", subscription_woof, dht_error_msg);
        exit(1);
    }

    if (list.size == DHT_MAX_SUBSCRIPTIONS) {
        log_error("maximum number of subscriptions %d has been reached", list.size);
        return 1;
    }

    memcpy(list.handlers[list.size], arg->handler, sizeof(list.handlers[list.size]));
    memcpy(list.namespace[list.size], arg -> handler_namespace, sizeof(list.namespace[list.size]));
    list.size += 1;
    log_debug("number of subscription: %d", list.size);
    int i;
    for (i = 0; i < list.size; ++i) {
        log_debug("[%d] %s/%s", i, list.namespace[i], list.handlers[i]);
    }

    unsigned long seq = WooFPut(subscription_woof, NULL, &list);
    if (WooFInvalid(seq)) {
        log_error("failed to update subscription list %s", arg->topic_name);
        exit(1);
    }
    log_info("%s/%s subscribed to topic %s", arg->handler_namespace, arg->handler, arg->topic_name);

    return 1;
}
