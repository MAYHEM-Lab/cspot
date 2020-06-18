#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include "woofc.h"
#include "dht.h"
#include "dht_utils.h"

int r_set_finger(WOOF *wf, unsigned long seq_no, void *ptr) {
	DHT_SET_FINGER_ARG *arg = (DHT_SET_FINGER_ARG *)ptr;

	log_set_tag("set_finger");
	// log_set_level(DHT_LOG_DEBUG);
	log_set_level(DHT_LOG_INFO);
	log_set_output(stdout);

	unsigned long seq = set_finger_info(arg->finger_index, &arg->finger);
	if (WooFInvalid(seq)) {
		log_error("failed to set finger[%d]", arg->finger_index);
		exit(1);
	}
	log_debug("set finger[%d] to %s", arg->finger_index, arg->finger.replicas[arg->finger.leader]);

	return 1;
}
