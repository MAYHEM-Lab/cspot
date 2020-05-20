#include <stdio.h>
#include <string.h>
#include "woofc.h"
#include "dht.h"
#include "dht_utils.h"

int d_stablize(WOOF *wf, unsigned long seq_no, void *ptr) {
	log_set_tag("stablize");
	// log_set_level(DHT_LOG_DEBUG);
	log_set_level(DHT_LOG_INFO);
	log_set_output(stdout);

	char woof_name[DHT_NAME_LENGTH];
	if (node_woof_name(woof_name) < 0) {
		log_error("failed to get local node's woof name");
		return;
	}

	unsigned long seq = WooFGetLatestSeqno(DHT_TABLE_WOOF);
	if (WooFInvalid(seq)) {
		log_error("failed to get latest dht_table seq_no");
		exit(1);
	}
	DHT_TABLE dht_table = {0};
	if (WooFGet(DHT_TABLE_WOOF, &dht_table, seq) < 0) {
		log_error("failed to get latest dht_table with seq_no %lu", seq);
		exit(1);
	}

	if (memcmp(dht_table.successor_hash[0], dht_table.node_hash, SHA_DIGEST_LENGTH) == 0) {
		log_debug("current successor is its self");

		// successor = predecessor;
		if ((dht_table.predecessor_hash[0] != 0)
			&& (memcmp(dht_table.successor_hash[0], dht_table.predecessor_hash, SHA_DIGEST_LENGTH) != 0)) {
			memcpy(dht_table.successor_hash[0], dht_table.predecessor_hash, sizeof(dht_table.successor_hash[0]));
			memcpy(dht_table.successor_addr[0], dht_table.predecessor_addr, sizeof(dht_table.successor_addr[0]));

			seq = WooFPut(DHT_TABLE_WOOF, NULL, &dht_table);
			if (WooFInvalid(seq)) {
				log_error("failed to update successor");
				exit(1);
			}
			log_info("updated successor to %s", dht_table.successor_addr[0]);
		}

		// successor.notify(n);
		DHT_NOTIFY_ARG notify_arg = {0};
		memcpy(notify_arg.node_hash, dht_table.node_hash, sizeof(notify_arg.node_hash));
		memcpy(notify_arg.node_addr, dht_table.node_addr, sizeof(notify_arg.node_addr));
		seq = WooFPut(DHT_NOTIFY_WOOF, "h_notify", &notify_arg);
		if (WooFInvalid(seq)) {
			log_error("failed to call notify on self %s", dht_table.node_addr);
			exit(1);
		}
		log_debug("calling notify on self %s", dht_table.successor_addr[0]);
	} else if (dht_table.successor_addr[0][0] == 0) {
		log_info("no successor, set it back to self");
		memcpy(dht_table.successor_hash[0], dht_table.node_hash, sizeof(dht_table.successor_hash[0]));
		memcpy(dht_table.successor_addr[0], dht_table.node_addr, sizeof(dht_table.successor_addr[0]));
		seq = WooFPut(DHT_TABLE_WOOF, NULL, &dht_table);
		if (WooFInvalid(seq)) {
			log_error("failed to set successor back to self");
			exit(1);
		}
		log_info("successor set to self: %s", dht_table.successor_addr[0]);
	} else {
		log_debug("current successor_addr: %s", dht_table.successor_addr[0]);

		// x = successor.predecessor
		DHT_GET_PREDECESSOR_ARG arg = {0};
		sprintf(arg.callback_woof, "%s/%s", woof_name, DHT_STABLIZE_CALLBACK_WOOF);
		sprintf(arg.callback_handler, "h_stablize_callback");
		char successor_woof_name[DHT_NAME_LENGTH];
		while (dht_table.successor_addr[0][0] != 0) {
			sprintf(successor_woof_name, "%s/%s", dht_table.successor_addr[0], DHT_GET_PREDECESSOR_WOOF);
			seq = WooFPut(successor_woof_name, "h_get_predecessor", &arg);
			if (WooFInvalid(seq)) {
				log_warn("failed to invoke h_get_predecessor", successor_woof_name);

				// failed to contact successor, use the next one
				shift_successor_list(dht_table.successor_addr, dht_table.successor_hash);
				seq = WooFPut(DHT_TABLE_WOOF, NULL, &dht_table);
				if (WooFInvalid(seq)) {
					log_error("failed to shift successor");
					exit(1);
				}
				log_debug("successor shifted. new: %s", dht_table.successor_addr[0]);
				continue;
			}
			log_debug("asked to get_predecessor from %s", successor_woof_name);
			return 1;
		}
	}
	
	return 1;
}
