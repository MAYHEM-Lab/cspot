#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "raft.h"
#include "client.h"

#define ARGS "f:c:s"
char *Usage = "client_test -f config_file -c count\n\
-s for synchronously put\n";

int main(int argc, char **argv) {
	char config_file[256] = "";
	int count = 10;
	int sync = 0;

	int c;
	while ((c = getopt(argc, argv, ARGS)) != EOF) {
		switch (c) {
			case 'f': {
				strncpy(config_file, optarg, sizeof(config_file));
				break;
			}
			case 'c': {
				count = atoi(optarg);
				break;
			}
			case 's': {
				sync = 1;
				break;
			}
			default: {
				fprintf(stderr, "Unrecognized command %c\n", (char)c);
				fprintf(stderr, "%s", Usage);
				exit(1);
			}
		}
	}
	if(config_file[0] == 0) {
		fprintf(stderr, "Must specify config file\n");
		fprintf(stderr, "%s", Usage);
		exit(1);
	}

	FILE *fp = fopen(config_file, "r");
	if (fp == NULL) {
		fprintf(stderr, "can't read config file\n");
		exit(1);
	}
	if (raft_init_client(fp) < 0) {
		fprintf(stderr, "can't init client\n");
		fclose(fp);	
		exit(1);
	}
	fclose(fp);

	char current_leader[RAFT_WOOF_NAME_LENGTH];
	if (raft_current_leader(raft_client_leader, current_leader) < 0) {
		fprintf(stderr, "couldn't get the current leader\n");
		exit(1);
	}
	raft_set_client_leader(current_leader);

	unsigned long index[count], term[count], seq_no[count];
	int i;
	for (i = 0; i < count; ++i) {
		RAFT_DATA_TYPE data;
		if (sync) {
			sprintf(data.val, "sync_%d", i);
		} else {
			sprintf(data.val, "async_%d", i);
		}
		int err = raft_put(&data, &index[i], &term[i], &seq_no[i], sync);
		while (err == RAFT_REDIRECTED) {
			err = raft_put(&data, &index[i], &term[i], &seq_no[i], sync);
		}
		if (err < 0) {
			printf("failed to put %s\n", data.val);
		} else {
			if (sync) {
				printf("put data[%d]: %s, index: %lu, term: %lu, seq_no: %lu\n", i, data.val, index[i], term[i], seq_no[i]);
			} else {
				printf("put data[%d]: %s, seqno: %lu\n", i, data.val, seq_no[i]);
			}
		}
	}

	if (!sync) {
		for (i = 0; i < count; ++i) {
			if (raft_index_from_put(&index[i], &term[i], seq_no[i]) < 0) {
				printf("failed to get index and term from request[%d]\n", i);
			}
		}
	}

	for (i = 0; i < count; ++i) {
		RAFT_DATA_TYPE data;
		printf("getting data[%d], index: %lu, term: %lu\n", i, index[i], term[i]);
		int err = raft_get(&data, index[i], term[i]);
		while (err == RAFT_PENDING) {
			err = raft_get(&data, index[i], term[i]);
		}
		if (err < 0) {
			printf("failed to get data[%d]\n", i);
		} else {
			printf("get data[%d]: %s, index: %lu, term: %lu\n", i, data.val, index[i], term[i]);
		}
	}

	return 0;
}

