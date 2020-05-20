#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <openssl/sha.h>
#include "woofc.h"
#include "woofc-access.h"
#include "dht.h"
#include "dht_utils.h"

int handler_wrapper(WOOF *wf, unsigned long seq_no, void *ptr) {
	DHT_INVOCATION_ARG *arg = (DHT_INVOCATION_ARG *)ptr;

	log_set_tag("PUT_HANDLER_NAME");
	log_set_level(DHT_LOG_INFO);
	log_set_output(stdout);

	unsigned long element_size = WooFMsgGetElSize(arg->woof_name);
	if (WooFInvalid(element_size)) {
		log_error("failed to get element size of the topic");
		exit(1);
	}
	void *element = malloc(element_size);
	if (element == NULL) {
		log_error("failed to malloc element with size %lu", element_size);
		exit(1);
	}
	if (WooFGet(arg->woof_name, element, arg->seq_no) < 0) {
		log_error("failed to read element from WooF %s at %lu", arg->woof_name, arg->seq_no);
		free(element);
		exit(1);
	}
	char topic_name[DHT_NAME_LENGTH];
	if (WooFNameFromURI(arg->woof_name, topic_name, DHT_NAME_LENGTH) < 0) {
		log_error("failed to get topic name from uri");
		free(element);
		exit(1);
	}
	int err = PUT_HANDLER_NAME(arg->woof_name, arg->seq_no, element);
	free(element);

	return err;
}
