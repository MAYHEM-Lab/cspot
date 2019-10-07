#define DEBUG

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <openssl/sha.h>

#include "woofc.h"
#include "dht.h"

int init_topic(WOOF *wf, unsigned long seq_no, void *ptr)
{
	log_set_level(LOG_DEBUG);
	// log_set_level(LOG_INFO);
	log_set_output(stdout);

	INIT_TOPIC_ARG *arg = (INIT_TOPIC_ARG *)ptr;
	SUBSCRIPTION_LIST list;
	int err;
	unsigned long seq;
	int i;
	char msg[256];
	char subscription_woof[512];

	err = WooFCreate(arg->topic, arg->element_size, arg->history_size);
	if (err < 0)
	{
		sprintf(msg, "couldn't create woof %s", arg->topic);
		log_error("init_topic", msg);
		exit(1);
	}
	sprintf(msg, "created woof %s with element_size %lu history_size %lu", arg->topic, arg->element_size, arg->history_size);
	log_info("init_topic", msg);

	sprintf(subscription_woof, "%s_%s", DHT_SUBSCRIPTION_LIST_WOOF, arg->topic);
	err = WooFCreate(subscription_woof, sizeof(SUBSCRIPTION_LIST), 10);
	if (err < 0)
	{
		sprintf(msg, "couldn't create woof %s", subscription_woof);
		log_error("init_topic", msg);
		exit(1);
	}
	sprintf(msg, "created woof %s", subscription_woof);
	log_info("init_topic", msg);

	list.size = 0;
	memset(list.handlers, 0, sizeof(list.handlers));
	seq = WooFPut(subscription_woof, NULL, &list);
	if (WooFInvalid(seq))
	{
		sprintf(msg, "couldn't initialize subscription list %s", subscription_woof);
		log_error("subscribe", msg);
		exit(1);
	}
	sprintf(msg, "initialized subscription list on woof %s", subscription_woof);
	log_info("init_topic", msg);

	return 1;
}
