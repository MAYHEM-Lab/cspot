#include "dht.h"
#include "dht_utils.h"
#include "monitor.h"
#include "woofc.h"

#include <stdlib.h>
#include <string.h>

int h_fix_finger_callback(WOOF* wf, unsigned long seq_no, void* ptr) {
    log_set_tag("h_fix_finger_callback");
    log_set_level(DHT_LOG_INFO);
    // log_set_level(DHT_LOG_DEBUG);
    log_set_output(stdout);
    WooFMsgCacheInit();

    DHT_FIX_FINGER_CALLBACK_ARG arg = {0};
    if (monitor_cast(ptr, &arg, sizeof(DHT_FIX_FINGER_CALLBACK_ARG)) < 0) {
        log_error("failed to call monitor_cast");
        monitor_exit(ptr);
        WooFMsgCacheShutdown();
        exit(1);
    }

    log_debug("find_successor leader: %s(%d)", arg.node_replicas[arg.node_leader], arg.node_leader);

    // finger[i] = find_successor(x);
    int i = arg.finger_index;
    DHT_FINGER_INFO finger = {0};
    memcpy(finger.hash, arg.node_hash, sizeof(finger.hash));
    memcpy(finger.replicas, arg.node_replicas, sizeof(finger.replicas));
    finger.leader = arg.node_leader;
    unsigned long seq = set_finger_info(i, &finger);
    if (WooFInvalid(seq)) {
        log_error("failed to update finger[%d]", i);
        monitor_exit(ptr);
        WooFMsgCacheShutdown();
        exit(1);
    }

    char hash_str[2 * SHA_DIGEST_LENGTH + 1];
    print_node_hash(hash_str, finger.hash);
    log_debug("updated finger_addr[%d] = %s(%s)", i, arg.node_replicas[arg.node_leader], hash_str);

    monitor_exit(ptr);
    WooFMsgCacheShutdown();
    return 1;
}
