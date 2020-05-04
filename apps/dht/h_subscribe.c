#define DEBUG

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <openssl/sha.h>

#include "woofc.h"
#include "dht.h"

int h_subscribe(WOOF *wf, unsigned long seq_no, void *ptr) {
	SUBSCRIPTION_ARG *arg = (SUBSCRIPTION_ARG *)ptr;

	log_set_tag("subscribe");
	log_set_level(LOG_DEBUG);
	// log_set_level(LOG_INFO);
	log_set_output(stdout);

	char subscription_woof[DHT_NAME_LENGTH];
	sprintf(subscription_woof, "%s_%s", DHT_SUBSCRIPTION_LIST_WOOF, arg->topic);
	unsigned long seq = WooFGetLatestSeqno(subscription_woof);
	if (WooFInvalid(seq)) {
		log_error("couldn't get latest seq_no of subscription list of %s", arg->topic);
		exit(1);
	}
	SUBSCRIPTION_LIST list;
	if (WooFGet(subscription_woof, &list, seq) < 0) {
		log_error("couldn't get latest subscription list of %s with seq_no %lu", arg->topic, seq);
		exit(1);
	}

	memcpy(list.handlers[list.size], arg->handler, sizeof(list.handlers[list.size]));
	list.size += 1;
	log_debug("number of subscription: %d", list.size);
	char msg[2048];
	sprintf(msg, "handlers:");
	int i;
	for (i = 0; i < list.size; ++i) {
		sprintf(msg + strlen(msg), " %s", list.handlers[i]);
	}
	log_debug(msg);

	seq = WooFPut(subscription_woof, NULL, &list);
	if (WooFInvalid(seq)) {
		log_error("couldn't update subscription list %s", arg->topic);
		exit(1);
	}

	return 1;
}
