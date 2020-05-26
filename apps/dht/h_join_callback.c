#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <openssl/sha.h>
#include "woofc.h"
#include "dht.h"
#include "dht_utils.h"

int h_join_callback(WOOF *wf, unsigned long seq_no, void *ptr) {
	DHT_JOIN_ARG *arg = (DHT_JOIN_ARG *)ptr;

	log_set_tag("join_callback");
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
	
	log_debug("woof_name: %s", woof_name);
	char hash_str[2 * SHA_DIGEST_LENGTH + 1];
	print_node_hash(hash_str, dht_table.node_hash);
	log_debug("node_hash: %s", hash_str);
	print_node_hash(hash_str, arg->node_hash);
	log_debug("find_successor result hash: %s", hash_str);
	log_debug("find_successor result addr: %s", arg->node_replicas[arg->node_leader]);

	if (hmap_set_replicas(arg->node_hash, arg->node_replicas, arg->node_leader) < 0) {
		log_error("failed to set successor's replica in hashmap: %s", dht_error_msg);
		exit(1);
	}

	memcpy(dht_table.successor_hash[0], arg->node_hash, sizeof(dht_table.successor_hash[0]));
	unsigned long seq = WooFPut(DHT_TABLE_WOOF, NULL, &dht_table);
	if (WooFInvalid(seq)) {
		log_error("failed to update DHT table to woof %s", DHT_TABLE_WOOF);
		exit(1);
	}

	if (dht_start_daemon() < 0) {
		sprintf(dht_error_msg, "failed to start daemon");
		return -1;
	}

	log_info("joined, successor: %s", arg->node_replicas[arg->node_leader]);

	return 1;
}
