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

char node_woof[256];

char *woof_to_create[] = {DHT_TABLE_WOOF, DHT_FIND_SUCCESSOR_ARG_WOOF, DHT_FIND_SUCCESSOR_RESULT_WOOF,
	DHT_GET_PREDECESSOR_ARG_WOOF, DHT_GET_PREDECESSOR_RESULT_WOOF, DHT_NOTIFY_ARG_WOOF};
unsigned long woof_element_size[] = {sizeof(DHT_TABLE_EL), sizeof(FIND_SUCESSOR_ARG), sizeof(FIND_SUCESSOR_RESULT),
	sizeof(GET_PREDECESSOR_ARG), sizeof(GET_PREDECESSOR_RESULT), sizeof(NOTIFY_ARG)};

int main(int argc, char **argv)
{
	// log_set_level(LOG_DEBUG);
	log_set_level(LOG_INFO);
	log_set_output(stdout);

	int err;
	DHT_TABLE_EL el;
	char woof_name[2048];
	char node_woof[2048];
	unsigned long seq_no;
	INIT_TOPIC_ARG init_topic_arg;
	SUBSCRIPTION_ARG sub_arg;
	TEST_EL test_el;
	char handler[64];
	char msg[256];

	err = node_woof_name(woof_name);
	if (err < 0)
	{
		printf("couldn't get local woof name\n");
		fflush(stdout);
		return 0;
	}

	init_topic_arg.element_size = sizeof(TEST_EL);
	init_topic_arg.history_size = 10;
	sprintf(init_topic_arg.topic, "test_topic");
	sprintf(node_woof, "%s/%s", woof_name, DHT_INIT_TOPIC_ARG_WOOF);
	seq_no = WooFPut(node_woof, "init_topic", &init_topic_arg);
	if (WooFInvalid(seq_no))
	{
		printf("couldn't call init_topic on woof %s\n", node_woof);
		fflush(stdout);
		return 0;
	}
	printf("init_topic done on woof %s\n", node_woof);
	fflush(stdout);
	
	sprintf(sub_arg.topic, "test_topic");
	sprintf(sub_arg.handler, "test2");
	sprintf(node_woof, "%s/%s", woof_name, DHT_SUBSCRIPTION_ARG_WOOF);
	seq_no = WooFPut(node_woof, "subscribe", &sub_arg);
	if (WooFInvalid(seq_no))
	{
		printf("couldn't call subscribe on woof %s\n", node_woof);
		fflush(stdout);
		return 0;
	}
	printf("subscribe done on woof %s\n", node_woof);
	fflush(stdout);

	sprintf(test_el.msg, "test message");
	sprintf(node_woof, "%s/%s", woof_name, "test_topic");
	seq_no = WooFPut(node_woof, "trigger", &test_el);
	if (WooFInvalid(seq_no))
	{
		printf("couldn't call trigger on woof %s\n", node_woof);
		fflush(stdout);
		return 0;
	}
	printf("put to topic %s\n", "test_topic");
	fflush(stdout);

	return (0);
}
