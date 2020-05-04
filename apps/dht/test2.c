#define DEBUG

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <openssl/sha.h>

#include "woofc.h"
#include "dht.h"

int handler_function(WOOF *wf, unsigned long seq_no, void *ptr) {
	TEST_EL *el = (TEST_EL *)ptr;
	printf("from woof %s at %lu with string: %s\n", wf->shared->filename, seq_no, el->msg);
	return 1;
}

int test2(WOOF *wf, unsigned long seq_no, void *ptr) {
	TRIGGER_ARG *arg = (TRIGGER_ARG *)ptr;

	log_set_tag("HANDLER_WRAPPER");
	// log_set_level(LOG_DEBUG);
	log_set_level(LOG_INFO);
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
		exit(1);
	}
	if (WooFRead(woof, element, seqno) < 0) {
		log_error("couldn't read element from WooF %s at %lu", woof_name, seqno);
		free(element);
		exit(1);
	}
	int err = handler_function(woof, seqno, element);
	free(element);
	return err;
}
