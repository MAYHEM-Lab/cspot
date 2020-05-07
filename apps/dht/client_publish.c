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

#define ARGS "w:t:d:"
char *Usage = "client_publish -t topic -d data\n\
-w to specify a remote server, default to local\n";

int main(int argc, char **argv) {
	char server[DHT_NAME_LENGTH];
	char topic[DHT_NAME_LENGTH];
	char data[4096];
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
			case 'd': {
				strncpy(data, optarg, sizeof(data));
				break;
			}
			default: {
				fprintf(stderr, "unrecognized command %c\n", (char)c);
				fprintf(stderr, "%s", Usage);
				exit(1);
			}
		}
	}

	if (topic[0] == 0 || data[0] == 0) {
		fprintf(stderr, "%s", Usage);
		exit(1);
	}

	WooFInit();
	
	char woof_name[DHT_NAME_LENGTH];
	if (node_woof_name(woof_name) < 0) {
		printf("couldn't get local woof name\n");
		return 0;
	}
	
	unsigned long seq;
	if (server[0] == 0) {
		seq = dht_publish(NULL, topic, data);
	} else {
		seq = dht_publish(server, topic, data);
	}
	if (WooFInvalid(seq)) {
		fprintf(stderr, "failed to publish data\n");
		exit(1);
	}
	
	return 0;
}
