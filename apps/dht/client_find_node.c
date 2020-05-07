#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <openssl/sha.h>

#include "woofc.h"
#include "woofc-host.h"
#include "dht.h"
#include "client.h"

#define ARGS "w:t:"
char *Usage = "client_find -t topic\n";

int main(int argc, char **argv) {
	char server[DHT_NAME_LENGTH];
	char topic[DHT_NAME_LENGTH];
	
	int c;
	while ((c = getopt(argc, argv, ARGS)) != EOF) {
		switch (c) {
			case 'w': {
				strncpy(server, optarg, sizeof(server));
			}
			case 't': {
				strncpy(topic, optarg, sizeof(topic));
				break;
			}
			default: {
				fprintf(stderr, "unrecognized command %c\n", (char)c);
				fprintf(stderr, "%s", Usage);
				exit(1);
			}
		}
	}

	if (topic[0] == 0) {
		fprintf(stderr, "must specify topic\n");
		fprintf(stderr, "%s", Usage);
		exit(1);
	}

	WooFInit();

	char local_namespace[DHT_NAME_LENGTH];
	if (node_woof_name(local_namespace) < 0) {
		fprintf(stderr, "find: couldn't get local node's woof name\n");
		exit(1);
	}
	
	char result_node[DHT_NAME_LENGTH];
	if (server[0] != 0) {
		if (dht_find_node(server, topic, result_node) < 0) {
			fprintf(stderr, "failed to find the topic\n");
			exit(1);
		}
	} else {
		if (dht_find_node(NULL, topic, result_node) < 0) {
			fprintf(stderr, "failed to find the topic\n");
			exit(1);
		}
	}
	printf("node: %s\n", result_node);

	return 0;
}
