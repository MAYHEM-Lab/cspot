#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <openssl/sha.h>

#include "woofc.h"
#include "woofc-host.h"
#include "dht.h"

#define ARGS "w:"
char *Usage = "node_info -w node_woof\n";

int main(int argc, char **argv) {
	char node_woof[DHT_NAME_LENGTH];
	
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

	char dht_woof[DHT_NAME_LENGTH];
	if (node_woof[0] == 0) {
		WooFInit();
		sprintf(dht_woof, "%s", DHT_TABLE_WOOF);
	} else {
		sprintf(dht_woof, "%s/%s", node_woof, DHT_TABLE_WOOF);
	}

	unsigned long seq_no = WooFGetLatestSeqno(dht_woof);
	if (WooFInvalid(seq_no)) {
		fprintf(stderr, "couldn't get latest dht_table seq_no");
		return 0;
	}
	DHT_TABLE_EL dht_tbl;
	if (WooFGet(dht_woof, &dht_tbl, seq_no) < 0) {
		fprintf(stderr, "couldn't get latest dht_table with seq_no %lu", seq_no);
		return 0;
	}

	char hash[DHT_NAME_LENGTH];
	print_node_hash(hash, dht_tbl.node_hash);
	printf("node_hash: %s\n", hash);
	printf("node_addr: %s\n", dht_tbl.node_addr);
	print_node_hash(hash, dht_tbl.predecessor_hash);
	printf("predecessor_hash: %s\n", hash);
	printf("predecessor_addr: %s\n", dht_tbl.predecessor_addr);
	int i;
	for (i = 0; i < DHT_SUCCESSOR_LIST_R; ++i) {
		print_node_hash(hash, dht_tbl.successor_hash[i]);
		printf("successor_hash %d: %s\n", i, hash);
		printf("successor_addr %d: %s\n", i, dht_tbl.successor_addr[i]);
	}
	for (i = 1; i <= SHA_DIGEST_LENGTH * 8; ++i) {
		print_node_hash(hash, dht_tbl.finger_hash[i]);
		printf("finger_hash %d: %s\n", i, hash);
		printf("finger_addr %d: %s\n", i, dht_tbl.finger_addr[i]);
	}
	return 0;
}
