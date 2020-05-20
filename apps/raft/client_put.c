#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "raft.h"
#include "raft_client.h"

#define ARGS "f:d:srt:"
char *Usage = "client_put -f config -d data\n\
-s for synchronously put, -t timeout, -r to redirect if leader is down\n";

int get_result_delay = 20;

int main(int argc, char **argv) {
	char config[256];
	RAFT_DATA_TYPE data;
	memset(data.val, 0, sizeof(data.val));
	int sync = 0;
	int redirect = 0;
	int timeout = 0;

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

	if (config[0] == 0 || data.val[0] == 0) {
		fprintf(stderr, "%s", Usage);
		exit(1);
	}

	FILE *fp = fopen(config, "r");
	if (fp == NULL) {
		fprintf(stderr, "can't read config file\n");
		exit(1);
	}
	int members;
	char member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH];
	if (read_config(fp, &members, member_woofs) < 0) {
		fprintf(stderr, "failed to read the config file\n");
		fclose(fp);	
		exit(1);
	}
	if (raft_init_client(members, member_woofs) < 0) {
		fprintf(stderr, "can't init client\n");
		fclose(fp);	
		exit(1);
	}
	fclose(fp);
	
	if (sync) {
		unsigned long index = raft_sync_put(&data, timeout);
		while (index == RAFT_REDIRECTED) {
			index = raft_sync_put(&data, timeout);
		}
		if (raft_is_error(index)) {
			fprintf(stderr, "failed to put %s: %s\n", data.val, raft_error_msg);
			exit(1);
		} else {
			printf("put %s, index: %lu\n", data.val, index);
		}
	} else {
		unsigned long seq = raft_async_put(&data);
		if (raft_is_error(seq)) {
			fprintf(stderr, "failed to put %s: %s\n", data.val, raft_error_msg);
			exit(1);
		} else {
			printf("put %s, client_request_seqno: %lu\n", data.val, seq);
		}
	}
	
	return 0;
}

