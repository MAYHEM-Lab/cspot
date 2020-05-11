#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "raft.h"
#include "raft_client.h"

#define ARGS "f:t:"
char *Usage = "client_observe -f config -t timeout\n";

int get_result_delay = 20;

int main(int argc, char **argv) {
	char config[256];
	int timeout = 0;

	int c;
	while ((c = getopt(argc, argv, ARGS)) != EOF) {
		switch (c) {
			case 'f': {
				strncpy(config, optarg, sizeof(config));
				break;
			}
			case 't': {
				timeout = (int)strtoul(optarg, NULL, 0);
				break;
			}
			default: {
				fprintf(stderr, "Unrecognized command %c\n", (char)c);
				fprintf(stderr, "%s", Usage);
				exit(1);
			}
		}
	}

	if (config[0] == 0) {
		fprintf(stderr, "%s", Usage);
		exit(1);
	}

	FILE *fp = fopen(config, "r");
	if (fp == NULL) {
		fprintf(stderr, "failed to open config file %s\n", config);
		exit(1);
	}
	if (raft_init_client(fp) < 0) {
		fprintf(stderr, "can't init client\n");
		fclose(fp);	
		exit(1);
	}
	fclose(fp);

	fp = fopen(config, "r");
	if (fp == NULL) {
		fprintf(stderr, "failed to open new config file %s\n", config);
		exit(1);
	}
	fclose(fp);

	int err = raft_observe(timeout);
	while (err == RAFT_REDIRECTED) {
		err = raft_observe(timeout);
	}
	
	return err;
}

