#include "dht.h"
#include "dht_utils.h"
#include "monitor.h"
#include "woofc.h"

#include <stdio.h>
#include <string.h>

void get_finger_id(unsigned char* dst, const unsigned char* n, int i) {
    unsigned char carry = 0;
    // dst = n + 2^(i-1)
    int shift = i - 1;
    int j;
    for (j = SHA_DIGEST_LENGTH - 1; j >= 0; j--) {
        dst[j] = carry;
        if (SHA_DIGEST_LENGTH - 1 - (shift / 8) == j) {
            dst[j] += (unsigned char)(1 << (shift % 8));
        }
        if (n[j] >= (256 - dst[j])) {
            carry = 1;
        } else {
            carry = 0;
        }
        dst[j] += n[j];
    }
}

int d_fix_finger(WOOF* wf, unsigned long seq_no, void* ptr) {
    log_set_tag("fix_finger");
    log_set_level(DHT_LOG_INFO);
    // log_set_level(DHT_LOG_DEBUG);
    log_set_output(stdout);

    DHT_NODE_INFO node = {0};
    if (get_latest_node_info(&node) < 0) {
        log_error("couldn't get latest node info: %s", dht_error_msg);
        monitor_exit(ptr);
        exit(1);
    }

    int last_updated_index = 0;
    if (WooFGetLatestSeqno(DHT_FIX_FINGER_CALLBACK_WOOF) > 0) {
        DHT_FIX_FINGER_CALLBACK_ARG last_result = {0};
        if (get_latest_element(DHT_FIX_FINGER_CALLBACK_WOOF, &last_result) < 0) {
            log_error("failed to get the latest fix_finger_callback result: %s", dht_error_msg);
            monitor_exit(ptr);
            exit(1);
        }
        last_updated_index = last_result.finger_index;
    }
    log_debug("last updated finger index: %d", last_updated_index);

    int finger_index = (last_updated_index + 1);
    if (finger_index > SHA_DIGEST_LENGTH * 8) {
        finger_index = 1;
    }
    while (1) {
        // finger[i] = find_successor(n + 2^(i-1))
        // finger_hash = n + 2^(i-1)
        char hashed_finger_id[SHA_DIGEST_LENGTH];
        get_finger_id(hashed_finger_id, node.hash, finger_index);

        if (finger_index > 1) {
            DHT_FINGER_INFO finger = {0};
            if (get_latest_finger_info(finger_index - 1, &finger) < 0) {
                log_error("failed to get finger[%d]'s info: %s", finger_index - 1, dht_error_msg);
                monitor_exit(ptr);
                exit(1);
            }
            // if this finger's hash is covered by the previous finger's range, no need to find_successor
            if (in_range(hashed_finger_id, node.hash, finger.hash) ||
                memcmp(hashed_finger_id, node.hash, SHA_DIGEST_LENGTH) == 0) {
                unsigned long seq = set_finger_info(finger_index, &finger);
                if (WooFInvalid(seq)) {
                    log_error("failed to update finger[%d]", finger_index);
                    monitor_exit(ptr);
                    exit(1);
                }
                // log_debug("use finger[%d] for finger[%d]", finger_index - 1, finger_index);
                ++finger_index;
                if (finger_index > SHA_DIGEST_LENGTH * 8) {
                    finger_index = 1;
                }
                continue;
            }
        }
        DHT_FIND_SUCCESSOR_ARG find_sucessor_arg = {0};
        dht_init_find_arg(&find_sucessor_arg, "", hashed_finger_id, node.addr);
        find_sucessor_arg.action = DHT_ACTION_FIX_FINGER;
        find_sucessor_arg.action_seqno = (unsigned long)finger_index;
        log_debug("fixing finger[%d]", finger_index);

        unsigned long seq = WooFPut(DHT_FIND_SUCCESSOR_WOOF, "h_find_successor", &find_sucessor_arg);
        if (WooFInvalid(seq)) {
            log_error("failed to invoke find_successor on woof %s", DHT_FIND_SUCCESSOR_WOOF);
        }
        monitor_exit(ptr);
        return 1;
    }

    monitor_exit(ptr);
    return 1;
}
