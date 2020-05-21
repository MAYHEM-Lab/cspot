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

void dht_init(unsigned char *node_hash, char *node_addr, DHT_TABLE *dht_table) {
	memcpy(dht_table->node_hash, node_hash, sizeof(dht_table->node_hash));
	memcpy(dht_table->node_addr, node_addr, sizeof(dht_table->node_addr));
	memset(dht_table->finger_addr, 0, sizeof(dht_table->finger_addr));
	memset(dht_table->finger_hash, 0, sizeof(dht_table->finger_hash));
	memset(dht_table->successor_addr, 0, sizeof(dht_table->successor_addr));
	memset(dht_table->successor_hash, 0, sizeof(dht_table->successor_hash));
	memcpy(dht_table->successor_hash[0], node_hash, sizeof(dht_table->successor_hash[0]));
	memcpy(dht_table->successor_addr[0], node_addr, sizeof(dht_table->successor_addr[0]));
	memset(dht_table->predecessor_addr, 0, sizeof(dht_table->predecessor_addr));
	memset(dht_table->predecessor_hash, 0, sizeof(dht_table->predecessor_hash));
}

void dht_init_find_arg(DHT_FIND_SUCCESSOR_ARG *arg, char *key, char *hashed_key, char *callback_namespace) {
	arg->hops = 0;
	memcpy(arg->key, key, sizeof(arg->key));
	memcpy(arg->hashed_key, hashed_key, sizeof(arg->hashed_key));
	strcpy(arg->callback_namespace, callback_namespace);
}

void print_node_hash(char *dst, const unsigned char *id_hash) {
	int i;
	sprintf(dst, "");
	for (i = 0; i < SHA_DIGEST_LENGTH; i++)
	{
		sprintf(dst, "%s%02X", dst, id_hash[i]);
	}
}

// if n in (lower, upper)
int in_range(unsigned char *n, unsigned char *lower, unsigned char *upper) {
	if (memcmp(lower, upper, SHA_DIGEST_LENGTH) < 0) {
		if (memcmp(lower, n, SHA_DIGEST_LENGTH) < 0 && memcmp(n, upper, SHA_DIGEST_LENGTH) < 0) {
			return 1;
		}
		return 0;
	}
	if (memcmp(lower, n, SHA_DIGEST_LENGTH) < 0 || memcmp(n, upper, SHA_DIGEST_LENGTH) < 0) {
		return 1;
	}
	return 0;
}

void shift_successor_list(char successor_addr[DHT_SUCCESSOR_LIST_R][DHT_NAME_LENGTH], unsigned char successor_hash[DHT_SUCCESSOR_LIST_R][SHA_DIGEST_LENGTH]) {
	int i;
	for (i = 0; i < DHT_SUCCESSOR_LIST_R - 1; ++i){
		memcpy(successor_addr[i], successor_addr[i + 1], sizeof(successor_addr[i]));
		memcpy(successor_hash[i], successor_hash[i + 1], sizeof(successor_hash[i]));
	}
	memset(successor_addr[DHT_SUCCESSOR_LIST_R - 1], 0, sizeof(successor_addr[DHT_SUCCESSOR_LIST_R - 1]));
	memset(successor_hash[DHT_SUCCESSOR_LIST_R - 1], 0, sizeof(successor_hash[DHT_SUCCESSOR_LIST_R - 1]));
}

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
	sizeof(DHT_TABLE);
	return 0;
}

int dht_create_cluster(char *woof_name) {
	if (dht_create_woofs() < 0) {
		sprintf(dht_error_msg, "can't create woofs");
		return -1;
	}

	unsigned char node_hash[SHA_DIGEST_LENGTH];
	SHA1(woof_name, strlen(woof_name), node_hash);
	
	DHT_TABLE dht_table;
	dht_init(node_hash, woof_name, &dht_table);
	unsigned long seq_no = WooFPut(DHT_TABLE_WOOF, NULL, &dht_table);
	if (WooFInvalid(seq_no)) {
		sprintf(dht_error_msg, "failed to initialize DHT to woof %s", DHT_TABLE_WOOF);
		return -1;
	}

	if (dht_start_daemon() < 0) {
		sprintf(dht_error_msg, "failed to start daemon");
		return -1;
	}
	return 0;
}

int dht_join_cluster(char *node_woof, char *woof_name) {
	if (dht_create_woofs() < 0) {
		sprintf(dht_error_msg, "can't create woofs");
		return -1;
	}

	char hashed_key[SHA_DIGEST_LENGTH];
	SHA1(woof_name, strlen(woof_name), hashed_key);

	DHT_FIND_SUCCESSOR_ARG arg;
	dht_init_find_arg(&arg, woof_name, hashed_key, woof_name);
	arg.action = DHT_ACTION_JOIN;
	if (node_woof[strlen(node_woof) - 1] == '/') {
		sprintf(node_woof, "%s%s", node_woof, DHT_FIND_SUCCESSOR_WOOF);
	} else {
		sprintf(node_woof, "%s/%s", node_woof, DHT_FIND_SUCCESSOR_WOOF);
	}
	unsigned long seq_no = WooFPut(node_woof, "h_find_successor", &arg);
	if (WooFInvalid(seq_no)) {
		sprintf(dht_error_msg, "failed to invoke find_successor on %s", node_woof);
		return -1;
	}

	if (dht_start_daemon() < 0) {
		sprintf(dht_error_msg, "failed to start daemon");
		return -1;
	}
	return 0;
}
