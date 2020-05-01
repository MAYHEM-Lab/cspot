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
	if(config_file[0] == 0) {
		fprintf(stderr, "Must specify config file\n");
		fprintf(stderr, "%s", Usage);
		exit(1);
	}

	RAFT_SERVER_STATE server_state;
	server_state.current_term = 0;
	server_state.role = RAFT_FOLLOWER;
	if (observer) {
		server_state.role = RAFT_OBSERVER;
	}
	memset(server_state.voted_for, 0, RAFT_WOOF_NAME_LENGTH);
	server_state.commit_index = 0;
	server_state.current_config = RAFT_CONFIG_STABLE;
	if (node_woof_name(server_state.woof_name) < 0) {
		fprintf(stderr, "Couldn't get woof name\n");
	}
	memcpy(server_state.current_leader, server_state.woof_name, RAFT_WOOF_NAME_LENGTH);
	server_state.observers = 0;
	memset(server_state.member_woofs, 0, (RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS) * RAFT_WOOF_NAME_LENGTH);

	FILE *fp = fopen(config_file, "r");
	if (fp == NULL) {
		fprintf(stderr, "Can't open config file\n");
		exit(1);
	}
	if (read_config(fp, &server_state.members, server_state.member_woofs) < 0) {
		fprintf(stderr, "Can't read config file\n");
		fclose(fp);
		exit(1);
	}
	fclose(fp);

	WooFInit();

	unsigned long seq = WooFPut(RAFT_SERVER_STATE_WOOF, NULL, &server_state);
	if (WooFInvalid(seq)) {
		fprintf(stderr, "Couldn't initialize server state\n");
		exit(1);
	}

	if (monitor_create(RAFT_MONITOR_NAME) < 0) {
		fprintf(stderr, "Failed to create and start the handler monitor\n");
		exit(1);
	}

	printf("Server started.\n");
	printf("WooF namespace: %s\n", server_state.woof_name);
	printf("Cluster has %d members:\n", server_state.members);
	int i;
	for (i = 0; i < server_state.members; ++i) {
		printf("%d: %s\n", i + 1, server_state.member_woofs[i]);
	}

	RAFT_HEARTBEAT heartbeat;
	heartbeat.term = 0;
	heartbeat.timestamp = get_milliseconds();
	seq = WooFPut(RAFT_HEARTBEAT_WOOF, NULL, &heartbeat);
	if (WooFInvalid(seq)) {
		fprintf(stderr, "Couldn't put the first heartbeat\n");
		exit(1);
	}
	printf("Put a heartbeat\n");

	RAFT_CLIENT_PUT_ARG client_put_arg;
	client_put_arg.last_seqno = 0;
	seq = monitor_put(RAFT_MONITOR_NAME, RAFT_CLIENT_PUT_ARG_WOOF, "h_client_put", &client_put_arg);
	if (WooFInvalid(seq)) {
		fprintf(stderr, "Couldn't start h_client_put\n");
		exit(1);
	}
	if (!observer) {
		RAFT_TIMEOUT_CHECKER_ARG timeout_checker_arg;
		timeout_checker_arg.timeout_value = random_timeout(get_milliseconds());
		seq = monitor_put(RAFT_MONITOR_NAME, RAFT_TIMEOUT_CHECKER_WOOF, "h_timeout_checker", &timeout_checker_arg);
		if (WooFInvalid(seq)) {
			fprintf(stderr, "Couldn't start h_timeout_checker\n");
			exit(1);
		}
	}
	printf("Started daemon functions\n");
	
	return 0;
}

