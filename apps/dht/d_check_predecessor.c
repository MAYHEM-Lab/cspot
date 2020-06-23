#include "dht.h"
#include "dht_utils.h"
#include "woofc.h"

#include <stdio.h>
#include <string.h>

int d_check_predecessor(WOOF* wf, unsigned long seq_no, void* ptr) {
    log_set_tag("check_predecessor");
    log_set_level(DHT_LOG_INFO);
    // log_set_level(DHT_LOG_DEBUG);
    log_set_output(stdout);

    DHT_PREDECESSOR_INFO predecessor = {0};
    if (get_latest_predecessor_info(&predecessor) < 0) {
        log_error("couldn't get latest predecessor info: %s", dht_error_msg);
        exit(1);
    }

    if (is_empty(predecessor.hash)) {
        log_debug("predecessor is nil");
        return 1;
    }
    log_debug("checking predecessor: %s", predecessor_addr(&predecessor));

    char woof_name[DHT_NAME_LENGTH];
    if (node_woof_name(woof_name) < 0) {
        log_error("failed to get local node's woof name");
        exit(1);
    }

    // check if predecessor woof is working, do nothing
    DHT_GET_PREDECESSOR_ARG get_predecessor_arg = {0};
    char predecessor_woof_name[DHT_NAME_LENGTH];
    sprintf(predecessor_woof_name, "%s/%s", predecessor_addr(&predecessor), DHT_GET_PREDECESSOR_WOOF);
    unsigned long seq = WooFPut(predecessor_woof_name, NULL, &get_predecessor_arg);
    if (WooFInvalid(seq)) {
        // #ifdef USE_RAFT
        //         log_warn("failed to access predecessor %s, try other replicas", predecessor_addr(&predecessor));
        //         DHT_TRY_REPLICAS_ARG try_replicas_arg = {0};
        //         try_replicas_arg.type = DHT_TRY_PREDECESSOR;
        //         seq = WooFPut(DHT_TRY_REPLICAS_WOOF, "r_try_replicas", &try_replicas_arg);
        //         if (WooFInvalid(seq)) {
        //             log_error("failed to invoke r_try_replicas");
        //             exit(1);
        //         }
        // #else
        memset(&predecessor, 0, sizeof(predecessor));
        unsigned long seq = WooFPut(DHT_PREDECESSOR_INFO_WOOF, NULL, &predecessor);
        if (WooFInvalid(seq)) {
            log_error("failed to set predecessor to nil");
            exit(1);
        }
        log_warn("set predecessor to nil");
        // #endif
        return 1;
    }
    log_debug("predecessor %s is working", predecessor_addr(&predecessor));

    return 1;
}
