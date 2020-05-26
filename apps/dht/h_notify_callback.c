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

	DHT_TABLE dht_table = {0};
	if (get_latest_element(DHT_TABLE_WOOF, &dht_table) < 0) {
		log_error("couldn't get latest dht_table: %s", dht_error_msg);
		exit(1);
	}

	// set successor list
	int i;
	for (i = 0; i < DHT_REPLICA_NUMBER; ++i) {
		if (is_empty(result->successor_hash[i])) {
			break;
		}
		if (hmap_set_replicas(result->successor_hash[i], result->successor_replicas[i], result->successor_leader[i]) < 0) {
			log_error("failed to set successor[%d]'s replicas in hashmap: %s", i, dht_error_msg);
			exit(1);
		}
	}
	memcpy(dht_table.successor_hash, result->successor_hash, sizeof(dht_table.successor_hash));
	unsigned long seq = WooFPut(DHT_TABLE_WOOF, NULL, &dht_table);
	if (WooFInvalid(seq)) {
		log_error("failed to update successor list");
		exit(1);
	}
	log_debug("updated successor list");
	
	return 1;
}
