#define DEBUG

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <openssl/sha.h>

#include "woofc.h"
#include "dht.h"

int h_forward(WOOF *wf, unsigned long seq_no, void *ptr) {
	log_set_tag("forward");
	log_set_level(DHT_LOG_DEBUG);
	// log_set_level(LOG_INFO);
	log_set_output(stdout);

	char local_namespace[DHT_NAME_LENGTH];
	if (node_woof_name(local_namespace) < 0) {
		log_error("failed to get local namespace");
		exit(1);
	}

	char hashed_key[SHA_DIGEST_LENGTH];
	SHA1(wf->shared->filename, strlen(wf->shared->filename), hashed_key);
	DHT_FIND_SUCCESSOR_ARG arg;
	dht_init_find_arg(&arg, wf->shared->filename, hashed_key, "");
	arg.action = DHT_ACTION_TRIGGER;
	strcpy(arg.action_namespace, local_namespace);
	arg.action_seqno = seq_no;
	
	char target_namespace[DHT_NAME_LENGTH];
	memcpy(target_namespace, ptr, DHT_NAME_LENGTH);
	char woof_name[DHT_NAME_LENGTH];
	sprintf(woof_name, "%s/%s", target_namespace, DHT_FIND_SUCCESSOR_WOOF);
	unsigned long seq = WooFPut(woof_name, "h_find_successor", &arg);
	if (WooFInvalid(seq)) {
		fprintf(stderr, "failed to invoke h_find_successor on %s\n", woof_name);
		exit(1);
	}
	return 1;
}
