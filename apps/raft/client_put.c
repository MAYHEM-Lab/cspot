#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "raft.h"
#include "client.h"

#define ARGS "f:d:sr"
char *Usage = "client_put -f config -d data\n-s for synchronously put, -r to redirect if leader is down\n";

int get_result_delay = 20;

int main(int argc, char **argv) {
	char config[256];
	RAFT_DATA_TYPE data;
	bool sync = false;
	bool redirect = false;

	int c;
	while ((c = getopt(argc, argv, ARGS)) != EOF) {
		switch (c) {
			case 'f': {
				strncpy(config, optarg, sizeof(config));
				break;
			}
			case 'd': {
				strncpy(data.val, optarg, sizeof(data.val));
				break;
			}
			case 's': {
				sync = true;
				break;
			}
			case 'r': {
				sync = true;
				redirect = true;
				break;
			}
			default: {
				fprintf(stderr, "Unrecognized command %c\n", (char)c);
				fprintf(stderr, "%s", Usage);
				exit(1);
			}
		}
	}

	FILE *fp = fopen(config, "r");
	if (fp == NULL) {
		fprintf(stderr, "couldn't open config file");
		fflush(stderr);
		exit(1);
	}
	raft_init_client(fp);
	fclose(fp);

	unsigned long index, term;
	int err = raft_put(&data, &index, &term, sync);
	while (err == RAFT_REDIRECTED) {
		err = raft_put(&data, &index, &term, sync);
	}
	
	return err;
}

