#include "dht.h"
#include "dht_utils.h"
#include "woofc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int r_create_topic(WOOF* wf, unsigned long seq_no, void* ptr) {
    DHT_CREATE_TOPIC_ARG* arg = (DHT_CREATE_TOPIC_ARG*)ptr;

    log_set_tag("r_create_topic");
    log_set_level(DHT_LOG_INFO);
    log_set_level(DHT_LOG_DEBUG);
    log_set_output(stdout);

    if (WooFCreate(arg->topic_name, arg->element_size, arg->history_size) < 0) {
        log_error("failed to create topic: %s %lu %lu", arg->topic_name, arg->element_size, arg->history_size);
        exit(1);
    }
    log_info("topic %s created", arg->topic_name);

    return 1;
}
