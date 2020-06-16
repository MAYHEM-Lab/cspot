#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include "woofc.h"
#include "dht.h"
#include "dht_utils.h"

int r_set_predecessor(WOOF *wf, unsigned long seq_no, void *ptr) {
	DHT_PREDECESSOR_INFO *arg = (DHT_PREDECESSOR_INFO *)ptr;

	log_set_tag("set_predecessor");
	// log_set_level(DHT_LOG_DEBUG);
	log_set_level(DHT_LOG_INFO);
	log_set_output(stdout);

	unsigned long seq = WooFPut(DHT_PREDECESSOR_INFO_WOOF, NULL, arg);
	if (WooFInvalid(seq)) {
		log_error("failed to set predecessor");
		exit(1);
	}
	log_debug("set predecessor to %s", arg->replicas[arg->leader]);

	return 1;
}
