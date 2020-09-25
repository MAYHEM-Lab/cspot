#include "dht.h"
#include "dht_client.h"
#include "dht_utils.h"
#include "woofc-access.h"
#include "woofc.h"
#ifdef USE_RAFT
#include "raft.h"
#include "raft_client.h"
#endif

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern int PUT_HANDLER_NAME(char* woof_name, char* topic_name, unsigned long seq_no, void* ptr);

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
    DHT_INVOCATION_ARG* arg = (DHT_INVOCATION_ARG*)ptr;

    log_set_tag("PUT_HANDLER_NAME");
    log_set_level(DHT_LOG_INFO);
    // log_set_level(DHT_LOG_DEBUG);
    log_set_output(stdout);

    char client_addr[DHT_NAME_LENGTH];
    if (get_client_addr(client_addr) < 0) {
        log_error("failed to get client_addr: %s", dht_error_msg);
        exit(1);
    }
    dht_set_client_ip(client_addr);

#ifdef USE_RAFT
    log_debug("using raft, getting index %" PRIu64 " from %s", arg->seq_no, arg->woof_name);
    RAFT_DATA_TYPE raft_data = {0};
    raft_set_client_leader(arg->woof_name);
    unsigned long term = raft_sync_get(&raft_data, arg->seq_no, 0);
    if (raft_is_error(term)) {
        log_error("failed to get raft data from %s at %" PRIu64 ": %s", arg->woof_name, arg->seq_no, raft_error_msg);
        exit(1);
    }
    int err = PUT_HANDLER_NAME(arg->woof_name, arg->topic_name, arg->seq_no, raft_data.val);
#else
    log_debug("using woof, getting seqno %" PRIu64 " from %s", arg->seq_no, arg->woof_name);
    unsigned long element_size = WooFMsgGetElSize(arg->woof_name);
    if (WooFInvalid(element_size)) {
        log_error("failed to get element size of %s", arg->woof_name);
        exit(1);
    }

    void* element = malloc(element_size);
    if (element == NULL) {
        log_error("failed to malloc element with size %lu", element_size);
        exit(1);
    }
    if (WooFGet(arg->woof_name, element, arg->seq_no) < 0) {
        log_error("failed to read element from %s at %" PRIu64 "", arg->woof_name, arg->seq_no);
        free(element);
        exit(1);
    }

    int err = PUT_HANDLER_NAME(arg->woof_name, arg->topic_name, arg->seq_no, element);
    free(element);
#endif

    return err;
}
