#define DEBUG

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <openssl/sha.h>

#include "woofc.h"
#include "dht.h"

int fix_fingers_callback(WOOF *wf, unsigned long seq_no, void *ptr)
{
	// log_set_level(LOG_DEBUG);
	log_set_level(LOG_INFO);
	log_set_output(stdout);

	FIND_SUCESSOR_RESULT *result = (FIND_SUCESSOR_RESULT *)ptr;
	int err;
	int i;
	DHT_TABLE_EL dht_tbl;
	char woof_name[2048];
	char notify_woof_name[2048];
	unsigned char target_hash[SHA_DIGEST_LENGTH];
	unsigned long seq;
	NOTIFY_ARG notify_arg;
	char msg[256];

	err = node_woof_name(woof_name);
	if (err < 0)
	{
		log_error("fix_fingers_callback", "couldn't get local node's woof name");
		exit(1);
	}

	seq = WooFGetLatestSeqno(DHT_TABLE_WOOF);
	if (WooFInvalid(seq))
	{
		log_error("fix_fingers_callback", "couldn't get latest dht_table seq_no");
		exit(1);
	}
	err = WooFGet(DHT_TABLE_WOOF, &dht_tbl, seq);
	if (err < 0)
	{
		sprintf(msg, "couldn't get latest dht_table with seq_no %lu", seq);
		log_error("fix_fingers_callback", msg);
		exit(1);
	}

	sprintf(msg, "find_successor addr: %s", result->node_addr);
	log_debug("fix_fingers_callback", msg);

	// finger[i] = find_successor(x);
	i = result->finger_index;
	// sprintf(msg, "current finger_addr[%d] = %s, %s", i, dht_tbl.finger_addr[i], result->node_addr);
	// log_info("fix_fingers_callback", msg);
	if (memcmp(dht_tbl.finger_hash[i], result->node_hash, SHA_DIGEST_LENGTH) == 0)
	{
		sprintf(msg, "no need to update finger_addr[%d]", i);
		log_debug("fix_fingers_callback", msg);
		return 1;
	}
	
	strncpy(dht_tbl.finger_addr[i], result->node_addr, sizeof(dht_tbl.finger_addr[i]));
	memcpy(dht_tbl.finger_hash[i], result->node_hash, SHA_DIGEST_LENGTH);
	seq = WooFPut(DHT_TABLE_WOOF, NULL, &dht_tbl);
	if (WooFInvalid(seq))
	{
		sprintf(msg, "couldn't update finger[%d]", i);
		log_error("fix_fingers_callback", msg);
		exit(1);
	}

	print_node_hash(target_hash, result->target_hash);
	sprintf(msg, "updated finger_addr[%d](%s) = %s(", i, target_hash, dht_tbl.finger_addr[i]);
	print_node_hash(msg + strlen(msg), dht_tbl.finger_hash[i]);
	log_info("fix_fingers_callback", msg);

	return 1;
}
