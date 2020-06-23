#include "dht.h"
#include "dht_utils.h"
#include "woofc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int r_set_node(WOOF* wf, unsigned long seq_no, void* ptr) {
    DHT_NODE_INFO* arg = (DHT_NODE_INFO*)ptr;

    log_set_tag("set_node");
    // log_set_level(DHT_LOG_DEBUG);
    log_set_level(DHT_LOG_INFO);
    log_set_output(stdout);

    char woof_name[DHT_NAME_LENGTH];
    if (node_woof_name(woof_name) < 0) {
        log_error("failed to get local node's woof name");
        exit(1);
    }

    int i;
    for (i = 0; i < DHT_REPLICA_NUMBER; ++i) {
        if (strcmp(woof_name, arg->replicas[i]) == 0) {
            break;
        }
    }
    if (i == DHT_REPLICA_NUMBER) {
        log_error("node_addr is not part of node_replicas");
        exit(1);
    }
    arg->replica_id = i;

    unsigned long seq = WooFPut(DHT_NODE_INFO_WOOF, NULL, arg);
    if (WooFInvalid(seq)) {
        log_error("failed to set node info");
        exit(1);
    }
    log_debug("set node to %s", arg->addr);

    return 1;
}
