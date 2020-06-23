#include "dht.h"
#include "dht_utils.h"
#include "woofc-access.h"
#include "woofc.h"
#ifdef USE_RAFT
#include "raft.h"
#include "raft_client.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int handler_wrapper(WOOF* wf, unsigned long seq_no, void* ptr) {
    DHT_INVOCATION_ARG* arg = (DHT_INVOCATION_ARG*)ptr;

    log_set_tag("PUT_HANDLER_NAME");
    log_set_level(DHT_LOG_DEBUG);
    // log_set_level(DHT_LOG_INFO);
    log_set_output(stdout);

    char topic_name[DHT_NAME_LENGTH];
    if (WooFNameFromURI(arg->woof_name, topic_name, DHT_NAME_LENGTH) < 0) {
        log_error("failed to get topic name from URI");
        exit(1);
    }

#ifdef USE_RAFT
    log_debug("using raft, getting index %lu from %s", arg->seq_no, arg->woof_name);
    RAFT_DATA_TYPE raft_data = {0};
    raft_set_client_leader(arg->woof_name);
    unsigned long term = raft_sync_get(&raft_data, arg->seq_no, 0);
    if (raft_is_error(term)) {
        log_error("failed to get raft data from %s at %lu: %s", arg->woof_name, arg->seq_no, raft_error_msg);
        exit(1);
    }
    int err = PUT_HANDLER_NAME(topic_name, arg->seq_no, raft_data.val);
#else
    log_debug("using woof, getting seqno %lu from %s", arg->seq_no, arg->woof_name);
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
        log_error("failed to read element from %s at %lu", arg->woof_name, arg->seq_no);
        free(element);
        exit(1);
    }
    int err = PUT_HANDLER_NAME(topic_name, arg->seq_no, element);
    free(element);
#endif

    return err;
}
