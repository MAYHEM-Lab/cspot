#include "dht.h"
#include "dht_utils.h"
#include "woofc.h"
#ifdef USE_RAFT
#include "raft_client.h"
#endif

#include <stdlib.h>
#include <string.h>

int h_notify_callback(WOOF* wf, unsigned long seq_no, void* ptr) {
    DHT_NOTIFY_CALLBACK_ARG* result = (DHT_NOTIFY_CALLBACK_ARG*)ptr;

    log_set_tag("notify_callback");
    log_set_level(DHT_LOG_DEBUG);
    log_set_level(DHT_LOG_INFO);
    log_set_output(stdout);

    DHT_SUCCESSOR_INFO successor = {0};
    if (get_latest_successor_info(&successor) < 0) {
        log_error("couldn't get latest successor info: %s", dht_error_msg);
        exit(1);
    }

    // set successor list
    int i;
    for (i = 0; i < DHT_REPLICA_NUMBER; ++i) {
        if (is_empty(result->successor_hash[i])) {
            break;
        }
    }
    memcpy(successor.hash, result->successor_hash, sizeof(successor.hash));
    memcpy(successor.replicas, result->successor_replicas, sizeof(successor.replicas));
    memcpy(successor.leader, result->successor_leader, sizeof(successor.leader));
#ifdef USE_RAFT
    unsigned long index = raft_put_handler("r_set_successor", &successor, sizeof(DHT_SUCCESSOR_INFO), 0);
    while (index == RAFT_REDIRECTED) {
        log_debug("r_set_successor redirected to %s", raft_client_leader);
        index = raft_put_handler("r_set_successor", &successor, sizeof(DHT_SUCCESSOR_INFO), 0);
    }
    if (raft_is_error(index)) {
        log_error("failed to invoke r_set_successor using raft: %s", raft_error_msg);
        exit(1);
    }
#else
    unsigned long seq = WooFPut(DHT_SUCCESSOR_INFO_WOOF, NULL, &successor);
    if (WooFInvalid(seq)) {
        log_error("failed to update successor list");
        exit(1);
    }
#endif
    log_debug("updated successor list");

    return 1;
}
