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

int main(int argc, char **argv) {
	log_set_tag("join");
	log_set_level(DHT_LOG_DEBUG);
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

	if (join_cluster(node_woof) < 0) {
		log_error("failed to join cluster");
		exit(1);
	}

	return 0;
}
