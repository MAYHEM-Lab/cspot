#include <stdio.h>
#include <string.h>
#include "woofc.h"
#include "dht.h"
#include "dht_utils.h"

int d_stabilize(WOOF *wf, unsigned long seq_no, void *ptr) {
	log_set_tag("stabilize");
	// log_set_level(DHT_LOG_DEBUG);
	log_set_level(DHT_LOG_INFO);
	log_set_output(stdout);

	char woof_name[DHT_NAME_LENGTH];
	if (node_woof_name(woof_name) < 0) {
		log_error("failed to get local node's woof name");
		return;
	}

	DHT_NODE_INFO node = {0};
	if (get_latest_node_info(&node) < 0) {
		log_error("couldn't get latest node info: %s", dht_error_msg);
		exit(1);
	}
	DHT_PREDECESSOR_INFO predecessor = {0};
	if (get_latest_predecessor_info(&predecessor) < 0) {
		log_error("couldn't get latest predecessor info: %s", dht_error_msg);
		exit(1);
	}
	DHT_SUCCESSOR_INFO successor = {0};
	if (get_latest_successor_info(&successor) < 0) {
		log_error("couldn't get latest successor info: %s", dht_error_msg);
		exit(1);
	}

	if (memcmp(successor.hash[0], node.hash, SHA_DIGEST_LENGTH) == 0) {
		// successor = predecessor;
		if (!is_empty(predecessor.hash) && (memcmp(successor.hash[0], predecessor.hash, SHA_DIGEST_LENGTH) != 0)) {
			// predecessor's replicas should be in the hashmap already
			memcpy(successor.hash[0], predecessor.hash, sizeof(successor.hash[0]));
			memcpy(successor.replicas[0], predecessor.replicas, sizeof(successor.replicas[0]));
			successor.leader[0] = predecessor.leader;
			unsigned long seq = WooFPut(DHT_SUCCESSOR_INFO_WOOF, NULL, &successor);
			if (WooFInvalid(seq)) {
				log_error("failed to update successor");
				exit(1);
			}
			log_info("updated successor to predecessor because the current successor is itself");
		}

		// successor.notify(n);
		DHT_NOTIFY_ARG notify_arg = {0};
		memcpy(notify_arg.node_hash, node.hash, sizeof(notify_arg.node_hash));
		memcpy(notify_arg.node_replicas, node.replicas, sizeof(notify_arg.node_replicas));
		notify_arg.node_leader = node.replica_id;
		unsigned long seq = WooFPut(DHT_NOTIFY_WOOF, "h_notify", &notify_arg);
		if (WooFInvalid(seq)) {
			log_error("failed to call notify on self %s", node.addr);
			exit(1);
		}
		log_debug("calling notify on self");
	} else if (is_empty(successor.hash[0])) {
		log_info("no current successor");
		// node_hash should be in the hashmap already
		memcpy(successor.hash[0], node.hash, sizeof(successor.hash[0]));
		memcpy(successor.replicas[0], node.replicas, sizeof(successor.replicas[0]));
		successor.leader[0] = node.replica_id;
		unsigned long seq = WooFPut(DHT_SUCCESSOR_INFO_WOOF, NULL, &successor);
		if (WooFInvalid(seq)) {
			log_error("failed to set successor back to self");
			exit(1);
		}
		log_info("successor set to self");
	} else {
		char *successor_addr = successor.replicas[0][successor.leader[0]];
		log_debug("current successor_addr: %s", successor_addr);

		// x = successor.predecessor
		DHT_GET_PREDECESSOR_ARG arg = {0};
		sprintf(arg.callback_woof, "%s/%s", woof_name, DHT_STABILIZE_CALLBACK_WOOF);
		sprintf(arg.callback_handler, "h_stabilize_callback");
		char successor_woof_name[DHT_NAME_LENGTH];
		while (successor_addr[0] != 0) {
			sprintf(successor_woof_name, "%s/%s", successor_addr, DHT_GET_PREDECESSOR_WOOF);
			unsigned long seq = WooFPut(successor_woof_name, "h_get_predecessor", &arg);
			if (WooFInvalid(seq)) {
				log_warn("failed to invoke h_get_predecessor", successor_woof_name);
// TODO: use the next replica first
				// failed to contact successor, use the next one
				shift_successor_list(&successor);
				seq = WooFPut(DHT_SUCCESSOR_INFO_WOOF, NULL, &successor);
				if (WooFInvalid(seq)) {
					log_error("failed to shift successor");
					exit(1);
				}
				log_debug("successor shifted. new: %s", successor.replicas[0][successor.leader[0]]);
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
