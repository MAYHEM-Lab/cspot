#define DEBUG

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <openssl/sha.h>

#include "woofc.h"
#include "dht.h"

int handler_function(WOOF *wf, unsigned long seq_no, void *ptr)
{
	TEST_EL *el = (TEST_EL *)ptr;
	printf("from woof %s at %lu with string: %s\n", wf->shared->filename, seq_no, el->msg);
	fflush(stdout);
	return 1;
}

int test2(WOOF *wf, unsigned long seq_no, void *ptr)
{
	// log_set_level(LOG_DEBUG);
	log_set_level(LOG_INFO);
	log_set_output(stdout);

	TRIGGER_ARG *arg = (TRIGGER_ARG *)ptr;
	char *woof_name;
	unsigned long seqno;
	WOOF *woof;
	void *element;
	int err;
	char msg[256];

	woof_name = arg->woof_name;
	seqno = arg->seqno;
	woof = WooFOpen(woof_name);
	if (woof == NULL)
	{
		sprintf(msg, "couldn't open WooF %s", woof_name);
		log_error("HANDLER_WRAPPER", msg);
		exit(1);
	}
	element = malloc(woof->shared->element_size);
	if (element == NULL)
	{
		sprintf(msg, "couldn't malloc element with size %lu", woof->shared->element_size);
		log_error("HANDLER_WRAPPER", msg);
		exit(1);
	}
	err = WooFRead(woof, element, seqno);
	if (err < 0)
	{
		sprintf(msg, "couldn't read element from WooF %s at %lu", woof_name, seqno);
		log_error("HANDLER_WRAPPER", msg);
		exit(1);
	}
	err = handler_function(woof, seqno, element);
	free(element);
	return err;
}
