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

	DHT_TABLE dht_table = {0};
	if (get_latest_element(DHT_TABLE_WOOF, &dht_table) < 0) {
		fprintf(stderr, "couldn't get latest dht_table: %s", dht_error_msg);
		exit(1);
	}

	if (memcmp(dht_table.successor_hash[0], dht_table.node_hash, SHA_DIGEST_LENGTH) == 0) {
		log_debug("current successor is its self");

		// successor = predecessor;
		if (!is_empty(dht_table.predecessor_hash)
			&& (memcmp(dht_table.successor_hash[0], dht_table.predecessor_hash, SHA_DIGEST_LENGTH) != 0)) {
			// predecessor's replicas should be in the hashmap already
			memcpy(dht_table.successor_hash[0], dht_table.predecessor_hash, sizeof(dht_table.successor_hash[0]));
			unsigned long seq = WooFPut(DHT_TABLE_WOOF, NULL, &dht_table);
			if (WooFInvalid(seq)) {
				log_error("failed to update successor");
				exit(1);
			}
			log_info("updated successor to predecessor since there is only one node in the cluster");
		}

		// successor.notify(n);
		DHT_NOTIFY_ARG notify_arg = {0};
		memcpy(notify_arg.node_hash, dht_table.node_hash, sizeof(notify_arg.node_hash));
		memcpy(notify_arg.node_replicas, dht_table.replicas, sizeof(notify_arg.node_replicas));
		notify_arg.node_leader = dht_table.replica_id;
		unsigned long seq = WooFPut(DHT_NOTIFY_WOOF, "h_notify", &notify_arg);
		if (WooFInvalid(seq)) {
			log_error("failed to call notify on self %s", dht_table.node_addr);
			exit(1);
		}
		log_debug("calling notify on self");
	} else if (is_empty(dht_table.successor_hash[0])) {
		log_info("no successor, set it back to self");
		// node_hash should be in the hashmap already
		memcpy(dht_table.successor_hash[0], dht_table.node_hash, sizeof(dht_table.successor_hash[0]));
		unsigned long seq = WooFPut(DHT_TABLE_WOOF, NULL, &dht_table);
		if (WooFInvalid(seq)) {
			log_error("failed to set successor back to self");
			exit(1);
		}
		log_info("successor set to self");
	} else {
		DHT_HASHMAP_ENTRY successor = {0};
		if (hmap_get(dht_table.successor_hash[0], &successor) < 0) {
			log_error("failed to get the successor's replica addresses: %s", dht_error_msg);
			exit(1);
		}
		char *successor_addr = successor.replicas[successor.leader];
		log_debug("current successor_addr: %s", successor_addr);

		// x = successor.predecessor
		DHT_GET_PREDECESSOR_ARG arg = {0};
		sprintf(arg.callback_woof, "%s/%s", woof_name, DHT_STABLIZE_CALLBACK_WOOF);
		sprintf(arg.callback_handler, "h_stablize_callback");
		char successor_woof_name[DHT_NAME_LENGTH];
		while (successor_addr[0] != 0) {
			sprintf(successor_woof_name, "%s/%s", successor_addr, DHT_GET_PREDECESSOR_WOOF);
			unsigned long seq = WooFPut(successor_woof_name, "h_get_predecessor", &arg);
			if (WooFInvalid(seq)) {
				log_warn("failed to invoke h_get_predecessor", successor_woof_name);
// TODO: use the next replica first
				// failed to contact successor, use the next one
				shift_successor_list(dht_table.successor_hash);
				seq = WooFPut(DHT_TABLE_WOOF, NULL, &dht_table);
				if (WooFInvalid(seq)) {
					log_error("failed to shift successor");
					exit(1);
				}
				DHT_HASHMAP_ENTRY shifted_successor = {0};
				if (hmap_get(dht_table.successor_hash[0], &shifted_successor) < 0) {
					log_error("failed to get shifted successor's replica addresses: %s", dht_error_msg);
					exit(1);
				}
				char *shifted_successor_addr = shifted_successor.replicas[shifted_successor.leader];
				log_debug("successor shifted. new: %s", shifted_successor_addr);
				continue;
			}
			log_debug("asked to get_predecessor from %s", successor_woof_name);
			return 1;
		}
		log_error("fatal error: none of the successors is responding");
		exit(1);
	}
	
	return 1;
}
