#include <stdio.h>
#include <string.h>
#include "woofc.h"
#include "dht.h"
#include "dht_utils.h"

int h_find_result(WOOF *wf, unsigned long seq_no, void *ptr) {
	DHT_FIND_NODE_RESULT *result = (DHT_FIND_NODE_RESULT *)ptr;

	log_set_tag("find_result");
	// log_set_level(LOG_DEBUG);
	log_set_level(DHT_LOG_INFO);
	log_set_output(stdout);

	char hash_str[2 * SHA_DIGEST_LENGTH + 1];
	print_node_hash(hash_str, result->node_hash);
	log_info("node_addr: %s, node_hash: %s, hops: %d", result->node_addr, hash_str, result->hops);

	return 1;
}
