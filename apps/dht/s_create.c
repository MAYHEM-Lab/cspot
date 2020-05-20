#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <openssl/sha.h>
#include "woofc.h"
#include "woofc-host.h"
#include "dht.h"

int main(int argc, char **argv) {
	WooFInit();

	char woof_name[DHT_NAME_LENGTH];
	if (node_woof_name(woof_name) < 0) {
		fprintf(stderr, "failed to get local node's woof name: %s\n", dht_error_msg);
		return -1;
	}

	if (dht_create_cluster(woof_name) < 0) {
		fprintf(stderr, "failed to initialize the cluster: %s\n", dht_error_msg);
		exit(1);
	}

	return 0;
}
