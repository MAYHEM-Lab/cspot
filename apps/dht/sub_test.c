// #define DEBUG

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <openssl/sha.h>

#include "woofc.h"
#include "woofc-host.h"
#include "dht.h"

#define ARGS "t:h:"
char *Usage = "sub -t topic -h handler\n";

int main(int argc, char **argv) {
	char woof_name[DHT_NAME_LENGTH];
	if (node_woof_name(woof_name) < 0) {
		printf("couldn't get local woof name\n");
		return 0;
	}

	INIT_TOPIC_ARG init_topic_arg;
	init_topic_arg.element_size = sizeof(TEST_EL);
	init_topic_arg.history_size = 10;
	sprintf(init_topic_arg.topic, "test_topic");
	char node_woof[DHT_NAME_LENGTH];
	sprintf(node_woof, "%s/%s", woof_name, DHT_INIT_TOPIC_ARG_WOOF);
	unsigned long seq_no = WooFPut(node_woof, "init_topic", &init_topic_arg);
	if (WooFInvalid(seq_no)) {
		printf("couldn't call init_topic on woof %s\n", node_woof);
		return 0;
	}
	printf("init_topic done on woof %s\n", node_woof);
	
	SUBSCRIPTION_ARG sub_arg;
	sprintf(sub_arg.topic, "test_topic");
	sprintf(sub_arg.handler, "test2");
	sprintf(node_woof, "%s/%s", woof_name, DHT_SUBSCRIPTION_ARG_WOOF);
	seq_no = WooFPut(node_woof, "subscribe", &sub_arg);
	if (WooFInvalid(seq_no)) {
		printf("couldn't call subscribe on woof %s\n", node_woof);
		return 0;
	}
	printf("subscribe done on woof %s\n", node_woof);
	
	TEST_EL test_el;
	sprintf(test_el.msg, "test message");
	sprintf(node_woof, "%s/%s", woof_name, "test_topic");
	seq_no = WooFPut(node_woof, "trigger", &test_el);
	if (WooFInvalid(seq_no)) {
		printf("couldn't call trigger on woof %s\n", node_woof);
		fflush(stdout);
		return 0;
	}
	printf("put to topic %s\n", "test_topic");
	
	return 0;
}
