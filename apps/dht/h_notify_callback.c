#define DEBUG

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <openssl/sha.h>

#include "woofc.h"
#include "dht.h"

int h_notify_callback(WOOF *wf, unsigned long seq_no, void *ptr) {
	NOTIFY_RESULT *result = (NOTIFY_RESULT *)ptr;

	log_set_tag("notify_callback");
	// log_set_level(LOG_DEBUG);
	log_set_level(LOG_INFO);
	log_set_output(stdout);

	char woof_name[DHT_NAME_LENGTH];
	if (node_woof_name(woof_name) < 0) {
		log_error("couldn't get local node's woof name");
		exit(1);
	}

	unsigned long seq = WooFGetLatestSeqno(DHT_TABLE_WOOF);
	if (WooFInvalid(seq))
	{
		log_error("couldn't get latest dht_table seq_no");
		exit(1);
	}
	DHT_TABLE_EL dht_tbl;
	if (WooFGet(DHT_TABLE_WOOF, &dht_tbl, seq) < 0) {
		log_error("couldn't get latest dht_table with seq_no %lu", seq);
		exit(1);
	}

	// set successor list
	memcpy(dht_tbl.successor_addr, result->successor_addr, sizeof(dht_tbl.successor_addr));
	memcpy(dht_tbl.successor_hash, result->successor_hash, sizeof(dht_tbl.successor_hash));
	seq = WooFPut(DHT_TABLE_WOOF, NULL, &dht_tbl);
	if (WooFInvalid(seq)) {
		log_error("couldn't update successor list");
		exit(1);
	}
	log_debug("updated successor list");
	
	return 1;
}
