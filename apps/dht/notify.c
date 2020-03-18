#define DEBUG

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <openssl/sha.h>

#include "woofc.h"
#include "dht.h"

int notify(WOOF *wf, unsigned long seq_no, void *ptr)
{
	// log_set_level(LOG_DEBUG);
	log_set_level(LOG_INFO);
	log_set_output(stdout);

	NOTIFY_ARG *arg = (NOTIFY_ARG *)ptr;
	NOTIFY_RESULT result;
	int err;
	DHT_TABLE_EL dht_tbl;
	unsigned long seq;
	int i;
	char msg[256];

	sprintf(msg, "potential predecessor_addr: %s", arg->node_addr);
	log_debug("notify", msg);
sprintf(msg, "callback: %s/%s", arg->callback_woof, arg->callback_handler);
		log_debug("stablize", msg);
	seq = WooFGetLatestSeqno(DHT_TABLE_WOOF);
	if (WooFInvalid(seq))
	{
		log_error("notify", "couldn't get latest dht_table seq_no");
		exit(1);
	}
	err = WooFGet(DHT_TABLE_WOOF, &dht_tbl, seq);
	if (err < 0)
	{
		sprintf(msg, "couldn't get latest dht_table with seq_no %lu", seq);
		log_error("notify", msg);
		exit(1);
	}

	// if (predecessor is nil or n' ∈ (predecessor, n))
	if (dht_tbl.predecessor_addr[0] == 0
		|| memcmp(dht_tbl.predecessor_hash, dht_tbl.node_hash, SHA_DIGEST_LENGTH) == 0
		|| in_range(arg->node_hash, dht_tbl.predecessor_hash, dht_tbl.node_hash))
	{
		if (memcmp(dht_tbl.predecessor_hash, arg->node_hash, SHA_DIGEST_LENGTH) == 0)
		{
			sprintf(msg, "predecessor is the same, no need to update to %s", arg->node_addr);
			log_debug("notify", msg);
			return 1;
		}
		memcpy(dht_tbl.predecessor_hash, arg->node_hash, sizeof(dht_tbl.predecessor_hash));
		strncpy(dht_tbl.predecessor_addr, arg->node_addr, sizeof(dht_tbl.predecessor_addr));
		seq = WooFPut(DHT_TABLE_WOOF, NULL, &dht_tbl);
		if (WooFInvalid(seq))
		{
			log_error("notify", "couldn't update predecessor");
			exit(1);
		}
		sprintf(msg, "updated predecessor_hash: ");
		print_node_hash(msg + strlen(msg), dht_tbl.predecessor_hash);
		log_info("notify", msg);
		sprintf(msg, "updated predecessor_addr: %s", dht_tbl.predecessor_addr);
		log_info("notify", msg);
	}

	if (arg->callback_woof[0] == 0)
	{
		return 1;
	}

	// call notify_callback, where it updates successor list
	memcpy(result.successor_addr[0], dht_tbl.node_addr, sizeof(result.successor_addr[0]));
	memcpy(result.successor_hash[0], dht_tbl.node_hash, sizeof(result.successor_hash[0]));
	for (i = 0; i < DHT_SUCCESSOR_LIST_R - 1; i++)
	{
		memcpy(result.successor_addr[i + 1], dht_tbl.successor_addr[i], sizeof(result.successor_addr[i + 1]));
		memcpy(result.successor_hash[i + 1], dht_tbl.successor_hash[i], sizeof(result.successor_hash[i + 1]));
	}

	seq = WooFPut(arg->callback_woof, arg->callback_handler, &result);
	if (WooFInvalid(seq))
	{
		sprintf(msg, "couldn't put notify result to woof %s", arg->callback_woof);
		log_error("notify", msg);
		exit(1);
	}
	log_debug("notify", "returned successor list");
	
	return 1;
}
