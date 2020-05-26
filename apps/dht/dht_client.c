#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "woofc.h"
#include "woofc-access.h"
#include "dht.h"
#include "dht_client.h"
#ifdef USE_RAFT
#include "raft.h"
#include "raft_client.h"
#endif

int dht_find_node(char *topic_name, char result_node_replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH], int *result_node_leader) {
	char local_namespace[DHT_NAME_LENGTH];
	if (node_woof_name(local_namespace) < 0) {
		sprintf(dht_error_msg, "failed to get local woof namespace");
		return -1;
	}

	char hashed_key[SHA_DIGEST_LENGTH];
	SHA1(topic_name, strlen(topic_name), hashed_key);
	DHT_FIND_SUCCESSOR_ARG arg = {0};
	dht_init_find_arg(&arg, topic_name, hashed_key, local_namespace);
	arg.action = DHT_ACTION_FIND_NODE;

	char target_woof[DHT_NAME_LENGTH];
	char result_woof[DHT_NAME_LENGTH];
	sprintf(target_woof, "%s/%s", local_namespace, DHT_FIND_SUCCESSOR_WOOF);
	sprintf(result_woof, "%s/%s", local_namespace, DHT_FIND_NODE_RESULT_WOOF);
	unsigned long last_checked_result = WooFGetLatestSeqno(result_woof);

	unsigned long seq_no = WooFPut(target_woof, "h_find_successor", &arg);
	if (WooFInvalid(seq_no)) {
		sprintf(dht_error_msg, "failed to invoke h_find_successor on %s", target_woof);
		exit(1);
	}

	while (1) {
		unsigned long latest_result = WooFGetLatestSeqno(result_woof);
		unsigned long i;
		for (i = last_checked_result + 1; i <= latest_result; ++i) {
			DHT_FIND_NODE_RESULT result = {0};
			if (WooFGet(result_woof, &result, i) < 0) {
				sprintf(dht_error_msg, "failed to get result at %lu from %s", i, result_woof);
			}
			if (memcmp(result.topic, topic_name, SHA_DIGEST_LENGTH) == 0) {
				memcpy(result_node_replicas, result.node_replicas, sizeof(result.node_replicas));
				*result_node_leader = result.node_leader;
				return 0;
			}
		}
		last_checked_result = latest_result;
		usleep(10 * 1000);
	}
}

// call this on the node hosting the topic
int dht_create_topic(char *topic_name, unsigned long element_size, unsigned long history_size) {
#ifdef USE_RAFT
	if (element_size > RAFT_DATA_TYPE_SIZE) {
		sprintf(dht_error_msg, "element_size exceeds RAFT_DATA_TYPE_SIZE");
		return -1;
	}
#endif
	return WooFCreate(topic_name, element_size, history_size);
}

int dht_register_topic(char *topic_name) {
	char local_namespace[DHT_NAME_LENGTH];
	if (node_woof_name(local_namespace) < 0) {
		sprintf(dht_error_msg, "failed to get local woof namespace");
		return -1;
	}

	DHT_REGISTER_TOPIC_ARG register_topic_arg = {0};
	strcpy(register_topic_arg.topic_name, topic_name);
	strcpy(register_topic_arg.topic_namespace, local_namespace);
	unsigned long action_seqno = WooFPut(DHT_REGISTER_TOPIC_WOOF, NULL, &register_topic_arg);
	if (WooFInvalid(action_seqno)) {
		sprintf(dht_error_msg, "failed to store subscribe arg");
		return -1;
	}

	char hashed_key[SHA_DIGEST_LENGTH];
	SHA1(topic_name, strlen(topic_name), hashed_key);
	DHT_FIND_SUCCESSOR_ARG arg = {0};
	dht_init_find_arg(&arg, topic_name, hashed_key, ""); // namespace doesn't matter, acted in target namespace
	arg.action = DHT_ACTION_REGISTER_TOPIC;
	strcpy(arg.action_namespace, local_namespace);
	arg.action_seqno = action_seqno;

	char target_woof[DHT_NAME_LENGTH];
	sprintf(target_woof, "%s/%s", local_namespace, DHT_FIND_SUCCESSOR_WOOF);
	unsigned long seq_no = WooFPut(target_woof, "h_find_successor", &arg);
	if (WooFInvalid(seq_no)) {
		sprintf(dht_error_msg, "failed to invoke h_find_successor on %s", target_woof);
		exit(1);
	}
	return 0;
}

