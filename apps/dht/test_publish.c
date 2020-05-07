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
#include "client.h"

#define TEST_TOPIC "test"
#define TEST_HANDLER "test_handler"

typedef struct test_stc {
	char msg[256];
} TEST_EL;

int main(int argc, char **argv) {
	if (argc < 2) {
		fprintf(stderr, "Usage: ./test_publish data\n");
		exit(1);
	}
	WooFInit();

	char local_namespace[DHT_NAME_LENGTH];
	if (node_woof_name(local_namespace) < 0) {
		fprintf(stderr, "failed to get the local woof name\n");
		exit(1);
	}

	TEST_EL el;
	strcpy(el.msg, argv[1]);
	unsigned long seq = dht_publish(NULL, TEST_TOPIC, &el);
	if (WooFInvalid(seq)) {
		fprintf(stderr, "failed to publish to topic\n");
		exit(1);
	}
	printf("published to topic\n");
	return 0;
}
