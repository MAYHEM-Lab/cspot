#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <openssl/sha.h>
#include "woofc.h"
#include "dht.h"
#include "dht_utils.h"

int h_trigger(WOOF *wf, unsigned long seq_no, void *ptr) {
	DHT_TRIGGER_ARG *arg = (DHT_TRIGGER_ARG *)ptr;

	log_set_tag("trigger");
	log_set_level(DHT_LOG_DEBUG);
	// log_set_level(DHT_LOG_INFO);
	log_set_output(stdout);

#ifdef USE_RAFT
	log_debug("topic: %s, raft: %s, index: %lu", arg->topic_name, arg->element_woof, arg->element_seqno);
#else
	log_debug("topic: %s, woof: %s, seqno: %lu", arg->topic_name, arg->element_woof, arg->element_seqno);
#endif

	char local_namespace[DHT_NAME_LENGTH];
	if (node_woof_name(local_namespace) < 0) {
		log_error("failed to get local namespace");
		exit(1);
	}

	unsigned long latest_subscription = WooFGetLatestSeqno(arg->subscription_woof);
	if (WooFInvalid(latest_subscription) || latest_subscription == 0) {
		log_error("failed to get subscription list");
		exit(1);
	}
	DHT_SUBSCRIPTION_LIST list = {0};
	if (WooFGet(arg->subscription_woof, &list, latest_subscription) < 0) {
		log_error("failed to get the latest subscription list of %s", arg->topic_name);
		exit(1);
	}

	log_debug("number of subscription: %d", list.size);
	int i;
	for (i = 0; i < list.size; ++i) {
		log_debug("%s/%s", list.namespace[i], list.handlers[i]);

		DHT_INVOCATION_ARG invocation_arg = {0};
		strcpy(invocation_arg.woof_name, arg->element_woof);
		invocation_arg.seq_no = arg->element_seqno;
		
		char invocation_woof[DHT_NAME_LENGTH];
		sprintf(invocation_woof, "%s/%s", list.namespace[i], DHT_INVOCATION_WOOF);
		unsigned long seq = WooFPut(invocation_woof, list.handlers[i], &invocation_arg);
		if (WooFInvalid(seq)) {
			log_error("failed to trigger handler %s in %s", list.handlers[i], list.namespace[i]);
		} else {
			log_debug("triggered handler %s in %s", list.handlers[i], list.namespace[i]);
		}
	}

	return 1;
}
