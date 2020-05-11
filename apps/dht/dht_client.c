#include <stdio.h>
#include <string.h>
#include <time.h>
#include "woofc.h"
#include "woofc-access.h"
#include "dht.h"
#include "dht_client.h"

int dht_find_addr(char *topic_name, char *result_addr) {
	char local_namespace[DHT_NAME_LENGTH];
	if (node_woof_name(local_namespace) < 0) {
		fprintf(stderr, "failed to get local woof namespace\n");
		return -1;
	}

	char hashed_key[SHA_DIGEST_LENGTH];
	SHA1(topic_name, strlen(topic_name), hashed_key);
	DHT_FIND_SUCCESSOR_ARG arg;
	dht_init_find_arg(&arg, topic_name, hashed_key, local_namespace);
	arg.action = DHT_ACTION_FIND_ADDRESS;

	char target_woof[DHT_NAME_LENGTH];
	char result_woof[DHT_NAME_LENGTH];
	sprintf(target_woof, "%s/%s", local_namespace, DHT_FIND_SUCCESSOR_WOOF);
	sprintf(result_woof, "%s/%s", local_namespace, DHT_FIND_ADDRESS_RESULT_WOOF);
	unsigned long last_checked_result = WooFGetLatestSeqno(result_woof);

	unsigned long seq_no = WooFPut(target_woof, "h_find_successor", &arg);
	if (WooFInvalid(seq_no)) {
		fprintf(stderr, "failed to invoke h_find_successor on %s\n", target_woof);
		exit(1);
	}

	while (1) {
		unsigned long latest_result = WooFGetLatestSeqno(result_woof);
		unsigned long i;
		for (i = last_checked_result + 1; i <= latest_result; ++i) {
			DHT_FIND_ADDRESS_RESULT result;
			if (WooFGet(result_woof, &result, i) < 0) {
				fprintf(stderr, "failed to get result at %lu from %s\n", i, result_woof);
			}
			if (memcmp(result.topic, topic_name, SHA_DIGEST_LENGTH) == 0) {
				if (result.topic_addr[0] == 0) {
					return -1;
				}
				strcpy(result_addr, result.topic_addr);
				return 0;
			}
		}
		last_checked_result = latest_result;
		usleep(10 * 1000);
	}
}

int dht_find_node(char *topic_name, char *result_node) {
	char local_namespace[DHT_NAME_LENGTH];
	if (node_woof_name(local_namespace) < 0) {
		fprintf(stderr, "failed to get local woof namespace\n");
		return -1;
	}

	char hashed_key[SHA_DIGEST_LENGTH];
	SHA1(topic_name, strlen(topic_name), hashed_key);
	DHT_FIND_SUCCESSOR_ARG arg;
	dht_init_find_arg(&arg, topic_name, hashed_key, local_namespace);
	arg.action = DHT_ACTION_FIND_NODE;

	char target_woof[DHT_NAME_LENGTH];
	char result_woof[DHT_NAME_LENGTH];
	sprintf(target_woof, "%s/%s", local_namespace, DHT_FIND_SUCCESSOR_WOOF);
	sprintf(result_woof, "%s/%s", local_namespace, DHT_FIND_NODE_RESULT_WOOF);
	unsigned long last_checked_result = WooFGetLatestSeqno(result_woof);

	unsigned long seq_no = WooFPut(target_woof, "h_find_successor", &arg);
	if (WooFInvalid(seq_no)) {
		fprintf(stderr, "failed to invoke h_find_successor on %s\n", target_woof);
		exit(1);
	}

	while (1) {
		unsigned long latest_result = WooFGetLatestSeqno(result_woof);
		unsigned long i;
		for (i = last_checked_result + 1; i <= latest_result; ++i) {
			DHT_FIND_NODE_RESULT result;
			if (WooFGet(result_woof, &result, i) < 0) {
				fprintf(stderr, "failed to get result at %lu from %s\n", i, result_woof);
			}
			if (memcmp(result.topic, topic_name, SHA_DIGEST_LENGTH) == 0) {
				strcpy(result_node, result.node_addr);
				return 0;
			}
		}
		last_checked_result = latest_result;
		usleep(10 * 1000);
	}
}

// call this on the node hosting the topic
int dht_create_topic(char *topic_name, unsigned long element_size, unsigned long history_size) {
	if (WooFCreate(topic_name, DHT_NAME_LENGTH + element_size, history_size) < 0) {
		fprintf(stderr, "failed to create woof %s", topic_name);
		return -1;
	}
	return 0;
}

