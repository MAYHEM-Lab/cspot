#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "raft.h"
#include "client.h"

#define ARGS "f:c:s"
char *Usage = "client_test -f config_file -c count\n\
-s for synchronously put\n";

int main(int argc, char **argv) {
	char config_file[256];
	int count = 10;
	bool sync = false;

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
				sync = true;
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
	raft_init_client(fp);
	fclose(fp);

	char current_leader[RAFT_WOOF_NAME_LENGTH];
	if (raft_current_leader(raft_client_leader, current_leader) < 0) {
		fprintf(stderr, "couldn't get the current leader\n");
		exit(1);
	}
	raft_set_client_leader(current_leader);
	int i;
	for (i = 0; i < count; ++i) {
		RAFT_DATA_TYPE data;
		if (sync) {
			sprintf(data.val, "sync_%d", i);
		} else {
			sprintf(data.val, "async_%d", i);
		}
		if (raft_put(&data, NULL, NULL, sync) < 0) {
			printf("failed to put %s\n", data.val);
		}
	}
	return 0;
}

