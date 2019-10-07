#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <openssl/sha.h>

#include "woofc.h"
#include "woofc-host.h"
#include "dht.h"

int main(int argc, char **argv)
{
	int err;
	unsigned long seq_no;
	DHT_TABLE_EL dht_tbl;
	char woof_name[2048];
	char successor_woof_name[2048];
	GET_PREDECESSOR_ARG arg;
	NOTIFY_ARG notify_arg;
	char msg[128];

	// log_set_level(LOG_DEBUG);
	log_set_level(LOG_INFO);
	// FILE *f = fopen("log_stablize","w");
	// log_set_output(f);
	log_set_output(stdout);
	WooFInit();

	err = node_woof_name(woof_name);
	if (err < 0)
	{
		log_error("stablize", "couldn't get local node's woof name");
		return 0;
	}

	seq_no = WooFGetLatestSeqno(DHT_TABLE_WOOF);
	if (WooFInvalid(seq_no))
	{
		log_error("stablize", "couldn't get latest dht_table seq_no");
		return 0;
	}
	err = WooFGet(DHT_TABLE_WOOF, &dht_tbl, seq_no);
	if (err < 0)
	{
		sprintf(msg, "couldn't get latest dht_table with seq_no %lu", seq_no);
		log_error("stablize", msg);
		return 0;
	}

	if (memcmp(dht_tbl.successor_hash, dht_tbl.node_hash, SHA_DIGEST_LENGTH) == 0)
	{
		sprintf(msg, "current successor is its self");
		log_debug("stablize", msg);

		// successor = predecessor;
		sprintf(msg, "successor_hash:   ");
		print_node_hash(msg + strlen(msg), dht_tbl.successor_hash);
		log_info("stablize", msg);
		sprintf(msg, "predecessor_hash: ");
		print_node_hash(msg + strlen(msg), dht_tbl.predecessor_hash);
		log_info("stablize", msg);
		if ((dht_tbl.predecessor_hash[0] != 0) && (memcmp(dht_tbl.successor_hash, dht_tbl.predecessor_hash, SHA_DIGEST_LENGTH) != 0))
		{
			memcpy(dht_tbl.successor_hash, dht_tbl.predecessor_hash, SHA_DIGEST_LENGTH);
			sprintf(msg, "dht_tbl.successor_hash: ");
			print_node_hash(msg + strlen(msg), dht_tbl.successor_hash);
			log_info("stablize", msg);
			sprintf(msg, "dht_tbl.predecessor_hash: ");
			print_node_hash(msg + strlen(msg), dht_tbl.predecessor_hash);
			log_info("stablize", msg);

			strncpy(dht_tbl.successor_addr, dht_tbl.predecessor_addr, sizeof(dht_tbl.successor_addr));
			seq_no = WooFPut(DHT_TABLE_WOOF, NULL, &dht_tbl);
			if (WooFInvalid(seq_no))
			{
				log_error("stablize", "couldn't update successor");
				return 0;
			}

			seq_no = WooFGetLatestSeqno(DHT_TABLE_WOOF);
			if (WooFInvalid(seq_no))
			{
				log_error("stablize", "couldn't get latest dht_table seq_no");
				return 0;
			}
			err = WooFGet(DHT_TABLE_WOOF, &dht_tbl, seq_no);
			if (err < 0)
			{
				sprintf(msg, "couldn't get latest dht_table with seq_no %lu", seq_no);
				log_error("stablize", msg);
				return 0;
			}
			sprintf(msg, "dht_tbl.successor_hash: ");
			print_node_hash(msg + strlen(msg), dht_tbl.successor_hash);
			log_info("stablize", msg);
			sprintf(msg, "dht_tbl.predecessor_hash: ");
			print_node_hash(msg + strlen(msg), dht_tbl.predecessor_hash);
			log_info("stablize", msg);
			// sprintf(msg, "successor_hash:   ");
			// print_node_hash(msg + strlen(msg), dht_tbl.successor_hash);
			// log_info("stablize", msg);
			// sprintf(msg, "predecessor_hash: ");
			// print_node_hash(msg + strlen(msg), dht_tbl.predecessor_hash);
			// log_info("stablize", msg);
			// sprintf(msg, "memcmp(dht_tbl.successor_hash, dht_tbl.predecessor_hash, SHA_DIGEST_LENGTH): %d", memcmp(dht_tbl.successor_hash, dht_tbl.predecessor_hash, SHA_DIGEST_LENGTH));
			// log_info("stablize", msg);
			sprintf(msg, "updated sucessor to %s", dht_tbl.successor_addr);
			log_info("stablize", msg);
		}

		// successor.notify(n);
		memcpy(notify_arg.node_hash, dht_tbl.node_hash, sizeof(notify_arg.node_hash));
		strncpy(notify_arg.node_addr, dht_tbl.node_addr, sizeof(notify_arg.node_addr));

		seq_no = WooFPut(DHT_NOTIFY_ARG_WOOF, "notify", &notify_arg);
		if (WooFInvalid(seq_no))
		{
			sprintf(msg, "couldn't call notify on self %s", dht_tbl.node_addr);
			log_error("stablize", msg);
			return 0;
		}

		sprintf(msg, "called notify on self %s", dht_tbl.successor_addr);
		log_debug("stablize", msg);
	}
	else
	{
		sprintf(msg, "current successor_addr: %s", dht_tbl.successor_addr);
		log_debug("stablize", msg);

		sprintf(successor_woof_name, "%s/%s", dht_tbl.successor_addr, DHT_GET_PREDECESSOR_ARG_WOOF);
		sprintf(arg.callback_woof, "%s/%s", woof_name, DHT_FIND_SUCESSOR_RESULT_WOOF);
		sprintf(arg.callback_handler, "stablize_callback", sizeof(arg.callback_handler));

		// x = successor.predecessor
		seq_no = WooFPut(successor_woof_name, "get_predecessor", &arg);
		if (WooFInvalid(seq_no))
		{
			sprintf(msg, "couldn't get put to woof %s to get_predecessor", successor_woof_name);
			log_error("stablize", msg);
			return 0;
		}
		sprintf(msg, "asked to get_predecessor from %s", successor_woof_name);
		log_debug("stablize", msg);
	}

	return 0;
}
