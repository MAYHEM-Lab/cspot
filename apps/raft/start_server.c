#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "woofc.h"
#include "woofc-host.h"
#include "raft.h"

#define ARGS "f:"
char *Usage = "start_server -f config_file\n";

int main(int argc, char **argv)
{
	char config_file[256];

	int c;
	while ((c = getopt(argc, argv, ARGS)) != EOF) {
		switch (c) {
			case 'f':
				strncpy(config_file, optarg, sizeof(config_file));
				break;
			default:
				fprintf(stderr, "Unrecognized command %c\n", (char)c);
				fprintf(stderr, "%s", Usage);
				exit(1);
		}
	}
	if(config_file[0] == 0) {
		fprintf(stderr, "Must specify config file\n");
		fprintf(stderr, "%s",Usage);
		fflush(stderr);
		exit(1);
	}

	RAFT_SERVER_STATE server_state;
	server_state.current_term = 0;
	server_state.role = RAFT_FOLLOWER;
	int err = node_woof_name(server_state.woof_name);
	if (err < 0) {
		fprintf(stderr, "Couldn't get woof name\n");
		fflush(stderr);
	}

	FILE *fp = fopen(config_file, "r");
	if (fp == NULL) {
		fprintf(stderr, "Can't open config file\n");
		fflush(stderr);
		exit(1);
	}

	char buffer[256];
	if (fgets(buffer, sizeof(buffer), fp) == NULL) {
		fprintf(stderr, "Wrong format of config file\n");
		fflush(stderr);
		exit(1);
	}
	server_state.members = atoi(buffer);
	int i;
	for (i = 0; i < server_state.members; ++i) {
		if (fgets(buffer, sizeof(buffer), fp) == NULL) {
			fprintf(stderr, "Wrong format of config file\n");
			fflush(stderr);
			exit(1);
		}
		buffer[strcspn(buffer, "\n")] = 0;
		strncpy(server_state.member_woofs[i], buffer, RAFT_WOOF_NAME_LENGTH);
	}

	WooFInit();

	unsigned long seq = WooFPut(RAFT_SERVER_STATE_WOOF, NULL, &server_state);
	if (WooFInvalid(seq)) {
		fprintf(stderr, "Couldn't initialize server state\n");
		fflush(stderr);
		exit(1);
	}

	printf("Server started.\n");
	printf("WooF namespace: %s\n", server_state.woof_name);
	printf("Cluster has %d members:\n", server_state.members);
	for (i = 0; i < server_state.members; ++i) {
		printf("%d: %s\n", i + 1, server_state.member_woofs[i]);
	}
	fflush(stdout);

	RAFT_TERM_CHAIR_ARG term_chair_arg;
	term_chair_arg.last_term_seqno = 0;
	seq = WooFPut(RAFT_TERM_CHAIR_ARG_WOOF, "term_chair", &term_chair_arg);
	if (WooFInvalid(seq)) {
		fprintf(stderr, "Couldn't start the term_chair function loop\n");
		fflush(stderr);
		exit(1);
	}
	printf("Started term_chair function loop\n");
	fflush(stdout);

	RAFT_HEARTBEAT_ARG heartbeat_arg;
	heartbeat_arg.term = 0;
	seq = WooFPut(RAFT_HEARTBEAT_ARG_WOOF, "timeout_checker", &heartbeat_arg);
	if (WooFInvalid(seq)) {
		fprintf(stderr, "Couldn't put the initial heartbear\n");
		fflush(stderr);
		exit(1);
	}
	printf("Put a heartbeat\n");
	fflush(stdout);

	RAFT_REVIEW_REQUEST_VOTE_ARG review_vote_arg;
	review_vote_arg.last_request_seqno = 0;
	review_vote_arg.term = 0;
	memset(review_vote_arg.voted_for, 0, RAFT_WOOF_NAME_LENGTH);
	seq = WooFPut(RAFT_REVIEW_REQUEST_VOTE_ARG_WOOF, "review_request_vote", &review_vote_arg);
	if (WooFInvalid(seq)) {
		fprintf(stderr, "Couldn't start the review_request_vote function loop\n");
		fflush(stderr);
		exit(1);
	}
	printf("Started review_request_vote function loop\n");
	fflush(stdout);

	RAFT_REVIEW_APPEND_ENTRIES_ARG review_append_arg;
	review_append_arg.last_request_seqno = 0;
	seq = WooFPut(RAFT_REVIEW_APPEND_ENTRIES_ARG_WOOF, "review_append_entries", &review_append_arg);
	if (WooFInvalid(seq)) {
		fprintf(stderr, "Couldn't start the review_append_entries function loop\n");
		fflush(stderr);
		exit(1);
	}
	printf("Started review_append_entries function loop\n");
	fflush(stdout);
	
	return 0;
}

