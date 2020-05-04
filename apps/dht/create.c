#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <openssl/sha.h>

#include "woofc.h"
#include "woofc-host.h"
#include "dht.h"

char *woof_to_create[] = {DHT_TABLE_WOOF, DHT_FIND_SUCCESSOR_ARG_WOOF, DHT_FIND_SUCCESSOR_RESULT_WOOF,
	DHT_GET_PREDECESSOR_ARG_WOOF, DHT_GET_PREDECESSOR_RESULT_WOOF, DHT_NOTIFY_ARG_WOOF, DHT_NOTIFY_RESULT_WOOF,
	DHT_INIT_TOPIC_ARG_WOOF, DHT_SUBSCRIPTION_ARG_WOOF, DHT_TRIGGER_ARG_WOOF};
unsigned long woof_element_size[] = {sizeof(DHT_TABLE_EL), sizeof(FIND_SUCESSOR_ARG), sizeof(FIND_SUCESSOR_RESULT),
	sizeof(GET_PREDECESSOR_ARG), sizeof(GET_PREDECESSOR_RESULT), sizeof(NOTIFY_ARG), sizeof(NOTIFY_RESULT),
	sizeof(INIT_TOPIC_ARG), sizeof(SUBSCRIPTION_ARG), sizeof(TRIGGER_ARG)};

int main(int argc, char **argv) {
	DHT_TABLE_EL el;

	// log_set_level(LOG_DEBUG);
	log_set_level(LOG_INFO);
	// FILE *f = fopen("log_create","w");
	// log_set_output(f);
	log_set_output(stdout);
	WooFInit();

	int i;
	for (i = 0; i < (sizeof(woof_to_create) / sizeof(char *)); ++i) {
		if (WooFCreate(woof_to_create[i], woof_element_size[i], DHT_HISTORY_LENGTH) < 0) {
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
	char msg[256];
	sprintf(msg, "node_hash: ");
	print_node_hash(msg + strlen(msg), node_hash);
	log_info(msg);

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
