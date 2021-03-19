#include "dht.h"
#include "dht_utils.h"
#include "monitor.h"
#include "woofc-access.h"
#include "woofc.h"

#include <stdlib.h>
#include <string.h>

int h_invalidate_cache(WOOF* wf, unsigned long seq_no, void* ptr) {
    DHT_INVALIDATE_CACHE_ARG* arg = (DHT_INVALIDATE_CACHE_ARG*)ptr;
    log_set_tag("h_invalidate_cache");
    log_set_level(DHT_LOG_INFO);
    // log_set_level(DHT_LOG_DEBUG);
    log_set_output(stdout);

    if (dht_cache_node_invalidate(arg->topic_name) < 0) {
        log_error("failed to invalidate node cache: %s", dht_error_msg);
        exit(1);
    }
    log_debug("invalidate cache of %s", arg->topic_name);
    return 1;
}
