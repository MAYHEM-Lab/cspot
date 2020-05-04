#define DEBUG

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <openssl/sha.h>

#include "woofc.h"
#include "dht.h"

int h_get_predecessor(WOOF *wf, unsigned long seq_no, void *ptr) {
	GET_PREDECESSOR_ARG *arg = (GET_PREDECESSOR_ARG *)ptr;

	log_set_tag("get_predecessor");
	// log_set_level(LOG_DEBUG);
	log_set_level(LOG_INFO);
	log_set_output(stdout);

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
	log_debug("callback_woof: %s", arg->callback_woof);
	log_debug("callback_handler: %s", arg->callback_handler);

	GET_PREDECESSOR_RESULT result;
	memcpy(result.predecessor_hash, dht_tbl.predecessor_hash, sizeof(result.predecessor_hash));
	memcpy(result.predecessor_addr, dht_tbl.predecessor_addr, sizeof(result.predecessor_addr));
	seq = WooFPut(arg->callback_woof, arg->callback_handler, &result);
	if (WooFInvalid(seq)){
		log_error("couldn't put get_predecessor: result to woof %s", arg->callback_woof);
		exit(1);
	}
	char msg[256];
	sprintf(msg, "returned predecessor_hash: ");
	print_node_hash(msg + strlen(msg), dht_tbl.predecessor_hash);
	log_debug(msg);
	log_debug("returned predecessor_addr: %s", result.predecessor_addr);

	return 1;
}
