#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "woofc.h"
#include "woofc-host.h"
#include "raft.h"

#define ARGS "h:s:"
char *Usage = "client_get -h server_woof -s seq_no\n";

int get_result_delay = 20;

int main(int argc, char **argv)
{
	char server_woof[RAFT_WOOF_NAME_LENGTH];
	unsigned long seq_no;

	int c;
	while ((c = getopt(argc, argv, ARGS)) != EOF) {
		switch (c) {
			case 'h':
				strncpy(server_woof, optarg, RAFT_WOOF_NAME_LENGTH);
				if (server_woof[strlen(server_woof) - 1] == '/') {
					server_woof[strlen(server_woof) - 1] = 0;
				}
				break;
			case 's':
				seq_no = strtoul(optarg, NULL, 0);
				break;
			default:
				fprintf(stderr, "Unrecognized command %c\n", (char)c);
				fprintf(stderr, "%s", Usage);
				exit(1);
		}
	}
	if(server_woof[0] == 0 || seq_no == 0) {
		fprintf(stderr, "%s", Usage);
		fflush(stderr);
		exit(1);
	}

	RAFT_CLIENT_PUT_RESULT result;
	char woof_name[RAFT_WOOF_NAME_LENGTH];
	sprintf(woof_name, "%s/%s", server_woof, RAFT_CLIENT_PUT_RESULT_WOOF);
	unsigned long latest_result = WooFGetLatestSeqno(woof_name);
	if (WooFInvalid(latest_result)) {
		fprintf(stderr, "fail, 0, 0\n");
		fflush(stderr);
		exit(1);
	}
	if (latest_result < seq_no) {
		printf("pending, 0, 0\n");
		fflush(stdout);
		return 0;
	}
	int err = WooFGet(woof_name, &result, seq_no);
	if (err < 0) {
		fprintf(stderr, "fail, 0, 0\n");
		fflush(stderr);
		exit(1);
	}
	if (result.success == false) {
		printf("fail, %lu, %lu\n", result.seq_no, result.term);
		fflush(stdout);
	} else {
		printf("success, %lu, %lu\n", result.seq_no, result.term);
		fflush(stdout);
	}
	return 0;
}

