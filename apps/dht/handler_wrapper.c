#define DEBUG

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <openssl/sha.h>

#include "woofc.h"
#include "dht.h"

int handler_wrapper(WOOF *wf, unsigned long seq_no, void *ptr) {
	DHT_INVOCATION_ARG *arg = (DHT_INVOCATION_ARG *)ptr;

	log_set_tag("PUT_HANDLER_NAME");
	// log_set_level(LOG_DEBUG);
	log_set_level(DHT_LOG_INFO);
	log_set_output(stdout);

	char *woof_name = arg->woof_name;
	unsigned long seqno = arg->seqno;
	WOOF *woof = WooFOpen(woof_name);
	if (woof == NULL) {
		log_error("couldn't open WooF %s", woof_name);
		exit(1);
	}
	void *element = malloc(woof->shared->element_size);
	if (element == NULL) {
		log_error("couldn't malloc element with size %lu", woof->shared->element_size);
		WooFDrop(woof);
		exit(1);
	}
	if (WooFRead(woof, element, seqno) < 0) {
		log_error("couldn't read element from WooF %s at %lu", woof_name, seqno);
		free(element);
		WooFDrop(woof);
		exit(1);
	}
	int err = PUT_HANDLER_NAME(woof, seqno, element + DHT_NAME_LENGTH);
	free(element);
	WooFDrop(woof);
	return err;
}
