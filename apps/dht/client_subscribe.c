// #define DEBUG

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <openssl/sha.h>

#include "woofc.h"
#include "woofc-host.h"
#include "dht.h"
#include "dht_client.h"

#define ARGS "t:h:"
char *Usage = "client_subscribe -t topic -h handler\n";

int main(int argc, char **argv) {
	char topic[DHT_NAME_LENGTH];
	char handler[DHT_NAME_LENGTH];
	int c;
	while ((c = getopt(argc, argv, ARGS)) != EOF) {
		switch (c) {
			case 't': {
				strncpy(topic, optarg, sizeof(topic));
				break;
			}
			case 'h': {
				strncpy(handler, optarg, sizeof(handler));
				break;
			}
			default: {
				fprintf(stderr, "unrecognized command %c\n", (char)c);
				fprintf(stderr, "%s", Usage);
				exit(1);
			}
		}
	}

	if (topic[0] == 0 || handler[0] == 0) {
		fprintf(stderr, "%s", Usage);
		exit(1);
	}

	WooFInit();

	char woof_name[DHT_NAME_LENGTH];
	if (node_woof_name(woof_name) < 0) {
		printf("couldn't get local woof name\n");
		return 0;
	}
	
	if (dht_subscribe(topic, handler) < 0) {
		fprintf(stderr, "failed to subscribe to topic\n");
		exit(1);
	}
	
	return 0;
}
