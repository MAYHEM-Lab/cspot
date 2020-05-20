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
	char woof_name[DHT_NAME_LENGTH];
	if (node_woof_name(woof_name) < 0) {
		fprintf(stderr, "failed to get local node's woof name");
		return -1;
	}

	if (dht_join_cluster(node_woof, woof_name) < 0) {
		fprintf(stderr, "failed to join cluster");
		exit(1);
	}

	return 0;
}
