#define DEBUG

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <openssl/sha.h>

#include "woofc.h"
#include "dht.h"

int h_init_topic(WOOF *wf, unsigned long seq_no, void *ptr) {
	INIT_TOPIC_ARG *arg = (INIT_TOPIC_ARG *)ptr;

	log_set_tag("init_topic");
	log_set_level(LOG_DEBUG);
	// log_set_level(LOG_INFO);
	log_set_output(stdout);
	
	if (WooFCreate(arg->topic, arg->element_size, arg->history_size) < 0) {
		log_error("couldn't create woof %s", arg->topic);
		exit(1);
	}
	log_info("created woof %s with element_size %lu history_size %lu", arg->topic, arg->element_size, arg->history_size);

	char subscription_woof[DHT_NAME_LENGTH];
	sprintf(subscription_woof, "%s_%s", DHT_SUBSCRIPTION_LIST_WOOF, arg->topic);
	if (WooFCreate(subscription_woof, sizeof(SUBSCRIPTION_LIST), 10) < 0) {
		log_error("couldn't create woof %s", subscription_woof);
		exit(1);
	}
	log_info("created woof %s", subscription_woof);

	SUBSCRIPTION_LIST list;
	list.size = 0;
	memset(list.handlers, 0, sizeof(list.handlers));
	unsigned long seq = WooFPut(subscription_woof, NULL, &list);
	if (WooFInvalid(seq)) {
		log_error("couldn't initialize subscription list %s", subscription_woof);
		exit(1);
	}
	log_info("initialized subscription list on woof %s", subscription_woof);

	return 1;
}
