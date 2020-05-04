#define DEBUG

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <openssl/sha.h>

#include "woofc.h"
#include "dht.h"

int h_find_successor(WOOF *wf, unsigned long seq_no, void *ptr) {
	FIND_SUCESSOR_ARG *arg = (FIND_SUCESSOR_ARG *)ptr;

	log_set_tag("find_successor");
	// log_set_level(LOG_DEBUG);
	log_set_level(LOG_INFO);
	log_set_output(stdout);

// if (strcmp(arg->callback_handler, "h_fix_fingers_callback") == 0) {
// 	log_set_level(LOG_INFO);
// }
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
	char msg[256];
	sprintf(msg, "id_hash: ");
	print_node_hash(msg + strlen(msg), arg->id_hash);
	log_debug(msg);
	log_debug("request_seq_no: %lu", arg->request_seq_no);

	if (arg->request_seq_no == 0) {
		arg->request_seq_no = seq_no;
	}
	arg->hops++;

	// if id in (n, successor]
	if (in_range(arg->id_hash, dht_tbl.node_hash, dht_tbl.successor_hash[0])
		|| memcmp(arg->id_hash, dht_tbl.successor_hash[0], SHA_DIGEST_LENGTH) == 0) {
		// return successor
		if (arg->callback_woof[0] != 0) {
			FIND_SUCESSOR_RESULT result;
			memcpy(result.target_hash, arg->id_hash, sizeof(result.target_hash));
			result.request_seq_no = arg->request_seq_no;
			result.finger_index = arg->finger_index;
			result.hops = arg->hops;
			memcpy(result.node_addr, dht_tbl.successor_addr[0], sizeof(result.node_addr));
			memcpy(result.node_hash, dht_tbl.successor_hash[0], sizeof(result.node_hash));

			seq = WooFPut(arg->callback_woof, arg->callback_handler, &result);
			if (WooFInvalid(seq)) {
				log_error("couldn't put find_successor's result to woof %s", arg->callback_woof);
				exit(1);
			}
			sprintf(msg, "returned successor hash: ", result.node_hash);
			print_node_hash(msg + strlen(msg), result.node_hash);
			log_debug(msg);
			log_debug("returned successor addr: %s", result.node_addr);
		}
		return 1;
	}
	// else closest_preceding_node(id)
	char req_forward_woof[DHT_NAME_LENGTH];
	int i;
	for (i = SHA_DIGEST_LENGTH * 8; i > 0; --i) {
		if (dht_tbl.finger_addr[i][0] == 0) {
			continue;
		}
		// if finger[i] in (n, id)
		// return finger[i].find_succesor(id)
		if (in_range(dht_tbl.finger_hash[i], dht_tbl.node_hash, arg->id_hash)) {
			sprintf(req_forward_woof, "%s/%s", dht_tbl.finger_addr[i], DHT_FIND_SUCCESSOR_ARG_WOOF);
			seq = WooFPut(req_forward_woof, "h_find_successor", arg);
			if (WooFInvalid(seq)) {
				log_error("couldn't put woof to %s", req_forward_woof);
				exit(1);
			}
			log_debug("forwarded find_succesor request to finger[%d]: %s", i, dht_tbl.finger_addr[i]);
			return 1;
		}
	}

	// return n.find_succesor(id)
	log_debug("closest_preceding_node not found in finger table");

	sprintf(req_forward_woof, "%s/%s", dht_tbl.node_addr, DHT_FIND_SUCCESSOR_ARG_WOOF);
	seq = WooFPut(req_forward_woof, "h_find_successor", arg);
	if (WooFInvalid(seq)) {
		log_error("couldn't put woof to %s", req_forward_woof);
		exit(1);
	}
	log_debug("forwarded find_succesor request to self: %s", dht_tbl.node_addr);
	return 1;
}
