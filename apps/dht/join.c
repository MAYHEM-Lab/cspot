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
char *Usage = "join -w node_woof\n";

char node_woof[256];

char *woof_to_create[] = {DHT_TABLE_WOOF, DHT_FIND_SUCESSOR_ARG_WOOF, DHT_FIND_SUCESSOR_RESULT_WOOF,
	DHT_GET_PREDECESSOR_ARG_WOOF, DHT_GET_PREDECESSOR_RESULT_WOOF, DHT_NOTIFY_ARG_WOOF,
	DHT_INIT_TOPIC_ARG_WOOF, DHT_SUBSCRIPTION_ARG_WOOF, DHT_TRIGGER_ARG_WOOF};
unsigned long woof_element_size[] = {sizeof(DHT_TABLE_EL), sizeof(FIND_SUCESSOR_ARG), sizeof(FIND_SUCESSOR_RESULT),
	sizeof(GET_PREDECESSOR_ARG), sizeof(GET_PREDECESSOR_RESULT), sizeof(NOTIFY_ARG),
	sizeof(INIT_TOPIC_ARG), sizeof(SUBSCRIPTION_ARG), sizeof(TRIGGER_ARG)};

int main(int argc, char **argv)
{
	// log_set_level(LOG_DEBUG);
	log_set_level(LOG_INFO);
	// FILE *f = fopen("log_join","w");
	// log_set_output(f);
	log_set_output(stdout);

	int i;
	int c;
	int err;
	DHT_TABLE_EL el;
	unsigned char node_hash[SHA_DIGEST_LENGTH];
	char woof_name[2048];
	unsigned long seq_no;
	FIND_SUCESSOR_ARG arg;
	char msg[256];

	while ((c = getopt(argc, argv, ARGS)) != EOF)
	{
		switch (c)
		{
		case 'w':
			strncpy(node_woof, optarg, sizeof(node_woof));
			break;
		default:
			fprintf(stderr,
					"unrecognized command %c\n", (char)c);
			fprintf(stderr, "%s", Usage);
			exit(1);
		}
	}

	err = node_woof_name(woof_name);
	if (err < 0)
	{
		log_error("join", "couldn't get local node's woof name");
		exit(1);
	}

	if (node_woof[0] == 0)
	{
		fprintf(stderr, "must specify a node woof to join\n");
		fprintf(stderr, "%s", Usage);
		fflush(stderr);
		exit(1);
	}

	WooFInit();

	for (i = 0; i < 9; i++)
	{
		err = WooFCreate(woof_to_create[i], woof_element_size[i], 10);
		if (err < 0)
		{
			sprintf(msg, "couldn't create woof %s", woof_to_create[i]);
			log_error("join", msg);
			exit(1);
		}
		sprintf(msg, "created woof %s", woof_to_create[i]);
		log_debug("join", msg);
	}

	// compute the node hash with SHA1
	SHA1(woof_name, strlen(woof_name), arg.id_hash);
	sprintf(msg, "woof_name: %s", woof_name);
	log_info("join", msg);
	sprintf(msg, "hash: ");
	print_node_hash(msg + 6, arg.id_hash);
	log_info("join", msg);

	sprintf(arg.callback_woof, "%s/%s", woof_name, DHT_FIND_SUCESSOR_RESULT_WOOF);
	sprintf(arg.callback_handler, "join_callback", sizeof(arg.callback_handler));
	seq_no = WooFPut(node_woof, "find_successor", &arg);
	if (WooFInvalid(seq_no))
	{
		sprintf(msg, "couldn't call find_successor on woof %s", node_woof);
		log_error("join", msg);
		exit(1);
	}
	sprintf(msg, "called find_successor on %s", node_woof);
	log_info("join", msg);

	return (0);
}
