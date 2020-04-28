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
		fprintf(stderr, "failed to open old config file %s\n", old_config);
		exit(1);
	}
	if (raft_init_client(fp) < 0) {
		fprintf(stderr, "can't init client\n");
		fclose(fp);	
		exit(1);
	}
	fclose(fp);

	fp = fopen(new_config, "r");
	if (fp == NULL) {
		fprintf(stderr, "failed to open new config file %s\n", new_config);
		exit(1);
	}
	int members;
	char member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_WOOF_NAME_LENGTH];
	if (read_config(fp, &members, member_woofs) < 0) {
		fprintf(stderr, "failed to read new config file %s\n", new_config);
		fclose(fp);
		exit(1);
	}
	fclose(fp);

	unsigned long index, term;
	int err = raft_config_change(members, member_woofs, &index, &term);
	while (err == RAFT_REDIRECTED) {
		err = raft_config_change(members, member_woofs, &index, &term);
	}
	
	return err;
}

