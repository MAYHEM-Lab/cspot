#define DEBUG

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <openssl/sha.h>
#include "woofc.h"
#include "woofc-access.h"
#include "dht.h"

int h_trigger(WOOF *wf, unsigned long seq_no, void *ptr) {
	DHT_TRIGGER_ARG *arg = (DHT_TRIGGER_ARG *)ptr;

	log_set_tag("trigger");
	log_set_level(DHT_LOG_DEBUG);
	// log_set_level(LOG_INFO);
	log_set_output(stdout);

	char local_namespace[DHT_NAME_LENGTH];
	if (node_woof_name(local_namespace) < 0) {
		log_error("failed to get local namespace");
		exit(1);
	}

	char topic_name[DHT_NAME_LENGTH];
	if (WooFNameFromURI(arg->woof_name, topic_name, DHT_NAME_LENGTH) < 0) {
		log_error("failed to get topic name");
		exit(1);
	}
	char subscription_woof[DHT_NAME_LENGTH];
	sprintf(subscription_woof, "%s_%s", topic_name, DHT_SUBSCRIPTION_LIST_WOOF);
	unsigned long seq = WooFGetLatestSeqno(subscription_woof);
	if (WooFInvalid(seq)) {
		log_error("failed to get the latest seq_no from %s", subscription_woof);
		exit(1);
	}
	DHT_SUBSCRIPTION_LIST list;
	if (WooFGet(subscription_woof, &list, seq) < 0) {
		log_error("failed to get the latest subscription list of %s", topic_name);
		exit(1);
	}

	log_debug("number of subscription: %d", list.size);
	int i;
	for (i = 0; i < list.size; ++i) {
		log_debug("namespace: %s, handler: %s", list.namespace[i], list.handlers[i]);

		DHT_INVOCATION_ARG invocation_arg;
		invocation_arg.seqno = arg->seqno;
		strcpy(invocation_arg.woof_name, arg->woof_name);
		
		char invocation_woof[DHT_NAME_LENGTH];
		sprintf(invocation_woof, "%s/%s", list.namespace[i], DHT_INVOCATION_WOOF);
		seq = WooFPut(invocation_woof, list.handlers[i], &invocation_arg);
		if (WooFInvalid(seq)) {
			log_error("couldn't trigger handler %s in %s", list.handlers[i], list.namespace[i]);
		} else {
			log_debug("triggered handler %s in %s", list.handlers[i], list.namespace[i]);
		}
	}

	return 1;
}
