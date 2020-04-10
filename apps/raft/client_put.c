#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "woofc.h"
#include "woofc-host.h"
#include "raft.h"

#define ARGS "h:d:s"
char *Usage = "client_put -h server_woof -d data\n-s for synchronously put\n";

int get_result_delay = 20;

int main(int argc, char **argv) {
	char server_woof[RAFT_WOOF_NAME_LENGTH];
	RAFT_CLIENT_PUT_ARG arg;
	bool sync = false;

	int c;
	while ((c = getopt(argc, argv, ARGS)) != EOF) {
		switch (c) {
			case 'h':
				strncpy(server_woof, optarg, RAFT_WOOF_NAME_LENGTH);
				if (server_woof[strlen(server_woof) - 1] == '/') {
					server_woof[strlen(server_woof) - 1] = 0;
				}
				break;
			case 'd':
				strncpy(arg.data.val, optarg, sizeof(arg.data.val));
				break;
			case 's':
				sync = true;
				break;
			default:
				fprintf(stderr, "Unrecognized command %c\n", (char)c);
				fprintf(stderr, "%s", Usage);
				exit(1);
		}
	}
	if(server_woof[0] == 0 || arg.data.val[0] == 0) {
		fprintf(stderr, "%s", Usage);
		fflush(stderr);
		exit(1);
	}

	char woof_name[RAFT_WOOF_NAME_LENGTH];
	sprintf(woof_name, "%s/%s", server_woof, RAFT_CLIENT_PUT_ARG_WOOF);
	unsigned long seq = WooFPut(woof_name, NULL, &arg);
	if (WooFInvalid(seq)) {
		fprintf(stderr, "call failed, 0, 0\n");
		fflush(stderr);
		exit(1);
	}

	RAFT_CLIENT_PUT_RESULT result;
	sprintf(woof_name, "%s/%s", server_woof, RAFT_CLIENT_PUT_RESULT_WOOF);
	unsigned long latest_result = 0;
	while (latest_result < seq) {
		latest_result = WooFGetLatestSeqno(woof_name);
		usleep(get_result_delay * 1000);
	}
	int err = WooFGet(woof_name, &result, seq);
	if (err < 0) {
		fprintf(stderr, "error, 0, 0\n");
		fflush(stderr);
		exit(1);
	}
	if (result.appended == false) {
		printf("not a leader, %lu, %lu\n", result.seq_no, result.term);
		fflush(stdout);
	} else {
		if (sync) {
			RAFT_SERVER_STATE server_state;
			sprintf(woof_name, "%s/%s", server_woof, RAFT_SERVER_STATE_WOOF);
			unsigned long commit_index = 0;
			while (commit_index < result.seq_no) {
				unsigned long latest_server_state = WooFGetLatestSeqno(woof_name);
				err = WooFGet(woof_name, &server_state, latest_server_state);
				if (err < 0) {
					fprintf(stderr, "error, 0, 0\n");
					fflush(stderr);
					exit(1);
				}
				commit_index = server_state.commit_index;
				usleep(get_result_delay * 1000);
			}

			RAFT_LOG_ENTRY entry;
			sprintf(woof_name, "%s/%s", server_woof, RAFT_LOG_ENTRIES_WOOF);
			err = WooFGet(woof_name, &entry, result.seq_no);
			if (err < 0) {
				fprintf(stderr, "error, 0, 0\n");
				fflush(stderr);
				exit(1);
			}
			if (entry.term == result.term) {
				printf("committed, %lu, %lu\n", result.seq_no, entry.term);
				fflush(stdout);
			} else {
				printf("rejected, %lu, %lu\n", result.seq_no, entry.term);
				fflush(stdout);
			}
		} else {
			printf("pending, %lu, %lu\n", result.seq_no, result.term);
			fflush(stdout);
		}
	}
	return 0;
}

