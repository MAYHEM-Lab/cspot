#define DEBUG

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <openssl/sha.h>

#include "woofc.h"
#include "dht.h"

int subscribe(WOOF *wf, unsigned long seq_no, void *ptr)
{
	log_set_level(LOG_DEBUG);
	// log_set_level(LOG_INFO);
	log_set_output(stdout);

	SUBSCRIPTION_ARG *arg = (SUBSCRIPTION_ARG *)ptr;
	SUBSCRIPTION_LIST list;
	int err;
	unsigned long seq;
	int i;
	char msg[256];
	char handler[64];
	char subscription_woof[512];

	sprintf(subscription_woof, "%s_%s", DHT_SUBSCRIPTION_LIST_WOOF, arg->topic);
	seq = WooFGetLatestSeqno(subscription_woof);
	if (WooFInvalid(seq))
	{
		sprintf(msg, "couldn't get latest seq_no of subscription list of %s", arg->topic);
		log_error("subscribe", msg);
		exit(1);
	}
	err = WooFGet(subscription_woof, &list, seq);
	if (err < 0)
	{
		sprintf(msg, "couldn't get latest subscription list of %s with seq_no %lu", arg->topic, seq);
		log_error("subscribe", msg);
		exit(1);
	}

	memcpy(list.handlers + 64 * list.size, arg->handler, 64);
	list.size++;
	sprintf(msg, "number of subscription: %d", list.size);
	log_debug("subscribe", msg);
	sprintf(msg, "handlers: ");
	for (i = 0; i < list.size; i++)
	{
		memcpy(handler, list.handlers + 64 * i, 64);
		sprintf(msg + strlen(msg), "%s ", handler);
	}
	log_debug("subscribe", msg);

	seq = WooFPut(subscription_woof, NULL, &list);
	if (WooFInvalid(seq))
	{
		sprintf(msg, "couldn't update subscription list %s", arg->topic);
		log_error("subscribe", msg);
		exit(1);
	}

	return 1;
}