int dht_subscribe(char *topic_name, char *handler) {
	if (access(handler, F_OK) == -1) {
		sprintf(dht_error_msg, "handler %s doesn't exist", handler);
		return -1;
	}

	char local_namespace[DHT_NAME_LENGTH];
	if (node_woof_name(local_namespace) < 0) {
		sprintf(dht_error_msg, "failed to get local woof namespace");
		return -1;
	}

	DHT_SUBSCRIBE_ARG subscribe_arg = {0};
	strcpy(subscribe_arg.topic_name, topic_name);
	strcpy(subscribe_arg.handler, handler);
	strcpy(subscribe_arg.handler_namespace, local_namespace);
	unsigned long action_seqno = WooFPut(DHT_SUBSCRIBE_WOOF, NULL, &subscribe_arg);
	if (WooFInvalid(action_seqno)) {
		sprintf(dht_error_msg, "failed to store subscribe arg");
		return -1;
	}

	char hashed_key[SHA_DIGEST_LENGTH];
	SHA1(topic_name, strlen(topic_name), hashed_key);
	DHT_FIND_SUCCESSOR_ARG arg = {0};
	dht_init_find_arg(&arg, topic_name, hashed_key, ""); // namespace doesn't matter, acted in target namespace
	arg.action = DHT_ACTION_SUBSCRIBE;
	strcpy(arg.action_namespace, local_namespace);
	arg.action_seqno = action_seqno;

	char target_woof[DHT_NAME_LENGTH];
	sprintf(target_woof, "%s/%s", local_namespace, DHT_FIND_SUCCESSOR_WOOF);
	unsigned long seq = WooFPut(target_woof, "h_find_successor", &arg);
	if (WooFInvalid(seq)) {
		sprintf(dht_error_msg, "failed to invoke h_find_successor on %s", target_woof);
		exit(1);
	}

	return 0;
}

unsigned long dht_publish(char *topic_name, void *element) {
	char node_replicas[DHT_REPLICA_NUMBER][DHT_NAME_LENGTH];
	int node_leader;
	if (dht_find_node(topic_name, node_replicas, &node_leader) < 0) {
		sprintf(dht_error_msg, "failed to find node hosting the topic");
		return -1;
	}
	char *node_addr = node_replicas[node_leader];
	
	// TODO: if failed use the next replica
	// get the topic namespace
	char registration_woof[DHT_NAME_LENGTH];
	sprintf(registration_woof, "%s/%s_%s", node_addr, topic_name, DHT_TOPIC_REGISTRATION_WOOF);

	DHT_TOPIC_REGISTRY topic_entry = {0};
	if (get_latest_element(registration_woof, &topic_entry) < 0) {
		sprintf(dht_error_msg, "failed to get topic registration info from %s: %s", registration_woof, dht_error_msg);
		return -1;
	}
	
	DHT_TRIGGER_ARG trigger_arg = {0};
	strcpy(trigger_arg.topic_name, topic_name);
	sprintf(trigger_arg.subscription_woof, "%s/%s_%s", node_addr, topic_name, DHT_SUBSCRIPTION_LIST_WOOF);
#ifdef USE_RAFT
	printf("using raft, leader: %s\n", topic_entry.topic_namespace);
	raft_set_client_leader(topic_entry.topic_namespace);
	RAFT_DATA_TYPE raft_data = {0};
	memcpy(raft_data.val, element, sizeof(raft_data.val));
	sprintf(trigger_arg.element_woof, "%s", topic_entry.topic_namespace);
	unsigned long index = raft_sync_put(&raft_data, 0);
	while (index == RAFT_REDIRECTED) {
		printf("redirected to %s\n", raft_client_leader);
		index = raft_sync_put(&raft_data, 0);
	}
	if (raft_is_error(index)) {
		sprintf(dht_error_msg, "failed to put to raft: %s", raft_error_msg);
		return -1;
	}
	trigger_arg.element_seqno = index;
#else
	sprintf(trigger_arg.element_woof, "%s/%s", topic_entry.topic_namespace, topic_name);
	printf("using woof: %s\n", trigger_arg.element_woof);
	trigger_arg.element_seqno = WooFPut(trigger_arg.element_woof, NULL, element);
	if (WooFInvalid(trigger_arg.element_seqno)) {
		sprintf(dht_error_msg, "failed to put element to woof");
		return -1;
	}
#endif
	char trigger_woof[DHT_NAME_LENGTH];
	sprintf(trigger_woof, "%s/%s", topic_entry.topic_namespace, DHT_TRIGGER_WOOF);
	return WooFPut(trigger_woof, "h_trigger", &trigger_arg);
}