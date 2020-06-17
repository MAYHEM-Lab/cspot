#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <openssl/sha.h>
#include "woofc.h"
#include "dht.h"
#include "dht_utils.h"

int h_stabilize_callback(WOOF *wf, unsigned long seq_no, void *ptr) {
	DHT_STABILIZE_CALLBACK_ARG *result = (DHT_STABILIZE_CALLBACK_ARG *)ptr;

	log_set_tag("stabilize_callback");
	// log_set_level(DHT_LOG_DEBUG);
	log_set_level(DHT_LOG_INFO);
	log_set_output(stdout);

	DHT_NODE_INFO node = {0};
	if (get_latest_node_info(&node) < 0) {
		log_error("couldn't get latest node info: %s", dht_error_msg);
		exit(1);
	}
	DHT_SUCCESSOR_INFO successor = {0};
	if (get_latest_successor_info(&successor) < 0) {
		log_error("couldn't get latest successor info: %s", dht_error_msg);
		exit(1);
	}

	log_debug("get_predecessor addr: %s", result->predecessor_replicas[result->predecessor_leader]);

	// x = successor.predecessor
	// if (x ∈ (n, successor))
	if (!is_empty(result->predecessor_hash) && in_range(result->predecessor_hash, node.hash, successor.hash[0])) {
		// successor = x;
		if (memcmp(successor.hash[0], result->predecessor_hash, SHA_DIGEST_LENGTH) != 0) {
			memcpy(successor.hash[0], result->predecessor_hash, sizeof(successor.hash[0]));
			memcpy(successor.replicas[0], result->predecessor_replicas, sizeof(successor.replicas[0]));
			successor.leader[0] = result->predecessor_leader;
			unsigned long seq = WooFPut(DHT_SUCCESSOR_INFO_WOOF, NULL, &successor);
			if (WooFInvalid(seq)) {
				log_error("failed to update DHT table to %s", DHT_SUCCESSOR_INFO_WOOF);
				exit(1);
			}
			log_info("updated successor to %s", result->predecessor_replicas[result->predecessor_leader]);
		}
	}

	char *successor_leader = successor_addr(&successor, 0);

	// successor.notify(n);
	char notify_woof_name[DHT_NAME_LENGTH];
	sprintf(notify_woof_name, "%s/%s", successor_leader, DHT_NOTIFY_WOOF);
	DHT_NOTIFY_ARG notify_arg = {0};
	memcpy(notify_arg.node_hash, node.hash, sizeof(notify_arg.node_hash));
	memcpy(notify_arg.node_replicas, node.replicas, sizeof(notify_arg.node_replicas));
	notify_arg.node_leader = node.replica_id;
	sprintf(notify_arg.callback_woof, "%s/%s", node.addr, DHT_NOTIFY_CALLBACK_WOOF);
	sprintf(notify_arg.callback_handler, "h_notify_callback");
	unsigned long seq = WooFPut(notify_woof_name, "h_notify", &notify_arg);
	if (WooFInvalid(seq)) {
		log_error("failed to call notify on successor %s", notify_woof_name);
		exit(1);
	}
	log_debug("called notify on successor %s", successor_leader);

	return 1;
}
