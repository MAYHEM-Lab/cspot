#define DEBUG

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <openssl/sha.h>

#include "woofc.h"
#include "dht.h"

int get_predecessor(WOOF *wf, unsigned long seq_no, void *ptr)
{
	// log_set_level(LOG_DEBUG);
	log_set_level(LOG_INFO);
	log_set_output(stdout);

	GET_PREDECESSOR_ARG *arg = (GET_PREDECESSOR_ARG *)ptr;
	GET_PREDECESSOR_RESULT result;
	int err;
	DHT_TABLE_EL dht_tbl;
	unsigned long seq;
	int i;
	char msg[256];

	seq = WooFGetLatestSeqno(DHT_TABLE_WOOF);
	if (WooFInvalid(seq))
	{
		log_error("get_predecessor", "couldn't get latest dht_table seq_no");
		exit(1);
	}
	err = WooFGet(DHT_TABLE_WOOF, &dht_tbl, seq);
	if (err < 0)
	{
		sprintf(msg, "couldn't get latest dht_table with seq_no %lu", seq);
		log_error("get_predecessor", msg);
		exit(1);
	}
	sprintf(msg, "callback_woof: %s", arg->callback_woof);
	log_debug("get_predecessor", msg);
	sprintf(msg, "callback_handler: %s", arg->callback_handler);
	log_debug("get_predecessor", msg);

	memcpy(result.predecessor_hash, dht_tbl.predecessor_hash, sizeof(result.predecessor_hash));
	strncpy(result.predecessor_addr, dht_tbl.predecessor_addr, sizeof(result.predecessor_addr));
	seq = WooFPut(arg->callback_woof, arg->callback_handler, &result);
	if (WooFInvalid(seq))
	{
		sprintf(msg, "couldn't put get_predecessor: result to woof %s", arg->callback_woof);
		log_error("get_predecessor", msg);
		exit(1);
	}
	sprintf(msg, "returned predecessor_hash: ");
	print_node_hash(msg + strlen(msg), dht_tbl.predecessor_hash);
	log_debug("get_predecessor", msg);
	sprintf(msg, "returned predecessor_addr: %s", result.predecessor_addr);
	log_debug("get_predecessor", msg);

	return 1;
}
