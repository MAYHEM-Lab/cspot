#include "dht.h"
#include "dht_utils.h"
#include "woofc.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#ifdef USE_RAFT
#include "raft.h"
#endif

int d_daemon(WOOF* wf, unsigned long seq_no, void* ptr) {
    DHT_DAEMON_ARG* arg = (DHT_DAEMON_ARG*)ptr;

    log_set_tag("daemon");
    log_set_level(DHT_LOG_DEBUG);
    log_set_level(DHT_LOG_INFO);
    log_set_output(stdout);

#ifdef USE_RAFT
    if (!raft_is_leader()) {
        log_debug("not a raft leader, daemon went to sleep");
        unsigned long seq = WooFPut(DHT_DAEMON_WOOF, "d_daemon", arg);
        if (WooFInvalid(seq)) {
            log_error("failed to invoke next d_daemon");
            exit(1);
        }
        return 1;
    }
#endif

    unsigned long now = get_milliseconds();
    log_debug("since last stabilize: %lums", now - arg->last_stabilize);
    log_debug("since last check_predecessor: %lums", now - arg->last_check_predecessor);
    log_debug("since last fix_finger: %lums", now - arg->last_fix_finger);

    if (now - arg->last_stabilize > DHT_STABILIZE_FREQUENCY) {
        DHT_STABILIZE_ARG stabilize_arg;
        unsigned long seq = WooFPut(DHT_STABILIZE_WOOF, "d_stabilize", &stabilize_arg);
        if (WooFInvalid(seq)) {
            log_error("failed to invoke d_stabilize");
        }
        arg->last_stabilize = now;
    }

    if (now - arg->last_check_predecessor > DHT_CHECK_PREDECESSOR_FRQUENCY) {
        DHT_CHECK_PREDECESSOR_ARG check_predecessor_arg;
        unsigned long seq = WooFPut(DHT_CHECK_PREDECESSOR_WOOF, "d_check_predecessor", &check_predecessor_arg);
        if (WooFInvalid(seq)) {
            log_error("failed to invoke d_check_predecessor");
        }
        arg->last_check_predecessor = now;
    }

    if (now - arg->last_fix_finger > DHT_FIX_FINGER_FRQUENCY) {
        DHT_FIX_FINGER_ARG fix_finger_arg = {0};
        fix_finger_arg.finger_index = arg->last_fixed_finger_index;
        unsigned long seq = WooFPut(DHT_FIX_FINGER_WOOF, "d_fix_finger", &fix_finger_arg);
        if (WooFInvalid(seq)) {
            log_error("failed to invoke d_fix_finger");
        }
        arg->last_fix_finger = now;
        ++arg->last_fixed_finger_index;
        if (arg->last_fixed_finger_index > SHA_DIGEST_LENGTH * 8) {
            arg->last_fixed_finger_index = 1;
        }
    }

#ifdef USE_RAFT
    if (now - arg->last_replicate_state > DHT_REPLICATE_STATE_FREQUENCY) {
        DHT_REPLICATE_STATE_ARG replicate_state_arg;
        unsigned long seq = WooFPut(DHT_REPLICATE_STATE_WOOF, "d_replicate_state", &replicate_state_arg);
        if (WooFInvalid(seq)) {
            log_error("failed to invoke d_replicate_state");
        }
        arg->last_replicate_state = now;
    }
#endif

    // usleep(50 * 1000);
    unsigned long seq = WooFPut(DHT_DAEMON_WOOF, "d_daemon", arg);
    if (WooFInvalid(seq)) {
        log_error("failed to invoke next d_daemon");
        exit(1);
    }
    return 1;
}
