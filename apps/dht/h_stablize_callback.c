#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <openssl/sha.h>
#include "woofc.h"
#include "dht.h"
#include "dht_utils.h"

int h_stablize_callback(WOOF *wf, unsigned long seq_no, void *ptr) {
	DHT_STABLIZE_CALLBACK_ARG *result = (DHT_STABLIZE_CALLBACK_ARG *)ptr;

	log_set_tag("stablize_callback");
	// log_set_level(DHT_LOG_DEBUG);
	log_set_level(DHT_LOG_INFO);
	log_set_output(stdout);

	DHT_TABLE dht_table = {0};
	if (get_latest_element(DHT_TABLE_WOOF, &dht_table) < 0) {
		log_error("couldn't get latest dht_table: %s", dht_error_msg);
		exit(1);
	}

	log_debug("get_predecessor addr: %s", result->predecessor_replicas[result->predecessor_leader]);

	// x = successor.predecessor
	// if (x ∈ (n, successor))
	if (!is_empty(result->predecessor_hash) && 
		in_range(result->predecessor_hash, dht_table.node_hash, dht_table.successor_hash[0])) {
		// successor = x;
		if (memcmp(dht_table.successor_hash[0], result->predecessor_hash, SHA_DIGEST_LENGTH) != 0) {
			if (hmap_set_replicas(result->predecessor_hash, result->predecessor_replicas, result->predecessor_leader) < 0) {
				log_error("failed to set result predecessor's replicas: %s", dht_error_msg);
				exit(1);
			}
			memcpy(dht_table.successor_hash[0], result->predecessor_hash, sizeof(dht_table.successor_hash[0]));
			unsigned long seq = WooFPut(DHT_TABLE_WOOF, NULL, &dht_table);
			if (WooFInvalid(seq)) {
				log_error("failed to update DHT table to %s", DHT_TABLE_WOOF);
				exit(1);
			}
			log_info("updated successor to %s", result->predecessor_replicas[result->predecessor_leader]);
		}
	}

	DHT_HASHMAP_ENTRY successor = {0};
	if (hmap_get(dht_table.successor_hash[0], &successor) < 0) {
		log_error("failed to get successor's replicas: %s", dht_error_msg);
		exit(1);
	}
	char *successor_leader = successor.replicas[successor.leader];

	// successor.notify(n);
	char notify_woof_name[DHT_NAME_LENGTH];
	sprintf(notify_woof_name, "%s/%s", successor_leader, DHT_NOTIFY_WOOF);
	DHT_NOTIFY_ARG notify_arg = {0};
	memcpy(notify_arg.node_hash, dht_table.node_hash, sizeof(notify_arg.node_hash));
	memcpy(notify_arg.node_replicas, dht_table.replicas, sizeof(notify_arg.node_replicas));
	notify_arg.node_leader = dht_table.replica_id;
	sprintf(notify_arg.callback_woof, "%s/%s", dht_table.node_addr, DHT_NOTIFY_CALLBACK_WOOF);
	sprintf(notify_arg.callback_handler, "h_notify_callback");
	unsigned long seq = WooFPut(notify_woof_name, "h_notify", &notify_arg);
	if (WooFInvalid(seq)) {
		log_error("failed to call notify on successor %s", notify_woof_name);
		exit(1);
	}
	log_debug("called notify on successor %s", successor_leader);

	return 1;
}
