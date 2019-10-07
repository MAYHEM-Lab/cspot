#define DEBUG

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <openssl/sha.h>

#include "woofc.h"
#include "dht.h"

int trigger(WOOF *wf, unsigned long seq_no, void *ptr)
{
	log_set_level(LOG_DEBUG);
	// log_set_level(LOG_INFO);
	log_set_output(stdout);

	int err;
	SUBSCRIPTION_LIST list;
	TRIGGER_ARG trigger_arg;
	unsigned long seq;
	int i;
	char msg[256];
	char handler[64];
	char subscription_woof[512];

	sprintf(subscription_woof, "%s_%s", DHT_SUBSCRIPTION_LIST_WOOF, wf->shared->filename);
	seq = WooFGetLatestSeqno(subscription_woof);
	if (WooFInvalid(seq))
	{
		sprintf(msg, "couldn't get latest seq_no of subscription list of %s", wf->shared->filename);
		log_error("trigger", msg);
		exit(1);
	}
	err = WooFGet(subscription_woof, &list, seq);
	if (err < 0)
	{
		sprintf(msg, "couldn't get latest subscription list of %s with seq_no %lu", wf->shared->filename, seq);
		log_error("trigger", msg);
		exit(1);
	}

	sprintf(msg, "number of subscription: %d", list.size);
	log_debug("trigger", msg);
	for (i = 0; i < list.size; i++)
	{
		memcpy(handler, list.handlers + 64 * i, 64);
		sprintf(msg, "handler: %s, woof: %s[%lu]", handler, wf->shared->filename, seq_no);
		log_debug("trigger", msg);

		trigger_arg.seqno = seq_no;
		sprintf(trigger_arg.woof_name, "%s", wf->shared->filename);
		
		seq = WooFPut(DHT_TRIGGER_ARG_WOOF, handler, &trigger_arg);
		if (WooFInvalid(seq))
		{
			sprintf(msg, "couldn't trigger handler %s on woof %s", handler, wf->shared->filename);
			log_error("trigger", msg);
		}
		else
		{
			sprintf(msg, "triggered handler %s on woof %s", handler, wf->shared->filename);
			log_debug("trigger", msg);
		}
	}

	return 1;
}
