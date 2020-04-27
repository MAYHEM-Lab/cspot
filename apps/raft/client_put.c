#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "raft.h"
#include "client.h"

#define ARGS "f:d:sr"
char *Usage = "client_put -f config -d data\n\
-s for synchronously put, -r to redirect if leader is down\n";

int get_result_delay = 20;

int main(int argc, char **argv) {
	char config[256];
	RAFT_DATA_TYPE data;
	memset(data.val, 0, sizeof(data.val));
	int sync = 0;
	int redirect = 0;

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
				sync = 1;
				break;
			}
			case 'r': {
				sync = 1;
				redirect = 1;
				break;
			}
			default: {
				fprintf(stderr, "Unrecognized command %c\n", (char)c);
				fprintf(stderr, "%s", Usage);
				exit(1);
			}
		}
	}

	if (config[0] == 0 || data.val[0] == 0) {
		fprintf(stderr, "%s", Usage);
		exit(1);
	}

	FILE *fp = fopen(config, "r");
	if (fp == NULL) {
		fprintf(stderr, "couldn't open config file");
		exit(1);
	}
	if (raft_init_client(fp) < 0) {
		fprintf(stderr, "can't init client\n");
		fclose(fp);	
		exit(1);
	}
	fclose(fp);
	
	unsigned long index, term, seqno;
	int err = raft_put(&data, &index, &term, &seqno, sync);
	while (err == RAFT_REDIRECTED) {
		err = raft_put(&data, &index, &term, &seqno, sync);
	}
	if (err >= 0) {
		printf("index: %lu, term: %lu, seqno: %lu\n", index, term, seqno);
	}
	
	return err;
}

