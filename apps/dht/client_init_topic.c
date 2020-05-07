// #define DEBUG

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <openssl/sha.h>

#include "woofc.h"
#include "woofc-host.h"
#include "dht.h"
#include "client.h"

#define ARGS "t:s:n:"
char *Usage = "client_subscribe -t topic -s element_size -n history_size\n\
-w to specify a remote server, default to local\n";

int main(int argc, char **argv) {
	char server[DHT_NAME_LENGTH];
	char topic[DHT_NAME_LENGTH];
	unsigned long element_size;
	unsigned long history_size;

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
			case 's': {
				element_size = strtoul(optarg, NULL, 0);
				break;
			}
			case 'n': {
				history_size = strtoul(optarg, NULL, 0);
				break;
			}
			default: {
				fprintf(stderr, "unrecognized command %c\n", (char)c);
				fprintf(stderr, "%s", Usage);
				exit(1);
			}
		}
	}

	if (topic[0] == 0 || element_size == 0 || history_size == 0) {
		fprintf(stderr, "%s", Usage);
		exit(1);
	}

	WooFInit();

	char woof_name[DHT_NAME_LENGTH];
	if (node_woof_name(woof_name) < 0) {
		printf("couldn't get local woof namespace\n");
		return 0;
	}

	if (dht_create_topic(topic, element_size, history_size) < 0) {
		fprintf(stderr, "failed to create topic woof\n");
		exit(1);
	}

	if (server[0] == 0) {
		if (dht_register_topic(NULL, topic) < 0) {
			fprintf(stderr, "failed to register topic on DHT\n");
			exit(1);
		}
	} else {
		if (dht_register_topic(server, topic) < 0) {
			fprintf(stderr, "failed to register topic on DHT\n");
			exit(1);
		}
	}
	
	return 0;
}
