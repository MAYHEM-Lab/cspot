#define DEBUG

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <openssl/sha.h>

#include "woofc.h"
#include "dht.h"

int stablize_callback(WOOF *wf, unsigned long seq_no, void *ptr)
{
	// log_set_level(LOG_DEBUG);
	log_set_level(LOG_INFO);
	log_set_output(stdout);

	GET_PREDECESSOR_RESULT *result = (GET_PREDECESSOR_RESULT *)ptr;
	int err;
	int i;
	DHT_TABLE_EL dht_tbl;
	char woof_name[2048];
	char notify_woof_name[2048];
	unsigned char id_hash[SHA_DIGEST_LENGTH];
	unsigned long seq;
	NOTIFY_ARG notify_arg;
	char msg[256];

	err = node_woof_name(woof_name);
	if (err < 0)
	{
		log_error("stablize_callback", "couldn't get local node's woof name");
		exit(1);
	}

	seq = WooFGetLatestSeqno(DHT_TABLE_WOOF);
	if (WooFInvalid(seq))
	{
		log_error("stablize_callback", "couldn't get latest dht_table seq_no");
		exit(1);
	}
	err = WooFGet(DHT_TABLE_WOOF, &dht_tbl, seq);
	if (err < 0)
	{
		sprintf(msg, "couldn't get latest dht_table with seq_no %lu", seq);
		log_error("stablize_callback", msg);
		exit(1);
	}

	// compute the node hash with SHA1
	SHA1(woof_name, strlen(woof_name), id_hash);

	sprintf(msg, "get_predecessor addr: %s", result->predecessor_addr);
	log_debug("stablize_callback", msg);

	// x = successor.predecessor
	// if (x ∈ (n, successor))
	if (result->predecessor_addr[0] != 0 && 
		in_range(result->predecessor_hash, dht_tbl.node_hash, dht_tbl.successor_hash[0]))
	{
		// successor = x;
		if (memcmp(dht_tbl.successor_hash[0], result->predecessor_hash, SHA_DIGEST_LENGTH) != 0)
		{
			memcpy(dht_tbl.successor_hash[0], result->predecessor_hash, sizeof(dht_tbl.successor_hash[0]));
			strncpy(dht_tbl.successor_addr[0], result->predecessor_addr, sizeof(dht_tbl.successor_addr[0]));
			seq = WooFPut(DHT_TABLE_WOOF, NULL, &dht_tbl);
			if (WooFInvalid(seq))
			{
				sprintf(msg, "couldn't update DHT table to %s", DHT_TABLE_WOOF);
				log_error("stablize_callback", msg);
				exit(1);
			}
			sprintf(msg, "updated successor to %s", dht_tbl.successor_addr[0]);
			log_info("stablize_callback", msg);
		}
	}

	// successor.notify(n);
	sprintf(notify_woof_name, "%s/%s", dht_tbl.successor_addr[0], DHT_NOTIFY_ARG_WOOF);
	memcpy(notify_arg.node_hash, id_hash, sizeof(notify_arg.node_hash));
	strncpy(notify_arg.node_addr, woof_name, sizeof(notify_arg.node_addr));
	sprintf(notify_arg.callback_woof, "%s/%s", woof_name, DHT_NOTIFY_RESULT_WOOF);
	sprintf(notify_arg.callback_handler, "notify_callback");
	seq = WooFPut(notify_woof_name, "notify", &notify_arg);
	if (WooFInvalid(seq))
	{
		sprintf(msg, "couldn't call notify on successor %s", notify_woof_name);
		log_error("stablize_callback", msg);
		exit(1);
	}

	sprintf(msg, "called notify on successor %s", dht_tbl.successor_addr[0]);
	log_debug("stablize_callback", msg);

	return 1;
}
