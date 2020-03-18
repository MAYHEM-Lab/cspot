// #define DEBUG

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <openssl/sha.h>

#include "woofc.h"
#include "dht.h"

int join_callback(WOOF *wf, unsigned long seq_no, void *ptr)
{
	log_set_level(LOG_DEBUG);
	// log_set_level(LOG_INFO);
	log_set_output(stdout);

	FIND_SUCESSOR_RESULT *result = (FIND_SUCESSOR_RESULT *)ptr;
	int err;
	int i;
	DHT_TABLE_EL dht_tbl;
	char woof_name[2048];
	unsigned char id_hash[SHA_DIGEST_LENGTH];
	char msg[256];

	err = node_woof_name(woof_name);
	if (err < 0)
	{
		log_error("join_callback", "couldn't get local node's woof name");
		exit(1);
	}

	// compute the node hash with SHA1
	SHA1(woof_name, strlen(woof_name), id_hash);
	
	sprintf(msg, "woof_name: %s", woof_name);
	log_debug("join_callback", msg);
	sprintf(msg, "node_hash: ");
	print_node_hash(msg + strlen(msg), id_hash);
	log_debug("join_callback", msg);
	sprintf(msg, "find_successor result hash: ");
	print_node_hash(msg + strlen(msg), result->node_hash);
	log_debug("join_callback", msg);
	sprintf(msg, "find_successor result addr: %s", result->node_addr);
	log_debug("join_callback", msg);

	dht_init(id_hash, woof_name, &dht_tbl);
	strncpy(dht_tbl.successor_hash[0], result->node_hash, sizeof(dht_tbl.successor_hash[0]));
	strncpy(dht_tbl.successor_addr[0], result->node_addr, sizeof(dht_tbl.successor_addr[0]));
	seq_no = WooFPut(DHT_TABLE_WOOF, NULL, &dht_tbl);
	if (WooFInvalid(seq_no))
	{
		sprintf(msg, "couldn't update DHT table to woof %s", DHT_TABLE_WOOF);
		log_error("join_callback", msg);
		exit(1);
	}

	sprintf(msg, "updated predecessor: %s", dht_tbl.predecessor_addr);
	log_info("join_callback", msg);
	sprintf(msg, "updated successor: %s", dht_tbl.successor_addr[0]);
	log_info("join_callback", msg);

	return 1;
}
