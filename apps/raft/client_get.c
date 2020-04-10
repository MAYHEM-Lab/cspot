#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "raft.h"
#include "client.h"

#define ARGS "f:i:t:s"
char *Usage = "client_get -f config -i index -t term\n-s for synchronously get\n";

int main(int argc, char **argv) {
	char config[256];
	bool sync = false;
	unsigned long index = 0;
	unsigned long term = 0;

	int c;
	while ((c = getopt(argc, argv, ARGS)) != EOF) {
		switch (c) {
			case 'f': {
				strncpy(config, optarg, sizeof(config));
				break;
			}
			case 'i': {
				index = strtoul(optarg, NULL, 0);
				break;
			}
			case 't': {
				term = strtoul(optarg, NULL, 0);
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

	if (config[0] == 0 || index == 0) {
		fprintf(stderr, "%s", Usage);
		exit(1);
	}

	FILE *fp = fopen(config, "r");
	if (fp == NULL) {
		fprintf(stderr, "couldn't open config file");
		fflush(stderr);
		exit(1);
	}
	raft_init_client(fp);
	fclose(fp);

	RAFT_DATA_TYPE data;
	int err = raft_get(&data, index, term);
	if (err < 0) {
		return err;
	}
	printf("%s\n", data.val);
	fflush(stdout);

	return err;
}

