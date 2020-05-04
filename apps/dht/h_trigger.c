#define DEBUG

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <openssl/sha.h>

#include "woofc.h"
#include "dht.h"

int h_trigger(WOOF *wf, unsigned long seq_no, void *ptr) {
	log_set_tag("trigger");
	log_set_level(LOG_DEBUG);
	// log_set_level(LOG_INFO);
	log_set_output(stdout);

	char subscription_woof[DHT_NAME_LENGTH];
	sprintf(subscription_woof, "%s_%s", DHT_SUBSCRIPTION_LIST_WOOF, wf->shared->filename);
	unsigned long seq = WooFGetLatestSeqno(subscription_woof);
	if (WooFInvalid(seq)) {
		log_error("couldn't get latest seq_no of subscription list of %s", wf->shared->filename);
		exit(1);
	}
	SUBSCRIPTION_LIST list;
	if (WooFGet(subscription_woof, &list, seq) < 0) {
		log_error("couldn't get latest subscription list of %s with seq_no %lu", wf->shared->filename, seq);
		exit(1);
	}

	log_debug("number of subscription: %d", list.size);
	int i;
	for (i = 0; i < list.size; ++i) {
		log_debug("handler: %s, woof: %s[%lu]", list.handlers[i], wf->shared->filename, seq_no);

		TRIGGER_ARG trigger_arg;
		trigger_arg.seqno = seq_no;
		sprintf(trigger_arg.woof_name, "%s", wf->shared->filename);
		
		seq = WooFPut(DHT_TRIGGER_ARG_WOOF, list.handlers[i], &trigger_arg);
		if (WooFInvalid(seq)) {
			log_error("couldn't trigger handler %s on woof %s", list.handlers[i], wf->shared->filename);
		} else {
			log_debug("triggered handler %s on woof %s", list.handlers[i], wf->shared->filename);
		}
	}

	return 1;
}
