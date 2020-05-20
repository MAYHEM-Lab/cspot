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
	log_set_level(DHT_LOG_DEBUG);
	log_set_output(stdout);

	char woof_name[DHT_NAME_LENGTH];
	if (node_woof_name(woof_name) < 0) {
		log_error("failed to get local node's woof name");
		exit(1);
	}

	// compute the node hash with SHA1
	unsigned char id_hash[SHA_DIGEST_LENGTH];
	SHA1(woof_name, strlen(woof_name), id_hash);
	
	log_debug("woof_name: %s", woof_name);
	char hash_str[2 * SHA_DIGEST_LENGTH + 1];
	print_node_hash(hash_str, id_hash);
	log_debug("node_hash: %s", hash_str);
	print_node_hash(hash_str, arg->node_hash);
	log_debug("find_successor result hash: %s", hash_str);
	log_debug("find_successor result addr: %s", arg->node_addr);

	DHT_TABLE dht_tbl = {0};
	dht_init(id_hash, woof_name, &dht_tbl);
	memcpy(dht_tbl.successor_hash[0], arg->node_hash, sizeof(dht_tbl.successor_hash[0]));
	memcpy(dht_tbl.successor_addr[0], arg->node_addr, sizeof(dht_tbl.successor_addr[0]));
	unsigned long seq = WooFPut(DHT_TABLE_WOOF, NULL, &dht_tbl);
	if (WooFInvalid(seq)) {
		log_error("failed to update DHT table to woof %s", DHT_TABLE_WOOF);
		exit(1);
	}
	log_info("updated predecessor: %s", dht_tbl.predecessor_addr);
	log_info("updated successor: %s", dht_tbl.successor_addr[0]);

	return 1;
}
