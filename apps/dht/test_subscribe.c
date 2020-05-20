// #define DEBUG

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <openssl/sha.h>

#include "woofc.h"
#include "woofc-host.h"
#include "dht.h"
#include "dht_client.h"
#include "dht_utils.h"

#define TEST_TOPIC "test"
#define TEST_HANDLER "test_handler"
#define TEST_MESSAGE "test message"
#define TEST_COUNT 10

typedef struct test_stc {
	char msg[256];
	unsigned long sent;
} TEST_EL;

int main(int argc, char **argv) {
	WooFInit();

	char local_namespace[DHT_NAME_LENGTH];
	if (node_woof_name(local_namespace) < 0) {
		fprintf(stderr, "failed to get the local woof name: %s\n", dht_error_msg);
		exit(1);
	}

	if (dht_create_topic(TEST_TOPIC, sizeof(TEST_EL), DHT_HISTORY_LENGTH_SHORT) < 0) {
		fprintf(stderr, "failed to create topic: %s\n", dht_error_msg);
		exit(1);
	}
	printf("topic created\n");
	usleep(500 * 1000);

	if (dht_register_topic(TEST_TOPIC) < 0) {
		fprintf(stderr, "failed to register topic: %s\n", dht_error_msg);
		exit(1);
	}
	printf("topic registered\n");
	usleep(500 * 1000);

	if (dht_subscribe(TEST_TOPIC, TEST_HANDLER) < 0) {
		fprintf(stderr, "failed to subscribe to topic: %s\n", dht_error_msg);
		exit(1);
	}
	printf("handler subscribed to topic\n");
	usleep(500 * 1000);

	int i;
	for (i = 0; i < TEST_COUNT; ++i) {
		TEST_EL el = {0};
		sprintf(el.msg, "%s", TEST_MESSAGE);
		el.sent = get_milliseconds();
		unsigned long seq = dht_publish(TEST_TOPIC, &el);
		if (WooFInvalid(seq)) {
			fprintf(stderr, "failed to publish to topic: %s\n", dht_error_msg);
			exit(1);
		}
		printf("%s published to topic at %lu\n", el.msg, el.sent);
	}
	printf("test done\n");
	return 0;
}
