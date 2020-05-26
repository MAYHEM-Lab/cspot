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

	char woof_name[DHT_NAME_LENGTH];
	if (node_woof_name(woof_name) < 0) {
		log_error("failed to get local node's woof name");
		exit(1);
	}

	DHT_TABLE dht_table = {0};
	if (get_latest_element(DHT_TABLE_WOOF, &dht_table) < 0) {
		log_error("couldn't get latest dht_table: %s", dht_error_msg);
		exit(1);
	}

	if (is_empty(dht_table.predecessor_hash)) {
		log_debug("predecessor is nil");
		return 1;
	}
	
	DHT_HASHMAP_ENTRY predecessor = {0};
	if (hmap_get(dht_table.predecessor_hash, &predecessor) < 0) {
		log_error("failed to get predecessor's replicas: %s", dht_error_msg);
		exit(1);
	}
	log_debug("checking predecessor: %s", predecessor.replicas[predecessor.leader]);
	char *predecessor_addr = predecessor.replicas[predecessor.leader];

	// check if predecessor woof is working, do nothing
	DHT_GET_PREDECESSOR_ARG arg = {0};
	char predecessor_woof_name[DHT_NAME_LENGTH];
	sprintf(predecessor_woof_name, "%s/%s", predecessor_addr, DHT_GET_PREDECESSOR_WOOF);
	unsigned long seq = WooFPut(predecessor_woof_name, NULL, &arg);
	if (WooFInvalid(seq)) {
		log_warn("failed to access predecessor %s", predecessor_addr);

		// predecessor = nil;
		memset(dht_table.predecessor_hash, 0, sizeof(dht_table.predecessor_hash));
		seq = WooFPut(DHT_TABLE_WOOF, NULL, &dht_table);
		if (WooFInvalid(seq)) {
			log_error("failed to set predecessor to nil");
			exit(1);
		}
		log_warn("set predecessor to nil");
	}
	log_debug("predecessor %s is working", predecessor_addr);
	
	return 1;
}
