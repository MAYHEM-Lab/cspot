#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "woofc.h"
#include "woofc-host.h"
#include "raft.h"

int NUMBER_OF_WOOF = 14;
char *WOOF_TO_CREATE[] = {RAFT_LOG_ENTRIES_WOOF, RAFT_SERVER_STATE_WOOF, RAFT_REQUEST_VOTE_ARG_WOOF, RAFT_REQUEST_VOTE_RESULT_WOOF, 
	RAFT_REVIEW_REQUEST_VOTE_ARG_WOOF, RAFT_APPEND_ENTRIES_ARG_WOOF, RAFT_APPEND_ENTRIES_RESULT_WOOF, 
	RAFT_REVIEW_APPEND_ENTRIES_ARG_WOOF, RAFT_TERM_ENTRIES_WOOF, RAFT_NEXT_INDEX_WOOF, RAFT_REPLICATE_ENTRIES_WOOF,
	RAFT_TERM_CHAIR_ARG_WOOF, RAFT_TIMEOUT_ARG_WOOF, RAFT_HEARTBEAT_ARG_WOOF};
unsigned long WOOF_ELEMENT_SIZE[] = {sizeof(RAFT_LOG_ENTRY), sizeof(RAFT_SERVER_STATE), sizeof(RAFT_REQUEST_VOTE_ARG), sizeof(RAFT_REQUEST_VOTE_RESULT),
	sizeof(RAFT_REVIEW_REQUEST_VOTE_ARG), sizeof(RAFT_APPEND_ENTRIES_ARG), sizeof(RAFT_APPEND_ENTRIES_RESULT), 
	sizeof(RAFT_REVIEW_APPEND_ENTRIES_ARG), sizeof(RAFT_TERM_ENTRY), sizeof(RAFT_NEXT_INDEX), sizeof(RAFT_REPLICATE_ENTRIES_ARG),
	sizeof(RAFT_TERM_CHAIR_ARG), sizeof(RAFT_TIMEOUT_ARG), sizeof(RAFT_HEARTBEAT_ARG)};

int main(int argc, char **argv)
{
	WooFInit();
	int i, err;
	for (i = 0; i < NUMBER_OF_WOOF; i++) {
		err = WooFCreate(WOOF_TO_CREATE[i], WOOF_ELEMENT_SIZE[i], RAFT_WOOF_HISTORY_SIZE);
		if (err < 0) {
			fprintf(stderr, "Couldn't create woof %s\n", WOOF_TO_CREATE[i]);
			fflush(stderr);
			exit(1);
		}
	}
	return 0;
}

