#include <stdio.h>
#include <string.h>
#include <time.h>
#include "woofc.h"
#include "dht.h"
#include "dht_utils.h"

int d_daemon(WOOF *wf, unsigned long seq_no, void *ptr) {
	DHT_DAEMON_ARG *arg = (DHT_DAEMON_ARG *)ptr;

	log_set_tag("daemon");
	log_set_level(DHT_LOG_DEBUG);
	log_set_output(stdout);

	unsigned long now = get_milliseconds();
	if (now - arg->last_stablize > DHT_STABLIZE_FREQUENCY) {
		DHT_STABLIZE_ARG stablize_arg;
		unsigned long seq = WooFPut(DHT_STABLIZE_WOOF, "d_stablize", &stablize_arg);
		if (WooFInvalid(seq)) {
			log_error("failed to invoke d_stablize");
		}
		arg->last_stablize = now;
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
			log_error("failed to invoke d_check_predecessor");
		}
		arg->last_fix_finger = now;
		++arg->last_fixed_finger_index;
		if (arg->last_fixed_finger_index > SHA_DIGEST_LENGTH * 8) {
			arg->last_fixed_finger_index = 1;
		}
	}

	usleep(50 * 1000);
	unsigned long seq = WooFPut(DHT_DAEMON_WOOF, "d_daemon", arg);
	if (WooFInvalid(seq)) {
		log_error("failed to invoke next d_daemon");
		exit(1);
	}
	return 1;
}
