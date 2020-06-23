#include "dht.h"
#include "dht_utils.h"
#include "woofc.h"

#include <string.h>

int h_invalidate_fingers(WOOF* wf, unsigned long seq_no, void* ptr) {
    DHT_INVALIDATE_FINGERS_ARG* arg = (DHT_INVALIDATE_FINGERS_ARG*)ptr;

    log_set_tag("invalidate_fingers");
    log_set_level(DHT_LOG_INFO);
    log_set_level(DHT_LOG_DEBUG);
    log_set_output(stdout);

    char hash_str[DHT_NAME_LENGTH];
    print_node_hash(hash_str, arg->finger_hash);

    int i;
    for (i = 1; i <= SHA_DIGEST_LENGTH * 8; ++i) {
        DHT_FINGER_INFO finger = {0};
        if (get_latest_finger_info(i, &finger) < 0) {
            log_error("failed to get finger[%d]'s info: %s", i, dht_error_msg);
            continue;
        }
        if (memcmp(finger.hash, arg->finger_hash, sizeof(finger.hash)) == 0) {
            memset(&finger, 0, sizeof(finger));
            unsigned long seq = set_finger_info(i, &finger);
            if (WooFInvalid(seq)) {
                log_error("failed to invalidate finger[%d]", i);
                continue;
            }
        }
    }
    log_debug("invalidated all fingers with hash %s", hash_str);
    return 1;
}
