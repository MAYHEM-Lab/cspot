#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <openssl/sha.h>

#include "woofc.h"
#include "woofc-host.h"
#include "dht.h"

#define ARGS "t:n:"
char *Usage = "find -t topic -n node_addr\n";

int main(int argc, char **argv) {
	char topic[DHT_NAME_LENGTH];
	char node_addr[DHT_NAME_LENGTH];
	
	int c;
	while ((c = getopt(argc, argv, ARGS)) != EOF) {
		switch (c) {
			case 't': {
				strncpy(topic, optarg, sizeof(topic));
				break;
			}
			case 'n': {
				strncpy(node_addr, optarg, sizeof(node_addr));
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

	char local_woof[DHT_NAME_LENGTH];
	if (node_woof_name(local_woof) < 0) {
		fprintf(stderr, "find: couldn't get local node's woof name\n");
		exit(1);
	}

	if (node_addr[0] == 0) {
		fprintf(stderr, "must specify node woof\n");
		fprintf(stderr, "%s", Usage);
		exit(1);
	}

	FIND_SUCESSOR_ARG arg;
	find_init(local_woof, "h_find_result", topic, &arg);

	WooFInit();
	char remote_woof[DHT_NAME_LENGTH];
	sprintf(remote_woof, "%s/%s", node_addr, DHT_FIND_SUCCESSOR_ARG_WOOF);
	unsigned long seq_no = WooFPut(remote_woof, "h_find_successor", &arg);
	if (WooFInvalid(seq_no)) {
		fprintf(stderr, "couldn't put woof to %s\n", DHT_FIND_SUCCESSOR_ARG_WOOF);
		exit(1);
	}

	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	printf("%lu, %lu\n", seq_no, ts.tv_sec * 1000 + ts.tv_nsec / 1000000);

	return 0;
}
