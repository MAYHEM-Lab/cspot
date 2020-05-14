#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "raft.h"
#include "raft_client.h"

#define ARGS "f:c:t:s"
char *Usage = "client_test -f config_file -c count\n\
-s for synchronously put, -t timeout\n";

int main(int argc, char **argv) {
	char config_file[256] = "";
	int count = 10;
	int sync = 0;
	int timeout = 0;

	int c;
	while ((c = getopt(argc, argv, ARGS)) != EOF) {
		switch (c) {
			case 'f': {
				strncpy(config_file, optarg, sizeof(config_file));
				break;
			}
			case 'c': {
				count = (int)strtoul(optarg, NULL, 0);
				break;
			}
			case 's': {
				sync = 1;
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

	char current_leader[RAFT_NAME_LENGTH];
	if (raft_current_leader(raft_client_leader, current_leader) < 0) {
		fprintf(stderr, "couldn't get the current leader\n");
		exit(1);
	}
	raft_set_client_leader(current_leader);

	if (sync) {
		int i;
		for (i = 0; i < count; ++i) {
			RAFT_DATA_TYPE data;
			sprintf(data.val, "sync_%d", i);
			unsigned long index = raft_sync_put(&data, timeout);
			while (index == RAFT_REDIRECTED) {
				index = raft_sync_put(&data, timeout);
			}
			if (raft_is_error(index)) {
				fprintf(stderr, "failed to put %s: %s\n", data.val, raft_client_error_msg);
			} else {
				printf("put data[%d]: %s, index: %lu\n", i, data.val, index);
			}
		}
	} else {
		unsigned long seq[count];
		int i;
		for (i = 0; i < count; ++i) {
			RAFT_DATA_TYPE data;
			sprintf(data.val, "async_%d", i);
			seq[i] = raft_async_put(&data);
			if (raft_is_error(seq[i])) {
				fprintf(stderr, "failed to put %s: %s\n", data.val, raft_client_error_msg);
			} else {
				printf("put data[%d]: %s, client_request_seqno: %lu\n", i, data.val, seq[i]);
			}
		}
	}
	
	return 0;
}
