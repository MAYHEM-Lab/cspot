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
	int err;
	DHT_TABLE_EL dht_tbl;
	unsigned long seq;
	int i;
	char msg[256];

	sprintf(msg, "potential predecessor_addr: %s", arg->node_addr);
	log_debug("notify", msg);

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
	
	return 1;
}
