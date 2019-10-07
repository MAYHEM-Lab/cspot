#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <openssl/sha.h>

#include "woofc.h"
#include "woofc-host.h"
#include "dht.h"

char *woof_to_create[] = {DHT_TABLE_WOOF, DHT_FIND_SUCESSOR_ARG_WOOF, DHT_FIND_SUCESSOR_RESULT_WOOF,
	DHT_GET_PREDECESSOR_ARG_WOOF, DHT_GET_PREDECESSOR_RESULT_WOOF, DHT_NOTIFY_ARG_WOOF,
	DHT_INIT_TOPIC_ARG_WOOF, DHT_SUBSCRIPTION_ARG_WOOF, DHT_TRIGGER_ARG_WOOF};
unsigned long woof_element_size[] = {sizeof(DHT_TABLE_EL), sizeof(FIND_SUCESSOR_ARG), sizeof(FIND_SUCESSOR_RESULT),
	sizeof(GET_PREDECESSOR_ARG), sizeof(GET_PREDECESSOR_RESULT), sizeof(NOTIFY_ARG),
	sizeof(INIT_TOPIC_ARG), sizeof(SUBSCRIPTION_ARG), sizeof(TRIGGER_ARG)};

int main(int argc, char **argv)
{
	int err;
	int i;
	DHT_TABLE_EL el;
	unsigned char node_hash[SHA_DIGEST_LENGTH];
	char woof_name[2048];
	unsigned long seq_no;
	char msg[128];

	// log_set_level(LOG_DEBUG);
	log_set_level(LOG_INFO);
	// FILE *f = fopen("log_create","w");
	// log_set_output(f);
	log_set_output(stdout);
	WooFInit();

	for (i = 0; i < 9; i++)
	{
		err = WooFCreate(woof_to_create[i], woof_element_size[i], 10);
		if (err < 0)
		{
			sprintf(msg, "couldn't create woof %s", woof_to_create[i]);
			log_error("create", msg);
			exit(1);
		}
		sprintf(msg, "created woof %s", woof_to_create[i]);
		log_debug("create", msg);
	}

	// compute the node hash with SHA1
	err = node_woof_name(woof_name);
	if (err < 0)
	{
		log_error("create", "couldn't get local node's woof name");
		exit(1);
	}
	SHA1(woof_name, strlen(woof_name), node_hash);
	
	sprintf(msg, "woof_name: %s", woof_name);
	log_info("create", msg);
	sprintf(msg, "node_hash: ");
	print_node_hash(msg + strlen(msg), node_hash);
	log_info("create", msg);

	dht_init(node_hash, woof_name, &el);
	seq_no = WooFPut(DHT_TABLE_WOOF, NULL, &el);
	if (WooFInvalid(seq_no))
	{
		sprintf(msg, "couldn't initialize DHT to woof %s", DHT_TABLE_WOOF);
		log_error("create", msg);
		exit(1);
	}
	sprintf(msg, "updated woof %s", DHT_TABLE_WOOF);
	log_debug("create", msg);

	sprintf(msg, "predecessor: %s", el.predecessor_addr);
	log_info("create", msg);
	sprintf(msg, "successor: %s", el.successor_addr);
	log_info("create", msg);
	return (0);
}
