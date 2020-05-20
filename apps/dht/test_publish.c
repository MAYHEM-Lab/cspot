#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <openssl/sha.h>

#include "woofc.h"
#include "woofc-host.h"
#include "dht.h"
#include "dht_utils.h"
#include "dht_client.h"

#define TEST_TOPIC "test"
#define TEST_HANDLER "test_handler"

typedef struct test_stc {
	char msg[256];
	unsigned long sent;
} TEST_EL;

int main(int argc, char **argv) {
	if (argc < 2) {
		fprintf(stderr, "Usage: ./test_publish data\n");
		exit(1);
	}
	WooFInit();

	char local_namespace[DHT_NAME_LENGTH];
	if (node_woof_name(local_namespace) < 0) {
		fprintf(stderr, "failed to get the local woof name: %s\n", dht_error_msg);
		exit(1);
	}

	TEST_EL el = {0};
	strcpy(el.msg, argv[1]);
	el.sent = get_milliseconds();
	unsigned long seq = dht_publish(TEST_TOPIC, &el);
	if (WooFInvalid(seq)) {
		fprintf(stderr, "failed to publish to topic: %s\n", dht_error_msg);
		exit(1);
	}
	printf("%s published to topic at %lu\n", el.msg, el.sent);
	return 0;
}
