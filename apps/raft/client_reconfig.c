#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "raft.h"
#include "client.h"

#define ARGS "o:f:"
char *Usage = "client_reconfig -o old_config -f new_config\n";

int get_result_delay = 20;

int main(int argc, char **argv) {
	char old_config[256];
	char new_config[256];
	RAFT_DATA_TYPE data;
	
	int c;
	while ((c = getopt(argc, argv, ARGS)) != EOF) {
		switch (c) {
			case 'o': {
				strncpy(old_config, optarg, sizeof(old_config));
				break;
			}
			case 'f': {
				strncpy(new_config, optarg, sizeof(new_config));
				break;
			}
			default: {
				fprintf(stderr, "Unrecognized command %c\n", (char)c);
				fprintf(stderr, "%s", Usage);
				exit(1);
			}
		}
	}

	if (old_config[0] == 0 || new_config[0] == 0) {
		fprintf(stderr, "%s", Usage);
		exit(1);
	}

	FILE *fp = fopen(old_config, "r");
	if (fp == NULL) {
		fprintf(stderr, "couldn't open old config file %s\n", old_config);
		fflush(stderr);
		exit(1);
	}
	raft_init_client(fp);
	fclose(fp);

	fp = fopen(new_config, "r");
	if (fp == NULL) {
		fprintf(stderr, "couldn't open new config file %s\n", new_config);
		fflush(stderr);
		exit(1);
	}
	int members;
	char member_woofs[RAFT_MAX_SERVER_NUMBER][RAFT_WOOF_NAME_LENGTH];
	read_config(fp, &members, member_woofs);
	fclose(fp);

	unsigned long index, term;
	int err = raft_reconfig(members, member_woofs, &index, &term);
	while (err == RAFT_REDIRECTED) {
		err = raft_reconfig(members, member_woofs, &index, &term);
	}
	
	return err;
}

