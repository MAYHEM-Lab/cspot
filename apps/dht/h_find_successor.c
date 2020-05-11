#define DEBUG

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <openssl/sha.h>

#include "woofc.h"
#include "dht.h"

int h_find_successor(WOOF *wf, unsigned long seq_no, void *ptr) {
	DHT_FIND_SUCCESSOR_ARG *arg = (DHT_FIND_SUCCESSOR_ARG *)ptr;

	log_set_tag("find_successor");
	// log_set_level(DHT_LOG_DEBUG);
	log_set_level(DHT_LOG_INFO);
	log_set_output(stdout);

	unsigned long seq = WooFGetLatestSeqno(DHT_TABLE_WOOF);
	if (WooFInvalid(seq)) {
		log_error("failed to get latest seq_no from %s", DHT_TABLE_WOOF);
		exit(1);
	}
	DHT_TABLE dht_table;
	if (WooFGet(DHT_TABLE_WOOF, &dht_table, seq) < 0) {
		log_error("failed to get the latest dht_table");
		exit(1);
	}
	arg->hops++;
	char hash_str[2 * SHA_DIGEST_LENGTH + 1];
	print_node_hash(hash_str, arg->hashed_key);
	log_debug("callback_namespace: %s, hashed_key: %s, hops: %d", arg->callback_namespace, hash_str, arg->hops);

	// if id in (n, successor]
	if (in_range(arg->hashed_key, dht_table.node_hash, dht_table.successor_hash[0])
		|| memcmp(arg->hashed_key, dht_table.successor_hash[0], SHA_DIGEST_LENGTH) == 0) {
		// return successor
		switch (arg->action) {
			case DHT_ACTION_FIND_ADDRESS: {
				DHT_FIND_ADDRESS_RESULT action_result;
				memcpy(action_result.topic, arg->key, sizeof(action_result.topic));
				action_result.hops = arg->hops;

				char registration_woof[DHT_NAME_LENGTH];
				sprintf(registration_woof, "%s/%s_%s", dht_table.successor_addr[0], action_result.topic, DHT_TOPIC_REGISTRATION_WOOF);
				unsigned long latest_topic_entry = WooFGetLatestSeqno(registration_woof);
				DHT_TOPIC_ENTRY topic_entry;
				if (WooFGet(registration_woof, &topic_entry, latest_topic_entry) < 0) {
					log_error("failed to get topic registration info");
					memset(action_result.topic_addr, 0, sizeof(action_result.topic_addr));
				}
				memcpy(action_result.topic_addr, topic_entry.topic_namespace, sizeof(action_result.topic_addr));

				char callback_woof[DHT_NAME_LENGTH];
				sprintf(callback_woof, "%s/%s", arg->callback_namespace, DHT_FIND_ADDRESS_RESULT_WOOF);
				seq = WooFPut(callback_woof, NULL, &action_result);
				if (WooFInvalid(seq)) {
					log_error("failed to put find_address result on %s", callback_woof);
					exit(1);
				}
				break;
			}
			case DHT_ACTION_FIND_NODE: {
				DHT_FIND_NODE_RESULT action_result;
				memcpy(action_result.topic, arg->key, sizeof(action_result.topic));
				memcpy(action_result.node_addr, dht_table.successor_addr[0], sizeof(action_result.node_addr));
				memcpy(action_result.node_hash, dht_table.successor_hash[0], sizeof(action_result.node_hash));
				action_result.hops = arg->hops;

				char callback_woof[DHT_NAME_LENGTH];
				sprintf(callback_woof, "%s/%s", arg->callback_namespace, DHT_FIND_NODE_RESULT_WOOF);
				seq = WooFPut(callback_woof, NULL, &action_result);
				if (WooFInvalid(seq)) {
					log_error("failed to put find_node result on %s", callback_woof);
					exit(1);
				}
				log_debug("found node_addr %s for action %s", action_result.node_addr, "FIND_NODE");
				break;
			}
			case DHT_ACTION_JOIN: {
				DHT_JOIN_ARG action_arg;
				memcpy(action_arg.node_addr, dht_table.successor_addr[0], sizeof(action_arg.node_addr));
				memcpy(action_arg.node_hash, dht_table.successor_hash[0], sizeof(action_arg.node_hash));

				char callback_woof[DHT_NAME_LENGTH];
				sprintf(callback_woof, "%s/%s", arg->callback_namespace, DHT_JOIN_WOOF);
				seq = WooFPut(callback_woof, "h_join", &action_arg);
				if (WooFInvalid(seq)) {
					log_error("failed to invoke h_join on %s", callback_woof);
					exit(1);
				}
				log_debug("found successor_addr %s for action %s", action_arg.node_addr, "JOIN");
				break;
			}
			case DHT_ACTION_FIX_FINGER: {
				DHT_FIX_FINGER_ARG action_arg;
				memcpy(action_arg.node_addr, dht_table.successor_addr[0], sizeof(action_arg.node_addr));
				memcpy(action_arg.node_hash, dht_table.successor_hash[0], sizeof(action_arg.node_hash));
				action_arg.finger_index = (int)arg->action_seqno;
				
				char callback_woof[DHT_NAME_LENGTH];
				sprintf(callback_woof, "%s/%s", arg->callback_namespace, DHT_FIX_FINGER_WOOF);
				seq = WooFPut(callback_woof, "h_fix_finger", &action_arg);
				if (WooFInvalid(seq)) {
					log_error("failed to invoke h_fix_finger on %s", callback_woof);
					exit(1);
				}
				log_debug("found successor_addr %s for action %s", action_arg.node_addr, "FIX_FINGER");
				break;
			}
			case DHT_ACTION_REGISTER_TOPIC: {
				char action_arg_woof[DHT_NAME_LENGTH];
				sprintf(action_arg_woof, "%s/%s", arg->action_namespace, DHT_REGISTER_TOPIC_WOOF);
				DHT_REGISTER_TOPIC_ARG action_arg;
				if (WooFGet(action_arg_woof, &action_arg, arg->action_seqno) < 0) {
					log_error("failed to get register_topic_arg from %s", action_arg_woof);
					exit(1);
				}
				
				char callback_woof[DHT_NAME_LENGTH];
				sprintf(callback_woof, "%s/%s", dht_table.successor_addr[0], DHT_REGISTER_TOPIC_WOOF);
				seq = WooFPut(callback_woof, "h_register_topic", &action_arg);
				if (WooFInvalid(seq)) {
					log_error("failed to invoke h_register_topic on %s", callback_woof);
					exit(1);
				}
				log_debug("found successor_addr %s for action %s", dht_table.successor_addr[0], "REGISTER_TOPIC");
				break;
			}
			case DHT_ACTION_SUBSCRIBE: {
				char action_arg_woof[DHT_NAME_LENGTH];
				sprintf(action_arg_woof, "%s/%s", arg->action_namespace, DHT_SUBSCRIBE_WOOF);
				DHT_SUBSCRIBE_ARG action_arg;
				if (WooFGet(action_arg_woof, &action_arg, arg->action_seqno) < 0) {
					log_error("failed to get subscribe_arg from %s", action_arg_woof);
					exit(1);
				}
				
				char callback_woof[DHT_NAME_LENGTH];
				sprintf(callback_woof, "%s/%s", dht_table.successor_addr[0], DHT_SUBSCRIBE_WOOF);
				seq = WooFPut(callback_woof, "h_subscribe", &action_arg);
				if (WooFInvalid(seq)) {
					log_error("failed to invoke h_subscribe on %s", callback_woof);
					exit(1);
				}
				log_debug("found successor_addr %s for action %s", dht_table.successor_addr[0], "SUBSCRIBE");
				break;
			}
			case DHT_ACTION_TRIGGER: {
				DHT_TRIGGER_ARG action_arg;
				sprintf(action_arg.woof_name, "%s/%s", arg->action_namespace, arg->key);
				action_arg.seqno = arg->action_seqno;

				char callback_woof[DHT_NAME_LENGTH];
				sprintf(callback_woof, "%s/%s", dht_table.successor_addr[0], DHT_TRIGGER_WOOF);
				seq = WooFPut(callback_woof, "h_trigger", &action_arg);
				if (WooFInvalid(seq)) {
					log_error("failed to invoke h_trigger on %s", callback_woof);
					exit(1);
				}
				log_debug("found successor_addr %s for action %s", dht_table.successor_addr[0], "TRIGGER");
				break;
			}
			default: {
				break;
			}
		}
		return 1;
	}
	// else closest_preceding_node(id)
	char req_forward_woof[DHT_NAME_LENGTH];
	int i;
	for (i = SHA_DIGEST_LENGTH * 8; i > 0; --i) {
		if (dht_table.finger_addr[i][0] == 0) {
			continue;
		}
		// if finger[i] in (n, id)
		// return finger[i].find_succesor(id)
		if (in_range(dht_table.finger_hash[i], dht_table.node_hash, arg->hashed_key)) {
			sprintf(req_forward_woof, "%s/%s", dht_table.finger_addr[i], DHT_FIND_SUCCESSOR_WOOF);
			seq = WooFPut(req_forward_woof, "h_find_successor", arg);
			if (WooFInvalid(seq)) {
				log_error("failed to forward find_successor request to %s", req_forward_woof);
				exit(1);
			}
			log_debug("forwarded find_succesor request to finger[%d]: %s", i, dht_table.finger_addr[i]);
			return 1;
		}
	}

	// return n.find_succesor(id)
	log_debug("closest_preceding_node not found in finger table");

	sprintf(req_forward_woof, "%s/%s", dht_table.node_addr, DHT_FIND_SUCCESSOR_WOOF);
	seq = WooFPut(req_forward_woof, "h_find_successor", arg);
	if (WooFInvalid(seq)) {
		log_error("failed to put woof to %s", req_forward_woof);
		exit(1);
	}
	log_debug("forwarded find_succesor request to self: %s", dht_table.node_addr);
	return 1;
}
