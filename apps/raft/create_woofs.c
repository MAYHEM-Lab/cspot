#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "woofc.h"
#include "woofc-host.h"
#include "raft.h"

char *WOOF_TO_CREATE[] = {RAFT_LOG_ENTRIES_WOOF, RAFT_SERVER_STATE_WOOF, RAFT_HEARTBEAT_WOOF, RAFT_TIMEOUT_CHECKER_WOOF,
	RAFT_APPEND_ENTRIES_ARG_WOOF, RAFT_APPEND_ENTRIES_RESULT_WOOF, RAFT_APPEND_ENTRIES_REORDER_WOOF,
	RAFT_CLIENT_PUT_ARG_WOOF, RAFT_CLIENT_PUT_RESULT_WOOF, RAFT_CLIENT_PUT_REORDER_WOOF,
	RAFT_RECONFIG_ARG_WOOF, RAFT_REPLICATE_ENTRIES_WOOF, RAFT_REQUEST_VOTE_ARG_WOOF, RAFT_REQUEST_VOTE_RESULT_WOOF};

unsigned long WOOF_ELEMENT_SIZE[] = {sizeof(RAFT_LOG_ENTRY), sizeof(RAFT_SERVER_STATE), sizeof(RAFT_HEARTBEAT), sizeof(RAFT_TIMEOUT_CHECKER_ARG),
	sizeof(RAFT_APPEND_ENTRIES_ARG), sizeof(RAFT_APPEND_ENTRIES_RESULT), sizeof(RAFT_APPEND_ENTRIES_REORDER),
	sizeof(RAFT_CLIENT_PUT_ARG), sizeof(RAFT_CLIENT_PUT_RESULT), sizeof(RAFT_CLIENT_PUT_REORDER),
	sizeof(RAFT_RECONFIG_ARG), sizeof(RAFT_REPLICATE_ENTRIES_ARG), sizeof(RAFT_REQUEST_VOTE_ARG), sizeof(RAFT_REQUEST_VOTE_RESULT)};

int main(int argc, char **argv) {
	WooFInit();
	int i, err;
	for (i = 0; i < sizeof(WOOF_TO_CREATE) / sizeof(char *); i++) {
		err = WooFCreate(WOOF_TO_CREATE[i], WOOF_ELEMENT_SIZE[i], RAFT_WOOF_HISTORY_SIZE);
		if (err < 0) {
			fprintf(stderr, "failed to create woof %s\n", WOOF_TO_CREATE[i]);
			fflush(stderr);
			exit(1);
		}
	}
	return 0;
}

