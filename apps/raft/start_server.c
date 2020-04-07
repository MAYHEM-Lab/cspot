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
	memset(server_state.voted_for, 0, RAFT_WOOF_NAME_LENGTH);
	server_state.commit_index = 0;
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
		if (buffer[strlen(buffer) - 1] == '/') {
			buffer[strlen(buffer) - 1] = 0;
		}
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

	RAFT_FUNCTION_LOOP function_loop;
	function_loop.review_append_entries.last_request_seqno = 0;
	function_loop.review_request_vote.last_request_seqno = 0;
	function_loop.term_chair.last_term_seqno = 0;
	// function_loop.replicate_entries will be set when promoted to leader
	sprintf(function_loop.next_invoking, "review_append_entries");
	seq = WooFPut(RAFT_FUNCTION_LOOP_WOOF, "review_append_entries", &function_loop);
	if (WooFInvalid(seq)) {
		fprintf(stderr, "Couldn't start function loop\n");
		fflush(stderr);
		exit(1);
	}
	printf("Started function loop\n");
	fflush(stdout);

	RAFT_HEARTBEAT heartbeat_arg;
	seq = WooFPut(RAFT_HEARTBEAT_WOOF, "timeout_checker", &heartbeat_arg);
	if (WooFInvalid(seq)) {
		fprintf(stderr, "Couldn't put the initial heartbear\n");
		fflush(stderr);
		exit(1);
	}
	printf("Put a heartbeat\n");
	fflush(stdout);
	
	return 0;
}

