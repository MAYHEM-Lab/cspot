#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "dht.h"

void get_finger_id(unsigned char *dst, const unsigned char *n, int i) {
	unsigned char carry = 0;
	// dst = n + 2^(i-1)
	int shift = i - 1;
	int j;
	for (j = SHA_DIGEST_LENGTH - 1; j >= 0; j--) {
		dst[j] = carry;
		if (SHA_DIGEST_LENGTH - 1 - (shift / 8) == j) {
			dst[j] += (unsigned char)(1 << (shift % 8));
		}
		if (n[j] >= (256 - dst[j])) {
			carry = 1;
		} else {
			carry = 0;
		}
		dst[j] += n[j];
	}
}

int d_fix_finger(WOOF *wf, unsigned long seq_no, void *ptr) {
	DHT_FIX_FINGER_ARG *arg = (DHT_FIX_FINGER_ARG *)ptr;

	log_set_tag("fix_finger");
	// log_set_level(DHT_LOG_DEBUG);
	log_set_level(DHT_LOG_INFO);
	log_set_output(stdout);

	char woof_name[DHT_NAME_LENGTH];
	if (node_woof_name(woof_name) < 0) {
		log_error("failed to get local node's woof name");
		exit(1);
	}

	// compute the node hash with SHA1
	unsigned char node_hash[SHA_DIGEST_LENGTH];
	SHA1(woof_name, strlen(woof_name), node_hash);
		
	// finger[i] = find_successor(n + 2^(i-1))
	// finger_hash = n + 2^(i-1)
	char hashed_finger_id[SHA_DIGEST_LENGTH];
	get_finger_id(hashed_finger_id, node_hash, arg->finger_index);
	DHT_FIND_SUCCESSOR_ARG find_sucessor_arg;
	dht_init_find_arg(&find_sucessor_arg, "", hashed_finger_id, woof_name);
	find_sucessor_arg.action = DHT_ACTION_FIX_FINGER;
	find_sucessor_arg.action_seqno = (unsigned long)arg->finger_index;
	// sprintf(msg, "fixing finger[%d](", finger_id);
	// print_node_hash(msg + strlen(msg), arg.id_hash);
	// log_debug("fix_finger", msg);

	unsigned long seq = WooFPut(DHT_FIND_SUCCESSOR_WOOF, "h_find_successor", &find_sucessor_arg);
	if (WooFInvalid(seq)) {
		log_error("failed to invoke find_successor on woof %s", DHT_FIND_SUCCESSOR_WOOF);
	}
	
	return 1;
}
