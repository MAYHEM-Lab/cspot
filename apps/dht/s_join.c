// #define DEBUG

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <openssl/sha.h>

#include "woofc.h"
#include "woofc-host.h"
#include "dht.h"

#define ARGS "w:"
char *Usage = "s_join -w node_woof\n";

char node_woof[DHT_NAME_LENGTH];

char *woof_to_create[] = {DHT_TABLE_WOOF, DHT_FIND_SUCCESSOR_WOOF, DHT_FIND_ADDRESS_RESULT_WOOF, DHT_FIND_NODE_RESULT_WOOF, 
	DHT_INVOCATION_WOOF, DHT_JOIN_WOOF, DHT_GET_PREDECESSOR_WOOF, DHT_NOTIFY_WOOF, DHT_NOTIFY_CALLBACK_WOOF,
	DHT_STABLIZE_CALLBACK_WOOF, DHT_FIX_FINGER_WOOF, DHT_REGISTER_TOPIC_WOOF, DHT_SUBSCRIBE_WOOF, DHT_TRIGGER_WOOF};
unsigned long woof_element_size[] = {sizeof(DHT_TABLE), sizeof(DHT_FIND_SUCCESSOR_ARG), sizeof(DHT_FIND_ADDRESS_RESULT), sizeof(DHT_FIND_NODE_RESULT),
	sizeof(DHT_INVOCATION_ARG), sizeof(DHT_JOIN_ARG), sizeof(GET_PREDECESSOR_ARG), sizeof(NOTIFY_ARG), sizeof(NOTIFY_RESULT), 
	sizeof(DHT_STABLIZE_CALLBACK), sizeof(DHT_FIX_FINGER_ARG), sizeof(DHT_REGISTER_TOPIC_ARG), sizeof(DHT_SUBSCRIBE_ARG), sizeof(DHT_TRIGGER_ARG)};

int main(int argc, char **argv) {
	log_set_tag("join");
	// log_set_level(LOG_DEBUG);
	log_set_level(DHT_LOG_INFO);
	// FILE *f = fopen("log_join","w");
	// log_set_output(f);
	log_set_output(stdout);

	int c;
	while ((c = getopt(argc, argv, ARGS)) != EOF) {
		switch (c) {
			case 'w': {
				strncpy(node_woof, optarg, sizeof(node_woof));
				break;
			}
			default: {
				fprintf(stderr, "unrecognized command %c\n", (char)c);
				fprintf(stderr, "%s", Usage);
				exit(1);
			}
		}
	}

	if (node_woof[0] == 0) {
		fprintf(stderr, "must specify a node woof to join\n");
		fprintf(stderr, "%s", Usage);
		exit(1);
	}
	
	WooFInit();

	int i;
	for (i = 0; i < (sizeof(woof_to_create) / sizeof(char *)); ++i) {
		if (WooFCreate(woof_to_create[i], woof_element_size[i], DHT_HISTORY_LENGTH_LONG) < 0) {
			log_error("couldn't create woof %s", woof_to_create[i]);
			exit(1);
		}
		log_debug("created woof %s", woof_to_create[i]);
	}

	char woof_name[DHT_NAME_LENGTH];
	if (node_woof_name(woof_name) < 0) {
		log_error("couldn't get local node's woof name");
		exit(1);
	}
	log_info("woof_name: %s", woof_name);
	char hashed_key[SHA_DIGEST_LENGTH];
	SHA1(woof_name, strlen(woof_name), hashed_key);
	char hash_str[2 * SHA_DIGEST_LENGTH + 1];
	print_node_hash(hash_str, hashed_key);
	log_info("hashed_key: %s", hash_str);

	// compute the node hash with SHA1
	DHT_FIND_SUCCESSOR_ARG arg;
	dht_init_find_arg(&arg, woof_name, hashed_key, woof_name);
	arg.action = DHT_ACTION_JOIN;
	if (node_woof[strlen(node_woof) - 1] == '/') {
		sprintf(node_woof, "%s%s", node_woof, DHT_FIND_SUCCESSOR_WOOF);
	} else {
		sprintf(node_woof, "%s/%s", node_woof, DHT_FIND_SUCCESSOR_WOOF);
	}
	unsigned long seq_no = WooFPut(node_woof, "h_find_successor", &arg);
	if (WooFInvalid(seq_no)) {
		log_error("failed to invoke find_successor on %s", node_woof);
		exit(1);
	}
	log_info("invoked h_find_successor on %s", node_woof);

	return 0;
}
