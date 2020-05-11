#define DEBUG

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <openssl/sha.h>

#include "woofc.h"
#include "dht.h"

int h_stablize_callback(WOOF *wf, unsigned long seq_no, void *ptr) {
	DHT_STABLIZE_CALLBACK *result = (DHT_STABLIZE_CALLBACK *)ptr;

	log_set_tag("stablize_callback");
	// log_set_level(LOG_DEBUG);
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
	DHT_TABLE dht_tbl;
	if (WooFGet(DHT_TABLE_WOOF, &dht_tbl, seq) < 0) {
		log_error("failed to get latest dht_table with seq_no %lu", seq);
		exit(1);
	}

	// compute the node hash with SHA1
	unsigned char id_hash[SHA_DIGEST_LENGTH];
	SHA1(woof_name, strlen(woof_name), id_hash);
	log_debug("get_predecessor addr: %s", result->predecessor_addr);

	// x = successor.predecessor
	// if (x ∈ (n, successor))
	if (result->predecessor_addr[0] != 0 && 
		in_range(result->predecessor_hash, dht_tbl.node_hash, dht_tbl.successor_hash[0])) {
		// successor = x;
		if (memcmp(dht_tbl.successor_hash[0], result->predecessor_hash, SHA_DIGEST_LENGTH) != 0) {
			memcpy(dht_tbl.successor_hash[0], result->predecessor_hash, sizeof(dht_tbl.successor_hash[0]));
			memcpy(dht_tbl.successor_addr[0], result->predecessor_addr, sizeof(dht_tbl.successor_addr[0]));
			seq = WooFPut(DHT_TABLE_WOOF, NULL, &dht_tbl);
			if (WooFInvalid(seq)) {
				log_error("failed to update DHT table to %s", DHT_TABLE_WOOF);
				exit(1);
			}
			log_info("updated successor to %s", dht_tbl.successor_addr[0]);
		}
	}

	// successor.notify(n);
	char notify_woof_name[DHT_NAME_LENGTH];
	sprintf(notify_woof_name, "%s/%s", dht_tbl.successor_addr[0], DHT_NOTIFY_WOOF);
	NOTIFY_ARG notify_arg;
	memcpy(notify_arg.node_hash, id_hash, sizeof(notify_arg.node_hash));
	strncpy(notify_arg.node_addr, woof_name, sizeof(notify_arg.node_addr));
	sprintf(notify_arg.callback_woof, "%s/%s", woof_name, DHT_NOTIFY_CALLBACK_WOOF);
	sprintf(notify_arg.callback_handler, "h_notify_callback");
	seq = WooFPut(notify_woof_name, "h_notify", &notify_arg);
	if (WooFInvalid(seq)) {
		log_error("failed to call notify on successor %s", notify_woof_name);
		exit(1);
	}
	log_debug("called notify on successor %s", dht_tbl.successor_addr[0]);

	return 1;
}
