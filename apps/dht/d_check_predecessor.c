#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "dht.h"

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

	unsigned long seq = WooFGetLatestSeqno(DHT_TABLE_WOOF);
	if (WooFInvalid(seq)) {
		log_error("failed to get latest dht_table seq_no");
		exit(1);
	}
	DHT_TABLE dht_table;
	if (WooFGet(DHT_TABLE_WOOF, &dht_table, seq) < 0) {
		log_error("failed to get latest dht_table with seq_no %lu", seq);
		exit(1);
	}

	// log_debug("check_predecessor", msg);
	log_debug("current predecessor_addr: %s", dht_table.predecessor_addr);
	if (dht_table.predecessor_addr[0] == 0) {
		log_debug("predecessor is nil");
		return 1;
	}
	log_debug("checking predecessor: %s", dht_table.predecessor_addr);

	// check if predecessor woof is working, do nothing
	DHT_GET_PREDECESSOR_ARG arg;
	char predecessor_woof_name[DHT_NAME_LENGTH];
	sprintf(predecessor_woof_name, "%s/%s", dht_table.predecessor_addr, DHT_GET_PREDECESSOR_WOOF);
	seq = WooFPut(predecessor_woof_name, NULL, &arg);
	if (WooFInvalid(seq)) {
		log_warn("failed to access predecessor %s", dht_table.predecessor_addr);

		// predecessor = nil;
		memset(dht_table.predecessor_addr, 0, sizeof(dht_table.predecessor_addr));
		memset(dht_table.predecessor_hash, 0, sizeof(dht_table.predecessor_hash));
		seq = WooFPut(DHT_TABLE_WOOF, NULL, &dht_table);
		if (WooFInvalid(seq)) {
			log_error("failed to set predecessor to nil");
			exit(1);
		}
		log_warn("set predecessor to nil");
	}
	log_debug("predecessor %s is working", dht_table.predecessor_addr);
	
	return 1;
}
