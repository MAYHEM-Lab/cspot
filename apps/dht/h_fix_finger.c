#define DEBUG

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <openssl/sha.h>

#include "woofc.h"
#include "dht.h"

int h_fix_finger(WOOF *wf, unsigned long seq_no, void *ptr) {
	DHT_FIX_FINGER_ARG *arg = (DHT_FIX_FINGER_ARG *)ptr;

	log_set_tag("h_fix_finger");
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
	DHT_TABLE dht_table;
	if (WooFGet(DHT_TABLE_WOOF, &dht_table, seq) < 0) {
		log_error("failed to get latest dht_table with seq_no %lu", seq);
		exit(1);
	}
	log_debug("find_successor addr: %s", arg->node_addr);

	// finger[i] = find_successor(x);
	int i = arg->finger_index;
	// sprintf(msg, "current finger_addr[%d] = %s, %s", i, dht_table.finger_addr[i], result->node_addr);
	// log_info("fix_fingers_callback", msg);
	if (memcmp(dht_table.finger_hash[i], arg->node_hash, SHA_DIGEST_LENGTH) == 0) {
		log_debug("no need to update finger_addr[%d]", i);
		return 1;
	}
	
	memcpy(dht_table.finger_addr[i], arg->node_addr, sizeof(dht_table.finger_addr[i]));
	memcpy(dht_table.finger_hash[i], arg->node_hash, sizeof(dht_table.finger_hash[i]));
	seq = WooFPut(DHT_TABLE_WOOF, NULL, &dht_table);
	if (WooFInvalid(seq)) {
		log_error("failed to update finger[%d]", i);
		exit(1);
	}

	char hash_str[2 * SHA_DIGEST_LENGTH + 1];
	print_node_hash(hash_str, dht_table.finger_hash[i]);
	log_debug("updated finger_addr[%d] = %s(%s)", i, dht_table.finger_addr[i], hash_str);

	return 1;
}
