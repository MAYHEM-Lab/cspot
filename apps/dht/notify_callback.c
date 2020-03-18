#define DEBUG

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <openssl/sha.h>

#include "woofc.h"
#include "dht.h"

int notify_callback(WOOF *wf, unsigned long seq_no, void *ptr)
{
	// log_set_level(LOG_DEBUG);
	log_set_level(LOG_INFO);
	log_set_output(stdout);

	NOTIFY_RESULT *result = (NOTIFY_RESULT *)ptr;
	int err;
	DHT_TABLE_EL dht_tbl;
	char woof_name[2048];
	unsigned long seq;
	char msg[256];

	err = node_woof_name(woof_name);
	if (err < 0)
	{
		log_error("notify_callback", "couldn't get local node's woof name");
		exit(1);
	}

	seq = WooFGetLatestSeqno(DHT_TABLE_WOOF);
	if (WooFInvalid(seq))
	{
		log_error("notify_callback", "couldn't get latest dht_table seq_no");
		exit(1);
	}
	err = WooFGet(DHT_TABLE_WOOF, &dht_tbl, seq);
	if (err < 0)
	{
		sprintf(msg, "couldn't get latest dht_table with seq_no %lu", seq);
		log_error("notify_callback", msg);
		exit(1);
	}

	// set successor list
	memcpy(dht_tbl.successor_addr, result->successor_addr, sizeof(char) * DHT_SUCCESSOR_LIST_R * 256);
	memcpy(dht_tbl.successor_hash, result->successor_hash, sizeof(unsigned char) * DHT_SUCCESSOR_LIST_R * SHA_DIGEST_LENGTH);
	seq = WooFPut(DHT_TABLE_WOOF, NULL, &dht_tbl);
	if (WooFInvalid(seq))
	{
		log_error("notify_callback", "couldn't update successor list");
		exit(1);
	}
	log_debug("notify_callback", "updated successor list");
	
	return 1;
}
