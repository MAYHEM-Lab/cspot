#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <openssl/sha.h>

#include "woofc.h"
#include "woofc-host.h"
#include "dht.h"

char *woof_to_create[] = {DHT_TABLE_WOOF, DHT_FIND_SUCCESSOR_WOOF, DHT_FIND_ADDRESS_RESULT_WOOF, DHT_FIND_NODE_RESULT_WOOF, 
	DHT_INVOCATION_WOOF, DHT_JOIN_WOOF, DHT_GET_PREDECESSOR_WOOF, DHT_NOTIFY_WOOF, DHT_NOTIFY_CALLBACK_WOOF,
	DHT_STABLIZE_CALLBACK_WOOF, DHT_FIX_FINGER_WOOF, DHT_REGISTER_TOPIC_WOOF, DHT_SUBSCRIBE_WOOF, DHT_TRIGGER_WOOF};
unsigned long woof_element_size[] = {sizeof(DHT_TABLE), sizeof(DHT_FIND_SUCCESSOR_ARG), sizeof(DHT_FIND_ADDRESS_RESULT), sizeof(DHT_FIND_NODE_RESULT),
	sizeof(DHT_INVOCATION_ARG), sizeof(DHT_JOIN_ARG), sizeof(GET_PREDECESSOR_ARG), sizeof(NOTIFY_ARG), sizeof(NOTIFY_RESULT), 
	sizeof(DHT_STABLIZE_CALLBACK), sizeof(DHT_FIX_FINGER_ARG), sizeof(DHT_REGISTER_TOPIC_ARG), sizeof(DHT_SUBSCRIBE_ARG), sizeof(DHT_TRIGGER_ARG)};

int main(int argc, char **argv) {
	log_set_tag("create");
	// log_set_level(LOG_DEBUG);
	log_set_level(DHT_LOG_INFO);
	// FILE *f = fopen("log_create","w");
	// log_set_output(f);
	log_set_output(stdout);
	WooFInit();

	int i;
	for (i = 0; i < (sizeof(woof_to_create) / sizeof(char *)); ++i) {
		if (WooFCreate(woof_to_create[i], woof_element_size[i], DHT_HISTORY_LENGTH_LONG) < 0) {
			log_error("couldn't create woof %s", woof_to_create[i]);
			exit(1);
		}
		log_debug("created woof %s", woof_to_create[i]);
	}

	// compute the node hash with SHA1
	char woof_name[DHT_NAME_LENGTH];
	if (node_woof_name(woof_name) < 0) {
		log_error("couldn't get local node's woof name");
		exit(1);
	}
	log_info("woof_name: %s", woof_name);
	unsigned char node_hash[SHA_DIGEST_LENGTH];
	SHA1(woof_name, strlen(woof_name), node_hash);
	char hash_str[2 * SHA_DIGEST_LENGTH + 1];
	print_node_hash(hash_str, node_hash);
	log_info("node_hash: %s", hash_str);

	DHT_TABLE el;
	dht_init(node_hash, woof_name, &el);
	unsigned long seq_no = WooFPut(DHT_TABLE_WOOF, NULL, &el);
	if (WooFInvalid(seq_no)) {
		log_error("couldn't initialize DHT to woof %s", DHT_TABLE_WOOF);
		exit(1);
	}
	log_debug("updated woof %s", DHT_TABLE_WOOF);

	log_info("predecessor: %s", el.predecessor_addr);
	log_info("successor: %s", el.successor_addr[0]);
	return 0;
}
