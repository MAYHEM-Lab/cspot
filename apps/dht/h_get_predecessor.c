#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <openssl/sha.h>
#include "woofc.h"
#include "dht.h"
#include "dht_utils.h"

int h_get_predecessor(WOOF *wf, unsigned long seq_no, void *ptr) {
	DHT_GET_PREDECESSOR_ARG *arg = (DHT_GET_PREDECESSOR_ARG *)ptr;

	log_set_tag("get_predecessor");
	// log_set_level(DHT_LOG_DEBUG);
	log_set_level(DHT_LOG_INFO);
	log_set_output(stdout);

	DHT_TABLE dht_table = {0};
	if (get_latest_element(DHT_TABLE_WOOF, &dht_table) < 0) {
		log_error("couldn't get latest dht_table: %s", dht_error_msg);
		exit(1);
	}
	log_debug("callback_woof: %s", arg->callback_woof);
	log_debug("callback_handler: %s", arg->callback_handler);

	DHT_STABLIZE_CALLBACK_ARG result = {0};
	if (!is_empty(dht_table.predecessor_hash)) {
		DHT_HASHMAP_ENTRY predecessor = {0};
		if (hmap_get(dht_table.predecessor_hash, &predecessor) < 0) {
			log_error("failed to get predecessor's replicas: %s", dht_error_msg);
			exit(1);
		}
		memcpy(result.predecessor_hash, dht_table.predecessor_hash, sizeof(result.predecessor_hash));
		memcpy(result.predecessor_replicas, predecessor.replicas, sizeof(result.predecessor_replicas));
		result.predecessor_leader = predecessor.leader;
	}
	unsigned long seq = WooFPut(arg->callback_woof, arg->callback_handler, &result);
	if (WooFInvalid(seq)){
		log_error("failed to put get_predecessor: result to woof %s", arg->callback_woof);
		exit(1);
	}
	char hash_str[2 * SHA_DIGEST_LENGTH + 1];
	print_node_hash(hash_str, dht_table.predecessor_hash);
	log_debug("returned predecessor_hash: %s", hash_str);
	log_debug("returned predecessor_addr: %s", result.predecessor_replicas[result.predecessor_leader]);

	return 1;
}
