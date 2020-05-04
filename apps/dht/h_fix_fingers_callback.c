#define DEBUG

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <openssl/sha.h>

#include "woofc.h"
#include "dht.h"

int h_fix_fingers_callback(WOOF *wf, unsigned long seq_no, void *ptr) {
	FIND_SUCESSOR_RESULT *result = (FIND_SUCESSOR_RESULT *)ptr;

	log_set_tag("fix_fingers_callback");
	// log_set_level(LOG_DEBUG);
	log_set_level(LOG_INFO);
	log_set_output(stdout);

	char woof_name[DHT_NAME_LENGTH];
	if (node_woof_name(woof_name) < 0) {
		log_error("couldn't get local node's woof name");
		exit(1);
	}

	unsigned long seq = WooFGetLatestSeqno(DHT_TABLE_WOOF);
	if (WooFInvalid(seq)) {
		log_error("couldn't get latest dht_table seq_no");
		exit(1);
	}
	DHT_TABLE_EL dht_tbl;
	if (WooFGet(DHT_TABLE_WOOF, &dht_tbl, seq) < 0) {
		log_error("couldn't get latest dht_table with seq_no %lu", seq);
		exit(1);
	}
	log_debug("find_successor addr: %s", result->node_addr);

	// finger[i] = find_successor(x);
	int i = result->finger_index;
	// sprintf(msg, "current finger_addr[%d] = %s, %s", i, dht_tbl.finger_addr[i], result->node_addr);
	// log_info("fix_fingers_callback", msg);
	if (memcmp(dht_tbl.finger_hash[i], result->node_hash, SHA_DIGEST_LENGTH) == 0) {
		log_debug("no need to update finger_addr[%d]", i);
		return 1;
	}
	
	memcpy(dht_tbl.finger_addr[i], result->node_addr, sizeof(dht_tbl.finger_addr[i]));
	memcpy(dht_tbl.finger_hash[i], result->node_hash, sizeof(dht_tbl.finger_hash[i]));
	seq = WooFPut(DHT_TABLE_WOOF, NULL, &dht_tbl);
	if (WooFInvalid(seq)) {
		log_error("couldn't update finger[%d]", i);
		exit(1);
	}

	unsigned char target_hash[SHA_DIGEST_LENGTH];
	print_node_hash(target_hash, result->target_hash);
	char msg[256];
	sprintf(msg, "updated finger_addr[%d](%s) = %s(", i, target_hash, dht_tbl.finger_addr[i]);
	print_node_hash(msg + strlen(msg), dht_tbl.finger_hash[i]);
	log_debug(msg);

	return 1;
}
