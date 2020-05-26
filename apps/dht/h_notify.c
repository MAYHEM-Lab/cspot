#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <openssl/sha.h>
#include "woofc.h"
#include "dht.h"
#include "dht_utils.h"

int h_notify(WOOF *wf, unsigned long seq_no, void *ptr) {
	DHT_NOTIFY_ARG *arg = (DHT_NOTIFY_ARG *)ptr;

	log_set_tag("notify");
	// log_set_level(DHT_LOG_DEBUG);
	log_set_level(DHT_LOG_INFO);
	log_set_output(stdout);

	log_debug("potential predecessor_addr: %s", arg->node_replicas[arg->node_leader]);
	DHT_TABLE dht_table = {0};
	if (get_latest_element(DHT_TABLE_WOOF, &dht_table) < 0) {
		log_error("couldn't get latest dht_table: %s", dht_error_msg);
		exit(1);
	}

	// if (predecessor is nil or n' ∈ (predecessor, n))
	if (is_empty(dht_table.predecessor_hash)
		|| memcmp(dht_table.predecessor_hash, dht_table.node_hash, SHA_DIGEST_LENGTH) == 0
		|| in_range(arg->node_hash, dht_table.predecessor_hash, dht_table.node_hash)) {
		if (memcmp(dht_table.predecessor_hash, arg->node_hash, SHA_DIGEST_LENGTH) == 0) {
			log_debug("predecessor is the same, no need to update");
			return 1;
		}
		if (hmap_set_replicas(arg->node_hash, arg->node_replicas, arg->node_leader) < 0) {
			log_error("failed to set predecessor's replicas in hashmap: %s", dht_error_msg);
			exit(1);
		}
		memcpy(dht_table.predecessor_hash, arg->node_hash, sizeof(dht_table.predecessor_hash));
		unsigned long seq = WooFPut(DHT_TABLE_WOOF, NULL, &dht_table);
		if (WooFInvalid(seq)) {
			log_error("failed to update predecessor");
			exit(1);
		}
		char hash_str[2 * SHA_DIGEST_LENGTH + 1];
		print_node_hash(hash_str, dht_table.predecessor_hash);
		log_info("updated predecessor_hash: %s", hash_str);
		log_info("updated predecessor_addr: %s", arg->node_replicas[arg->node_leader]);
	}

	if (arg->callback_woof[0] == 0) {
		return 1;
	}
	log_debug("callback: %s@%s", arg->callback_handler, arg->callback_woof);

	// call notify_callback, where it updates successor list
	DHT_NOTIFY_CALLBACK_ARG result = {0};
	DHT_HASHMAP_ENTRY entry = {0};
	if (hmap_get(dht_table.node_hash, &entry) < 0) {
		log_error("failed to get node's replicas: %s", dht_error_msg);
		exit(1);
	}
	memcpy(result.successor_hash[0], dht_table.node_hash, sizeof(result.successor_hash[0]));
	memcpy(result.successor_replicas[0], entry.replicas, sizeof(result.successor_replicas[0]));
	result.successor_leader[0] = entry.leader;
	int i;
	for (i = 0; i < DHT_SUCCESSOR_LIST_R - 1; ++i) {
		if (is_empty(dht_table.successor_hash[i])) {
			memset(result.successor_hash[i + 1], 0, sizeof(result.successor_hash[i + 1]));
			memset(result.successor_replicas[i + 1], 0, sizeof(result.successor_replicas[i + 1]));
			result.successor_leader[i + 1] = 0;
		} else {
			if (hmap_get(dht_table.successor_hash[i], &entry) < 0) {
				log_error("failed to get successor[%d]'s replicas: %s", i, dht_error_msg);
				exit(1);
			}
			memcpy(result.successor_hash[i + 1], dht_table.successor_hash[i], sizeof(result.successor_hash[i + 1]));
			memcpy(result.successor_replicas[i + 1], entry.replicas, sizeof(result.successor_replicas[i + 1]));
			result.successor_leader[i + 1] = entry.leader;
		}
	}

	unsigned long seq = WooFPut(arg->callback_woof, arg->callback_handler, &result);
	if (WooFInvalid(seq)) {
		log_error("failed to put notify result to woof %s", arg->callback_woof);
		exit(1);
	}
	
	return 1;
}
