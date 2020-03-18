#define DEBUG

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <openssl/sha.h>

#include "woofc.h"
#include "dht.h"

int find_successor(WOOF *wf, unsigned long seq_no, void *ptr)
{
	// log_set_level(LOG_DEBUG);
	log_set_level(LOG_INFO);
	log_set_output(stdout);
	
	FIND_SUCESSOR_ARG *arg = (FIND_SUCESSOR_ARG *)ptr;
	FIND_SUCESSOR_RESULT result;
	int err;
	DHT_TABLE_EL dht_tbl;
	unsigned long seq;
	int i;
	char req_forward_woof[2048];
	char msg[256];

// if (strcmp(arg->callback_handler, "fix_fingers_callback") == 0)
// {
// 	log_set_level(LOG_INFO);
// }
	seq = WooFGetLatestSeqno(DHT_TABLE_WOOF);
	if (WooFInvalid(seq))
	{
		log_error("find_successor", "couldn't get latest dht_table seq_no");
		exit(1);
	}
	err = WooFGet(DHT_TABLE_WOOF, &dht_tbl, seq);
	if (err < 0)
	{
		sprintf(msg, "couldn't get latest dht_table with seq_no %lu", seq);
		log_error("find_successor", msg);
		exit(1);
	}
	sprintf(msg, "callback_woof: %s", arg->callback_woof);
	log_debug("find_successor", msg);
	sprintf(msg, "callback_handler: %s", arg->callback_handler);
	log_debug("find_successor", msg);
	sprintf(msg, "id_hash: ");
	print_node_hash(msg + 9, arg->id_hash);
	log_debug("find_successor", msg);
	sprintf(msg, "request_seq_no: %lu", arg->request_seq_no);
	log_debug("find_successor", msg);

	if (arg->request_seq_no == 0)
	{
		arg->request_seq_no = seq_no;
	}
	arg->hops++;

	// if id in (n, successor]
	if (in_range(arg->id_hash, dht_tbl.node_hash, dht_tbl.successor_hash[0])
		|| memcmp(arg->id_hash, dht_tbl.successor_hash[0], SHA_DIGEST_LENGTH) == 0)
	{
		// return successor
		if (arg->callback_woof[0] != 0)
		{
			memcpy(result.target_hash, arg->id_hash, SHA_DIGEST_LENGTH);
			result.request_seq_no = arg->request_seq_no;
			result.finger_index = arg->finger_index;
			result.hops = arg->hops;
			strncpy(result.node_addr, dht_tbl.successor_addr[0], sizeof(result.node_addr));
			memcpy(result.node_hash, dht_tbl.successor_hash[0], SHA_DIGEST_LENGTH);

			seq = WooFPut(arg->callback_woof, arg->callback_handler, &result);
			if (WooFInvalid(seq))
			{
				sprintf(msg, "couldn't put find_successor's result to woof %s", arg->callback_woof);
				log_error("find_successor", msg);
				exit(1);
			}
			sprintf(msg, "returned successor hash: ", result.node_hash);
			print_node_hash(msg + 25, result.node_hash);
			log_debug("find_successor", msg);
			sprintf(msg, "returned successor addr: %s", result.node_addr);
			log_debug("find_successor", msg);
		}
		return 1;
	}
	// else closest_preceding_node(id)
	for (i = SHA_DIGEST_LENGTH * 8; i > 0; i--)
	{
		if (dht_tbl.finger_addr[i][0] == 0)
		{
			continue;
		}
		// if finger[i] in (n, id)
		// return finger[i].find_succesor(id)
		if (in_range(dht_tbl.finger_hash[i], dht_tbl.node_hash, arg->id_hash))
		{
			sprintf(req_forward_woof, "%s/%s", dht_tbl.finger_addr[i], DHT_FIND_SUCCESSOR_ARG_WOOF);
			seq_no = WooFPut(req_forward_woof, "find_successor", arg);
			if (WooFInvalid(seq_no))
			{
				sprintf(msg, "couldn't put woof to %s", req_forward_woof);
				log_error("find_successor", msg);
				exit(1);
			}
			sprintf(msg, "forwarded find_succesor request to finger[%d]: %s", i, dht_tbl.finger_addr[i]);
			log_debug("find_successor", msg);
			return 1;
		}
	}

	// return n.find_succesor(id)
	sprintf(msg, "closest_preceding_node not found in finger table");
	log_debug("find_successor", msg);

	sprintf(req_forward_woof, "%s/%s", dht_tbl.node_addr, DHT_FIND_SUCCESSOR_ARG_WOOF);
	seq_no = WooFPut(req_forward_woof, "find_successor", arg);
	if (WooFInvalid(seq_no))
	{
		sprintf(msg, "couldn't put woof to %s", req_forward_woof);
		log_error("find_successor", msg);
		exit(1);
	}
	sprintf(msg, "forwarded find_succesor request to self: %s", dht_tbl.node_addr);
	log_debug("find_successor", msg);
	return 1;
}
