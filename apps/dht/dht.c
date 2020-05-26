#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <openssl/sha.h>
#include "woofc.h"
#include "dht.h"
#include "dht_utils.h"

char DHT_WOOF_TO_CREATE[][DHT_NAME_LENGTH] = {
	DHT_CHECK_PREDECESSOR_WOOF,
	DHT_DAEMON_WOOF,
	DHT_FIND_NODE_RESULT_WOOF,
	DHT_FIND_SUCCESSOR_WOOF,
	DHT_FIX_FINGER_WOOF,
	DHT_FIX_FINGER_CALLBACK_WOOF,
	DHT_GET_PREDECESSOR_WOOF,
	DHT_INVOCATION_WOOF,
	DHT_JOIN_WOOF,
	DHT_NOTIFY_CALLBACK_WOOF,
	DHT_NOTIFY_WOOF,
	DHT_REGISTER_TOPIC_WOOF,
	DHT_STABLIZE_WOOF,
	DHT_STABLIZE_CALLBACK_WOOF,
	DHT_SUBSCRIBE_WOOF,
	DHT_TABLE_WOOF,
	DHT_TRIGGER_WOOF
};

unsigned long DHT_WOOF_ELEMENT_SIZE[] = {
	sizeof(DHT_CHECK_PREDECESSOR_ARG),
	sizeof(DHT_DAEMON_ARG),
	sizeof(DHT_FIND_NODE_RESULT),
	sizeof(DHT_FIND_SUCCESSOR_ARG),
	sizeof(DHT_FIX_FINGER_ARG),
	sizeof(DHT_FIX_FINGER_CALLBACK_ARG),
	sizeof(DHT_GET_PREDECESSOR_ARG),
	sizeof(DHT_INVOCATION_ARG),
	sizeof(DHT_JOIN_ARG),
	sizeof(DHT_NOTIFY_CALLBACK_ARG),
	sizeof(DHT_NOTIFY_ARG),
	sizeof(DHT_REGISTER_TOPIC_ARG),
	sizeof(DHT_STABLIZE_ARG),
	sizeof(DHT_STABLIZE_CALLBACK_ARG),
	sizeof(DHT_SUBSCRIBE_ARG),
	sizeof(DHT_TABLE),
	sizeof(DHT_TRIGGER_ARG)
};

int dht_create_woofs() {
	int num_woofs = sizeof(DHT_WOOF_TO_CREATE) / DHT_NAME_LENGTH;
	int i;
	for (i = 0; i < num_woofs; ++i) {
		if (WooFCreate(DHT_WOOF_TO_CREATE[i], DHT_WOOF_ELEMENT_SIZE[i], DHT_HISTORY_LENGTH_LONG) < 0) {
			return -1;
		}
	}
	return 0;
}

int dht_start_daemon() {
	DHT_DAEMON_ARG arg;
	arg.last_stablize = 0;
	arg.last_check_predecessor = 0;
	arg.last_fix_finger = 0;
	arg.last_fixed_finger_index = 1;
	unsigned long seq = WooFPut(DHT_DAEMON_WOOF, "d_daemon", &arg);
	if (WooFInvalid(seq)) {
		return -1;
	}
	return 0;
}

int dht_create_cluster(char *woof_name, char replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH]) {
	int replica_id;
	for (replica_id = 0; replica_id < DHT_REPLICA_NUMBER; ++replica_id) {
		if (strcmp(woof_name, replicas[replica_id]) == 0) {
			break;
		}
	}
	if (replica_id == DHT_REPLICA_NUMBER) {
		sprintf(dht_error_msg, "%s is not in replicas", woof_name);
		return -1;
	}

	if (dht_create_woofs() < 0) {
		sprintf(dht_error_msg, "can't create woofs");
		return -1;
	}

	unsigned char node_hash[SHA_DIGEST_LENGTH];
	SHA1(woof_name, strlen(woof_name), node_hash);
	
	if (hmap_set_replicas(node_hash, replicas, replica_id) < 0) {
		sprintf(dht_error_msg, "failed to set node's replicas in hashmap: %s", dht_error_msg);
		return -1;
	}

	DHT_TABLE dht_table = {0};
	dht_init(node_hash, woof_name, replicas, replica_id, &dht_table);
	unsigned long seq_no = WooFPut(DHT_TABLE_WOOF, NULL, &dht_table);
	if (WooFInvalid(seq_no)) {
		sprintf(dht_error_msg, "failed to initialize DHT to woof %s", DHT_TABLE_WOOF);
		return -1;
	}

	if (dht_start_daemon() < 0) {
		sprintf(dht_error_msg, "failed to start daemon");
		return -1;
	}
	char h[DHT_NAME_LENGTH];
	print_node_hash(h, node_hash);
	printf("node_hash: %s\nnode_addr: %s\nid: %d\n", h, woof_name, replica_id);
	int i;
	for (i = 0; i < DHT_REPLICA_NUMBER; ++i) {
		printf("replica: %s\n", replicas[i]);
	}
	return 0;
}

int dht_join_cluster(char *node_woof, char *woof_name, char replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH]) {
	int replica_id;
	for (replica_id = 0; replica_id < DHT_REPLICA_NUMBER; ++replica_id) {
		if (strcmp(woof_name, replicas[replica_id]) == 0) {
			break;
		}
	}
	if (replica_id == DHT_REPLICA_NUMBER) {
		sprintf(dht_error_msg, "%s is not in replicas", woof_name);
		return -1;
	}

	if (dht_create_woofs() < 0) {
		sprintf(dht_error_msg, "can't create woofs");
		return -1;
	}

	unsigned char node_hash[SHA_DIGEST_LENGTH];
	SHA1(woof_name, strlen(woof_name), node_hash);

	if (hmap_set_replicas(node_hash, replicas, replica_id) < 0) {
		sprintf(dht_error_msg, "failed to set node's replicas in hashmap: %s", dht_error_msg);
		return -1;
	}
	
	DHT_TABLE dht_table;
	dht_init(node_hash, woof_name, replicas, replica_id, &dht_table);
	unsigned long seq_no = WooFPut(DHT_TABLE_WOOF, NULL, &dht_table);
	if (WooFInvalid(seq_no)) {
		sprintf(dht_error_msg, "failed to initialize DHT");
		return -1;
	}

	DHT_FIND_SUCCESSOR_ARG arg;
	dht_init_find_arg(&arg, woof_name, node_hash, woof_name);
	arg.action = DHT_ACTION_JOIN;
	if (node_woof[strlen(node_woof) - 1] == '/') {
		sprintf(node_woof, "%s%s", node_woof, DHT_FIND_SUCCESSOR_WOOF);
	} else {
		sprintf(node_woof, "%s/%s", node_woof, DHT_FIND_SUCCESSOR_WOOF);
	}
	seq_no = WooFPut(node_woof, "h_find_successor", &arg);
	if (WooFInvalid(seq_no)) {
		sprintf(dht_error_msg, "failed to invoke find_successor on %s", node_woof);
		return -1;
	}

	return 0;
}
