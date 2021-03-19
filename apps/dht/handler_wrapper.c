#include "dht.h"
#include "dht_client.h"
#include "dht_utils.h"
#include "raft.h"
#include "raft_client.h"
#include "woofc-access.h"
#include "woofc.h"

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

extern char WooF_dir[2048];

extern int PUT_HANDLER_NAME(char* topic_name, unsigned long seq_no, void* ptr);

int get_client_addr(char* client_addr) {
    DHT_NODE_INFO node = {0};
    if (get_latest_node_info(&node) < 0) {
        sprintf(dht_error_msg, "failed to get the latest dht node info: %s", dht_error_msg);
        return -1;
    }
    int i;
    for (i = strlen("woof://"); i < strlen(node.addr); ++i) {
        if (node.addr[i] == '/') {
            client_addr[i - strlen("woof://")] = '\0';
            return 0;
        }
        client_addr[i - strlen("woof://")] = node.addr[i];
    }
    sprintf(dht_error_msg, "couldn't find the last \"/\" in %s", node.addr);
    return -1;
}

int handler_wrapper(WOOF* wf, unsigned long seq_no, void* ptr) {
    DHT_TRIGGER_ARG* arg = (DHT_TRIGGER_ARG*)ptr;

    log_set_tag("PUT_HANDLER_NAME");
    log_set_level(DHT_LOG_INFO);
    // log_set_level(DHT_LOG_DEBUG);
    log_set_output(stdout);

    // log_debug("using raft, getting index %" PRIu64 " from %s", arg->seq_no, arg->woof_name);
    RAFT_DATA_TYPE raft_data = {0};
    int i, j;
    for (i = 0; i < DHT_REPLICA_NUMBER; ++i) {
        j = (arg->leader_id + i) % DHT_REPLICA_NUMBER;
        if (arg->element_woof[j][0] == 0) {
            continue;
        }
        if (raft_get(arg->element_woof[j], &raft_data, arg->element_seqno) < 0) {
            log_warn("failed to get raft data from %s at %lu, probably not committed yet",
                     arg->element_woof[j],
                     arg->element_seqno);
        } else {
            log_debug("got data from %s[%lu]", arg->element_woof[j], arg->element_seqno);
            break;
        }
    }
    if (i == DHT_REPLICA_NUMBER) {
        log_error("failed to get raft data from any replica at %lu", arg->element_seqno);
        exit(1);
    }

    // unsigned long latest_index = dht_latest_earlier_index(arg->topic_name, arg->element_seqno);
    // if (WooFInvalid(latest_index)) {
    //     log_error("failed to get the latest index: %s", dht_error_msg);
    // }
    // int err = PUT_HANDLER_NAME(arg->topic_name, latest_index, raft_data.val);
    int err = PUT_HANDLER_NAME(arg->topic_name, arg->element_seqno, raft_data.val);
    return err;
}
