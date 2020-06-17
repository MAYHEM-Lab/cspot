#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <openssl/sha.h>
#include "woofc.h"
#include "dht.h"
#include "dht_utils.h"
#ifdef USE_RAFT
#include "raft_client.h"
#endif

int h_find_successor(WOOF *wf, unsigned long seq_no, void *ptr) {
	DHT_FIND_SUCCESSOR_ARG *arg = (DHT_FIND_SUCCESSOR_ARG *)ptr;

	log_set_tag("find_successor");
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

	arg->hops++;
	char hash_str[2 * SHA_DIGEST_LENGTH + 1];
	print_node_hash(hash_str, arg->hashed_key);
	log_debug("callback_namespace: %s, hashed_key: %s, hops: %d", arg->callback_namespace, hash_str, arg->hops);

	// if id in (n, successor]
	if (in_range(arg->hashed_key, node.hash, successor.hash[0])
		|| memcmp(arg->hashed_key, successor.hash[0], SHA_DIGEST_LENGTH) == 0) {
		// return successor
		char *replicas_leader = successor_addr(&successor, 0);
		switch (arg->action) {
			case DHT_ACTION_FIND_NODE: {
				DHT_FIND_NODE_RESULT action_result = {0};
				memcpy(action_result.topic, arg->key, sizeof(action_result.topic));
				memcpy(action_result.node_hash, successor.hash[0], sizeof(action_result.node_hash));
				memcpy(action_result.node_replicas, successor.replicas[0], sizeof(action_result.node_replicas));
				action_result.node_leader = successor.leader[0];
				action_result.hops = arg->hops;

				char callback_woof[DHT_NAME_LENGTH];
				sprintf(callback_woof, "%s/%s", arg->callback_namespace, DHT_FIND_NODE_RESULT_WOOF);
				unsigned long seq = WooFPut(callback_woof, NULL, &action_result);
				if (WooFInvalid(seq)) {
					log_error("failed to put find_node result on %s", callback_woof);
					exit(1);
				}
				log_debug("found node_addr %s for action %s", replicas_leader, "FIND_NODE");
				break;
			}
			case DHT_ACTION_JOIN: {
				DHT_JOIN_ARG action_arg = {0};
				memcpy(action_arg.node_hash, successor.hash[0], sizeof(action_arg.node_hash));
				memcpy(action_arg.node_replicas, successor.replicas[0], sizeof(action_arg.node_replicas));
				action_arg.node_leader = successor.leader[0];

				char callback_woof[DHT_NAME_LENGTH];
				sprintf(callback_woof, "%s/%s", arg->callback_namespace, DHT_JOIN_WOOF);
				unsigned long seq = WooFPut(callback_woof, "h_join_callback", &action_arg);
				if (WooFInvalid(seq)) {
					log_error("failed to invoke h_join_callback on %s", callback_woof);
					exit(1);
				}
				log_debug("found successor_addr %s for action %s", replicas_leader, "JOIN");
				break;
			}
			case DHT_ACTION_FIX_FINGER: {
				DHT_FIX_FINGER_CALLBACK_ARG action_arg = {0};
				memcpy(action_arg.node_hash, successor.hash[0], sizeof(action_arg.node_hash));
				memcpy(action_arg.node_replicas, successor.replicas[0], sizeof(action_arg.node_replicas));
				action_arg.node_leader = successor.leader[0];
				action_arg.finger_index = (int)arg->action_seqno;
				
				char callback_woof[DHT_NAME_LENGTH];
				sprintf(callback_woof, "%s/%s", arg->callback_namespace, DHT_FIX_FINGER_CALLBACK_WOOF);
				unsigned long seq = WooFPut(callback_woof, "h_fix_finger_callback", &action_arg);
				if (WooFInvalid(seq)) {
					log_error("failed to invoke h_fix_finger_callback on %s", callback_woof);
					exit(1);
				}
				log_debug("found successor_addr %s for action %s", replicas_leader, "FIX_FINGER");
				break;
			}
			case DHT_ACTION_REGISTER_TOPIC: {
				char action_arg_woof[DHT_NAME_LENGTH];
				sprintf(action_arg_woof, "%s/%s", arg->action_namespace, DHT_REGISTER_TOPIC_WOOF);
				DHT_REGISTER_TOPIC_ARG action_arg = {0};
				if (WooFGet(action_arg_woof, &action_arg, arg->action_seqno) < 0) {
					log_error("failed to get register_topic_arg from %s", action_arg_woof);
					exit(1);
				}

#ifdef USE_RAFT
				raft_set_client_leader(replicas_leader);
				unsigned long index = raft_put_handler("h_register_topic", &action_arg, sizeof(DHT_REGISTER_TOPIC_ARG), 0);
				while (index == RAFT_REDIRECTED) {
					log_debug("redirected to %s", raft_client_leader);
					index = raft_put_handler("h_register_topic", &action_arg, sizeof(DHT_REGISTER_TOPIC_ARG), 0);
				}
				if (raft_is_error(index)) {
					log_error("failed to invoke h_register_topic using raft: %s", raft_error_msg);
					exit(1);
				}
#else
				char callback_woof[DHT_NAME_LENGTH];
				sprintf(callback_woof, "%s/%s", replicas_leader, DHT_REGISTER_TOPIC_WOOF);
				unsigned long seq = WooFPut(callback_woof, "h_register_topic", &action_arg);
				if (WooFInvalid(seq)) {
					log_error("failed to invoke h_register_topic on %s", callback_woof);
					exit(1);
				}
#endif
				log_debug("found successor_addr %s for action %s", replicas_leader, "REGISTER_TOPIC");
				break;
			}
			case DHT_ACTION_SUBSCRIBE: {
				char action_arg_woof[DHT_NAME_LENGTH];
				sprintf(action_arg_woof, "%s/%s", arg->action_namespace, DHT_SUBSCRIBE_WOOF);
				DHT_SUBSCRIBE_ARG action_arg = {0};
				if (WooFGet(action_arg_woof, &action_arg, arg->action_seqno) < 0) {
					log_error("failed to get subscribe_arg from %s", action_arg_woof);
					exit(1);
				}
#ifdef USE_RAFT
				raft_set_client_leader(replicas_leader);
				unsigned long index = raft_put_handler("h_subscribe", &action_arg, sizeof(DHT_SUBSCRIBE_ARG), 0);
				while (index == RAFT_REDIRECTED) {
					log_debug("redirected to %s", raft_client_leader);
					index = raft_put_handler("h_subscribe", &action_arg, sizeof(DHT_SUBSCRIBE_ARG), 0);
				}
				if (raft_is_error(index)) {
					log_error("failed to invoke h_subscribe using raft: %s", raft_error_msg);
					exit(1);
				}
#else
				char callback_woof[DHT_NAME_LENGTH];
				sprintf(callback_woof, "%s/%s", replicas_leader, DHT_SUBSCRIBE_WOOF);
				unsigned long seq = WooFPut(callback_woof, "h_subscribe", &action_arg);
				if (WooFInvalid(seq)) {
					log_error("failed to invoke h_subscribe on %s", callback_woof);
					exit(1);
				}
#endif
				log_debug("found successor_addr %s for action %s", replicas_leader, "SUBSCRIBE");
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
		DHT_FINGER_INFO finger = {0};
		if (get_latest_finger_info(i, &finger) < 0) {
			log_error("failed to get finger[%d]'s info: %s", i, dht_error_msg);
			exit(1);
		}
		if (is_empty(finger.hash)) {
			continue;
		}
		// if finger[i] in (n, id)
		// return finger[i].find_succesor(id)
		if (in_range(finger.hash, node.hash, arg->hashed_key)) {
			char *finger_replicas_leader = finger_addr(&finger);
			sprintf(req_forward_woof, "%s/%s", finger_replicas_leader, DHT_FIND_SUCCESSOR_WOOF);
			unsigned long seq = WooFPut(req_forward_woof, "h_find_successor", arg);
			if (WooFInvalid(seq)) {
				log_error("failed to forward find_successor request to %s", req_forward_woof);
				exit(1);
			}
			log_debug("forwarded find_succesor request to finger[%d]: %s", i, finger_replicas_leader);
			return 1;
		}
	}
	// TODO: also check the other successors
	if (in_range(successor.hash[0], node.hash, arg->hashed_key)) {
		sprintf(req_forward_woof, "%s/%s", successor_addr(&successor, 0), DHT_FIND_SUCCESSOR_WOOF);
		unsigned long seq = WooFPut(req_forward_woof, "h_find_successor", arg);
		if (WooFInvalid(seq)) {
			log_error("failed to forward find_successor request to %s", req_forward_woof);
			exit(1);
		}
		log_debug("forwarded find_succesor request to successor: %s", successor_addr(&successor, 0));
		return 1;
	}

	// return n.find_succesor(id)
	log_debug("closest_preceding_node not found in finger table");

	sprintf(req_forward_woof, "%s/%s", node.addr, DHT_FIND_SUCCESSOR_WOOF);
	unsigned long seq = WooFPut(req_forward_woof, "h_find_successor", arg);
	if (WooFInvalid(seq)) {
		log_error("failed to put woof to %s", req_forward_woof);
		exit(1);
	}
	log_debug("forwarded find_succesor request to self: %s", node.addr);
	return 1;
}