int dht_register_topic(char *topic_name) {
	char local_namespace[DHT_NAME_LENGTH];
	if (node_woof_name(local_namespace) < 0) {
		fprintf(stderr, "failed to get local woof namespace\n");
		return -1;
	}

	DHT_REGISTER_TOPIC_ARG register_topic_arg;
	strcpy(register_topic_arg.topic_name, topic_name);
	strcpy(register_topic_arg.topic_namespace, local_namespace);
	unsigned long action_seqno = WooFPut(DHT_REGISTER_TOPIC_WOOF, NULL, &register_topic_arg);
	if (WooFInvalid(action_seqno)) {
		fprintf(stderr, "failed to store subscribe arg\n");
		return -1;
	}

	char hashed_key[SHA_DIGEST_LENGTH];
	SHA1(topic_name, strlen(topic_name), hashed_key);
	DHT_FIND_SUCCESSOR_ARG arg;
	dht_init_find_arg(&arg, topic_name, hashed_key, ""); // namespace doesn't matter, acted in target namespace
	arg.action = DHT_ACTION_REGISTER_TOPIC;
	strcpy(arg.action_namespace, local_namespace);
	arg.action_seqno = action_seqno;

	char target_woof[DHT_NAME_LENGTH];
	sprintf(target_woof, "%s/%s", local_namespace, DHT_FIND_SUCCESSOR_WOOF);
	unsigned long seq_no = WooFPut(target_woof, "h_find_successor", &arg);
	if (WooFInvalid(seq_no)) {
		fprintf(stderr, "failed to invoke h_find_successor on %s\n", target_woof);
		exit(1);
	}
	return 0;
}

int dht_subscribe(char *topic_name, char *handler) {
	char local_namespace[DHT_NAME_LENGTH];
	if (node_woof_name(local_namespace) < 0) {
		fprintf(stderr, "failed to get local woof namespace\n");
		return -1;
	}

	DHT_SUBSCRIBE_ARG subscribe_arg;
	strcpy(subscribe_arg.topic_name, topic_name);
	strcpy(subscribe_arg.handler, handler);
	strcpy(subscribe_arg.handler_namespace, local_namespace);
	unsigned long action_seqno = WooFPut(DHT_SUBSCRIBE_WOOF, NULL, &subscribe_arg);
	if (WooFInvalid(action_seqno)) {
		fprintf(stderr, "failed to store subscribe arg\n");
		return -1;
	}

	char hashed_key[SHA_DIGEST_LENGTH];
	SHA1(topic_name, strlen(topic_name), hashed_key);
	DHT_FIND_SUCCESSOR_ARG arg;
	dht_init_find_arg(&arg, topic_name, hashed_key, ""); // namespace doesn't matter, acted in target namespace
	arg.action = DHT_ACTION_SUBSCRIBE;
	strcpy(arg.action_namespace, local_namespace);
	arg.action_seqno = action_seqno;

	char target_woof[DHT_NAME_LENGTH];
	sprintf(target_woof, "%s/%s", local_namespace, DHT_FIND_SUCCESSOR_WOOF);
	unsigned long seq = WooFPut(target_woof, "h_find_successor", &arg);
	if (WooFInvalid(seq)) {
		fprintf(stderr, "failed to invoke h_find_successor on %s\n", target_woof);
		exit(1);
	}

	return 0;
}

unsigned long dht_publish(char *topic_name, void *element) {
	char topic_namespace[DHT_NAME_LENGTH];
	if (dht_find_addr(topic_name, topic_namespace) < 0) {
		fprintf(stderr, "failed to find topic address\n");
		return -1;
	}
	return dht_remote_publish(topic_namespace, topic_name, element);
}

unsigned long dht_remote_publish(char *topic_namespace, char *topic_name, void *element) {
	char woof_name[DHT_NAME_LENGTH];
	sprintf(woof_name, "%s/%s", topic_namespace, topic_name);
	unsigned long element_size = WooFMsgGetElSize(woof_name);
	if (WooFInvalid(element_size)) {
		fprintf(stderr, "failed to get element size of %s", woof_name);
		return -1;
	}
	void *ptr = malloc(element_size);
	memcpy(ptr, topic_namespace, DHT_NAME_LENGTH);
	memcpy(ptr + DHT_NAME_LENGTH, element, element_size - DHT_NAME_LENGTH);
	unsigned long seq = WooFPut(woof_name, "h_forward", ptr);
	free(ptr);
	return seq;
}