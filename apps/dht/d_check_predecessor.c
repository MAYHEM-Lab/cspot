#include <stdio.h>
#include <string.h>
#include "woofc.h"
#include "dht.h"
#include "dht_utils.h"

int d_check_predecessor(WOOF *wf, unsigned long seq_no, void *ptr) {
	log_set_tag("check_predecessor");
	// log_set_level(DHT_LOG_DEBUG);
	log_set_level(DHT_LOG_INFO);
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
	log_debug("checking predecessor: %s", predecessor.replicas[predecessor.leader]);
	int tried = 0;
	while (tried < DHT_REPLICA_NUMBER) {
		char *predecessor_addr = predecessor.replicas[predecessor.leader];
		// check if predecessor woof is working, do nothing
		DHT_GET_PREDECESSOR_ARG get_predecessor_arg = {0};
		char predecessor_woof_name[DHT_NAME_LENGTH];
		sprintf(predecessor_woof_name, "%s/%s", predecessor_addr, DHT_GET_PREDECESSOR_WOOF);
		unsigned long seq = WooFPut(predecessor_woof_name, NULL, &get_predecessor_arg);
		if (WooFInvalid(seq)) {
			log_warn("failed to access predecessor %s", predecessor_addr);
			do {
				predecessor.leader = (predecessor.leader + 1) % DHT_REPLICA_NUMBER;
				++tried;
			} while (predecessor.replicas[predecessor.leader][0] == 0);
			seq = WooFPut(DHT_PREDECESSOR_INFO_WOOF, NULL, &predecessor);
			if (WooFInvalid(seq)) {
				log_error("failed to try the next predecessor replica");
				exit(1);
			}
			log_warn("try next predecessor replica %s", predecessor.replicas[predecessor.leader]);
			continue;
		}
		log_debug("predecessor %s is working", predecessor_addr);
		return 1;
	}

	if (tried >= DHT_REPLICA_NUMBER) {
		// predecessor = nil;
		memset(&predecessor, 0, sizeof(predecessor));
		unsigned long seq = WooFPut(DHT_PREDECESSOR_INFO_WOOF, NULL, &predecessor);
		if (WooFInvalid(seq)) {
			log_error("failed to set predecessor to nil");
			exit(1);
		}
		log_warn("set predecessor to nil");
	}

	return 1;
}
