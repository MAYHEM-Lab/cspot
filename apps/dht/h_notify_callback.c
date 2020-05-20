#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <openssl/sha.h>
#include "woofc.h"
#include "dht.h"
#include "dht_utils.h"

int h_notify_callback(WOOF *wf, unsigned long seq_no, void *ptr) {
	DHT_NOTIFY_CALLBACK_ARG *result = (DHT_NOTIFY_CALLBACK_ARG *)ptr;

	log_set_tag("notify_callback");
	// log_set_level(HT_LOG_DEBUG);
	log_set_level(DHT_LOG_INFO);
	log_set_output(stdout);

	unsigned long seq = WooFGetLatestSeqno(DHT_TABLE_WOOF);
	if (WooFInvalid(seq))
	{
		log_error("failed to get latest dht_table seq_no");
		exit(1);
	}
	DHT_TABLE dht_table = {0};
	if (WooFGet(DHT_TABLE_WOOF, &dht_table, seq) < 0) {
		log_error("failed to get latest dht_table with seq_no %lu", seq);
		exit(1);
	}

	// set successor list
	memcpy(dht_table.successor_addr, result->successor_addr, sizeof(dht_table.successor_addr));
	memcpy(dht_table.successor_hash, result->successor_hash, sizeof(dht_table.successor_hash));
	seq = WooFPut(DHT_TABLE_WOOF, NULL, &dht_table);
	if (WooFInvalid(seq)) {
		log_error("failed to update successor list");
		exit(1);
	}
	log_debug("updated successor list");
	
	return 1;
}
