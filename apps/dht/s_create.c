#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <openssl/sha.h>

#include "woofc.h"
#include "woofc-host.h"
#include "dht.h"

int main(int argc, char **argv) {
	log_set_tag("create");
	// log_set_level(LOG_DEBUG);
	log_set_level(DHT_LOG_INFO);
	// FILE *f = fopen("log_create","w");
	// log_set_output(f);
	log_set_output(stdout);
	WooFInit();

	if (create_cluster() < 0) {
		log_error("failed to initialize the cluster");
		exit(1);
	}

	return 0;
}
