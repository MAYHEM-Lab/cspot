#define DEBUG

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <openssl/sha.h>

#include "woofc.h"
#include "dht.h"

int h_notify(WOOF *wf, unsigned long seq_no, void *ptr) {
	DHT_NOTIFY_ARG *arg = (DHT_NOTIFY_ARG *)ptr;

	log_set_tag("notify");
	// log_set_level(LOG_DEBUG);
	log_set_level(DHT_LOG_INFO);
	log_set_output(stdout);

	log_debug("potential predecessor_addr: %s", arg->node_addr);
	log_debug("callback: %s/%s", arg->callback_woof, arg->callback_handler);
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

	// if (predecessor is nil or n' ∈ (predecessor, n))
	if (dht_tbl.predecessor_addr[0] == 0
		|| memcmp(dht_tbl.predecessor_hash, dht_tbl.node_hash, SHA_DIGEST_LENGTH) == 0
		|| in_range(arg->node_hash, dht_tbl.predecessor_hash, dht_tbl.node_hash)) {
		if (memcmp(dht_tbl.predecessor_hash, arg->node_hash, SHA_DIGEST_LENGTH) == 0) {
			log_debug("predecessor is the same, no need to update to %s", arg->node_addr);
			return 1;
		}
		memcpy(dht_tbl.predecessor_hash, arg->node_hash, sizeof(dht_tbl.predecessor_hash));
		memcpy(dht_tbl.predecessor_addr, arg->node_addr, sizeof(dht_tbl.predecessor_addr));
		seq = WooFPut(DHT_TABLE_WOOF, NULL, &dht_tbl);
		if (WooFInvalid(seq)) {
			log_error("failed to update predecessor");
			exit(1);
		}
		char hash_str[2 * SHA_DIGEST_LENGTH + 1];
		print_node_hash(hash_str, dht_tbl.predecessor_hash);
		log_info("updated predecessor_hash: %s", hash_str);
		log_info("updated predecessor_addr: %s", dht_tbl.predecessor_addr);
	}

	if (arg->callback_woof[0] == 0) {
		return 1;
	}

	// call notify_callback, where it updates successor list
	DHT_NOTIFY_CALLBACK_ARG result;
	memcpy(result.successor_addr[0], dht_tbl.node_addr, sizeof(result.successor_addr[0]));
	memcpy(result.successor_hash[0], dht_tbl.node_hash, sizeof(result.successor_hash[0]));
	int i;
	for (i = 0; i < DHT_SUCCESSOR_LIST_R - 1; ++i) {
		memcpy(result.successor_addr[i + 1], dht_tbl.successor_addr[i], sizeof(result.successor_addr[i + 1]));
		memcpy(result.successor_hash[i + 1], dht_tbl.successor_hash[i], sizeof(result.successor_hash[i + 1]));
	}

	seq = WooFPut(arg->callback_woof, arg->callback_handler, &result);
	if (WooFInvalid(seq)) {
		log_error("failed to put notify result to woof %s", arg->callback_woof);
		exit(1);
	}
	log_debug("returned successor list");
	
	return 1;
}
