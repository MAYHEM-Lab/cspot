#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "raft.h"

int append_entries_result(WOOF *wf, unsigned long seq_no, void *ptr) {
	RAFT_APPEND_ENTRIES_RESULT *result = (RAFT_APPEND_ENTRIES_RESULT *)ptr;
	
	log_set_tag("append_entries_result");
	// log_set_level(LOG_INFO);
	log_set_level(LOG_DEBUG);
	log_set_output(stdout);

	// get the server's current term
	unsigned long last_server_state = WooFGetLatestSeqno(RAFT_SERVER_STATE_WOOF);
	if (WooFInvalid(last_server_state)) {
		log_error("couldn't get the latest seqno from %s", RAFT_SERVER_STATE_WOOF);
		exit(1);
	}
	RAFT_SERVER_STATE server_state;
	int err = WooFGet(RAFT_SERVER_STATE_WOOF, &server_state, last_server_state);
	if (err < 0) {
		log_error("couldn't get the server state");
		exit(1);
	}

	if (result->term > server_state.current_term) {
		// fall back to follower
		RAFT_TERM_ENTRY new_term;
		new_term.term = result->term;
		new_term.role = RAFT_FOLLOWER;
		unsigned long seq = WooFPut(RAFT_TERM_ENTRIES_WOOF, NULL, &new_term);
		if (WooFInvalid(seq)) {
			log_error("couldn't ask term chair to fall back to follower at term %lu", new_term.term);
			exit(1);
		}
		log_debug("result term %lu is higher than server's term %lu, fall back to follower", result->term, server_state.current_term);
	}
	return 1;
}
