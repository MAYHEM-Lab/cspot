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
#include "dht_utils.h"

#define ARGS "w:"
char *Usage = "client_node_info -w node_woof\n";

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

	DHT_TABLE dht_table = {0};
	if (get_latest_element(dht_woof, &dht_table) < 0) {
		fprintf(stderr, "couldn't get latest dht_table: %s", dht_error_msg);
		exit(1);
	}

	int i;
	DHT_HASHMAP_ENTRY hmap_entry = {0};
	char woof_name[DHT_NAME_LENGTH];
	char hash[DHT_NAME_LENGTH];
	// node
	print_node_hash(hash, dht_table.node_hash);
	printf("node_hash: %s\n", hash);
	printf("node_addr: %s\n", dht_table.node_addr);
	for (i = 0; i < DHT_REPLICA_NUMBER; ++i) {
		if (dht_table.replicas[i][0] == 0) {
			break;
		}
		printf("node_replica: %s\n", dht_table.replicas[i]);
	}

	// predecessor
	print_node_hash(hash, dht_table.predecessor_hash);
	printf("predecessor_hash: %s\n", hash);
	if (is_empty(dht_table.predecessor_hash)) {
		printf("predecessor_addr: nil\n");
	} else {
		if (node_woof[0] == 0) {
			sprintf(woof_name, "%s", DHT_HASHMAP_WOOF);
		} else {
			sprintf(woof_name, "%s/%s", node_woof, DHT_HASHMAP_WOOF);
		}
		print_node_hash(woof_name + strlen(woof_name), dht_table.predecessor_hash);
		if (get_latest_element(woof_name, &hmap_entry) < 0) {
			fprintf(stderr, "failed to get the latest predeccessor entry from hashmap: %s", dht_error_msg);
			exit(1);
		}
		printf("predecessor_addr: %s\n", hmap_entry.replicas[hmap_entry.leader]);
		for (i = 0; i < DHT_REPLICA_NUMBER; ++i) {
			if (hmap_entry.replicas[i][0] == 0) {
				break;
			}
			printf("predecessor_replica: %s\n", hmap_entry.replicas[i]);
		}
	}
	
	// successor
	int j;
	for (j = 0; j < DHT_SUCCESSOR_LIST_R; ++j) {
		print_node_hash(hash, dht_table.successor_hash[j]);
		printf("successor_hash %d: %s\n", j, hash);
		if (is_empty(dht_table.successor_hash[j])) {
			printf("successor_addr %d: nil\n", j);
		} else {
			if (node_woof[0] == 0) {
				sprintf(woof_name, "%s", DHT_HASHMAP_WOOF);
			} else {
				sprintf(woof_name, "%s/%s", node_woof, DHT_HASHMAP_WOOF);
			}
			print_node_hash(woof_name + strlen(woof_name), dht_table.successor_hash[j]);
			if (get_latest_element(woof_name, &hmap_entry) < 0) {
				fprintf(stderr, "failed to get the latest predeccessor entry from hashmap: %s", dht_error_msg);
				exit(1);
			}
			printf("successor_addr %d: %s\n", j, hmap_entry.replicas[hmap_entry.leader]);
			for (i = 0; i < DHT_REPLICA_NUMBER; ++i) {
				if (hmap_entry.replicas[i][0] == 0) {
					break;
				}
				printf("successor_replica: %s\n", hmap_entry.replicas[i]);
			}
		}
	}
	
	// finger
	for (j = 1; j <= SHA_DIGEST_LENGTH * 8; ++j) {
		print_node_hash(hash, dht_table.finger_hash[j]);
		printf("finger_hash %d: %s\n", j, hash);
		if (is_empty(dht_table.finger_hash[j])) {
			printf("finger_addr %d: nil\n", j);
		} else {
			if (node_woof[0] == 0) {
				sprintf(woof_name, "%s", DHT_HASHMAP_WOOF);
			} else {
				sprintf(woof_name, "%s/%s", node_woof, DHT_HASHMAP_WOOF);
			}
			print_node_hash(woof_name + strlen(woof_name), dht_table.finger_hash[j]);
			if (get_latest_element(woof_name, &hmap_entry) < 0) {
				fprintf(stderr, "failed to get the latest predeccessor entry from hashmap: %s", dht_error_msg);
				exit(1);
			}
			printf("finger_addr %d: %s\n", j, hmap_entry.replicas[hmap_entry.leader]);
		}
	}
	return 0;
}
