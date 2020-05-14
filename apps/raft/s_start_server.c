#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "woofc.h"
#include "woofc-host.h"
#include "raft.h"
#include "monitor.h"

#define ARGS "f:o"
char *Usage = "start_server -f config_file\n\
-o as observer\n";

int main(int argc, char **argv) {
	char config_file[256];
	int observer = 0;

	int c;
	while ((c = getopt(argc, argv, ARGS)) != EOF) {
		switch (c) {
			case 'f': {
				strncpy(config_file, optarg, sizeof(config_file));
				break;
			}
			case 'o': {
				observer = 1;
				break;
			}
			default: {
				fprintf(stderr, "Unrecognized command %c\n", (char)c);
				fprintf(stderr, "%s", Usage);
				exit(1);
			}
		}
	}
	if (config_file[0] == 0) {
		fprintf(stderr, "Must specify config file\n");
		fprintf(stderr, "%s", Usage);
		exit(1);
	}

	FILE *fp = fopen(config_file, "r");
	if (fp == NULL) {
		fprintf(stderr, "Can't open config file\n");
		exit(1);
	}
	int members;
	char member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH];
	if (read_config(fp, &members, member_woofs) < 0) {
		fprintf(stderr, "Can't read config file\n");
		fclose(fp);
		exit(1);
	}
	fclose(fp);
	
	WooFInit();
	if (start_server(members, member_woofs, observer) < 0) {
		fprintf(stderr, "Can't start server\n");
		exit(1);
	}
	
	return 0;
}

