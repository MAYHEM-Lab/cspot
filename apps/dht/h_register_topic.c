#define DEBUG

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <openssl/sha.h>

#include "woofc.h"
#include "dht.h"

int h_register_topic(WOOF *wf, unsigned long seq_no, void *ptr) {
	DHT_REGISTER_TOPIC_ARG *arg = (DHT_REGISTER_TOPIC_ARG *)ptr;

	log_set_tag("register_topic");
	log_set_level(DHT_LOG_DEBUG);
	// log_set_level(LOG_INFO);
	log_set_output(stdout);

	char woof_name[DHT_NAME_LENGTH];
	sprintf(woof_name, "%s_%s", arg->topic_name, DHT_TOPIC_REGISTRATION_WOOF);
	if (!WooFExist(woof_name)) {
		if (WooFCreate(woof_name, sizeof(DHT_TOPIC_ENTRY), DHT_HISTORY_LENGTH_SHORT) < 0) {
			log_error("failed to create woof %s", woof_name);
			exit(1);
		}
		log_info("created registration woof %s", woof_name);
	}

	DHT_TOPIC_ENTRY topic_entry;
	strcpy(topic_entry.topic_name, arg->topic_name);
	strcpy(topic_entry.topic_namespace, arg->topic_namespace);
	unsigned long seq = WooFPut(woof_name, NULL, &topic_entry);
	if (WooFInvalid(seq)) {
		log_error("failed to register topic");
		exit(1);
	}
	log_info("registered topic %s/%s", topic_entry.topic_namespace, topic_entry.topic_name);

	char subscription_woof[DHT_NAME_LENGTH];
	sprintf(subscription_woof, "%s_%s", arg->topic_name, DHT_SUBSCRIPTION_LIST_WOOF);
	if (WooFCreate(subscription_woof, sizeof(DHT_SUBSCRIPTION_LIST), DHT_HISTORY_LENGTH_SHORT) < 0) {
		log_error("failed to create %s", subscription_woof);
		exit(1);
	}
	log_info("created subscription woof %s", subscription_woof);
	DHT_SUBSCRIPTION_LIST list;
	list.size = 0;
	memset(list.handlers, 0, sizeof(list.handlers));
	memset(list.namespace, 0, sizeof(list.namespace));
	seq = WooFPut(subscription_woof, NULL, &list);
	if (WooFInvalid(seq)) {
		log_error("failed to initialize subscription list %s", subscription_woof);
		exit(1);
	}

	return 1;
}
