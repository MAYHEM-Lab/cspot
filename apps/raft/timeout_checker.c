#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "woofc.h"
#include "raft.h"

char function_tag[] = "timeout_checker";

int timeout_checker(WOOF *wf, unsigned long seq_no, void *ptr)
{
	RAFT_HEARTBEAT *arg = (RAFT_HEARTBEAT *)ptr;

	// log_set_level(LOG_INFO);
	log_set_level(LOG_DEBUG);
	log_set_output(stdout);
	
	unsigned long timeout = random_timeout(get_milliseconds());
	// wait for the timeout
	usleep(timeout * 1000);

	// check if it's leader, if so no need to timeout
	unsigned long latest_server_state = WooFGetLatestSeqno(RAFT_SERVER_STATE_WOOF);
	if (WooFInvalid(latest_server_state)) {
		sprintf(log_msg, "couldn't get the latest seqno from %s", RAFT_SERVER_STATE_WOOF);
		log_error(function_tag, log_msg);
		exit(1);
	}
	RAFT_SERVER_STATE server_state;
	int err = WooFGet(RAFT_SERVER_STATE_WOOF, &server_state, latest_server_state);
	if (err < 0) {
		log_error(function_tag, "couldn't get the latest sever state");
		exit(1);
	}
	if (server_state.role == RAFT_LEADER) {
		return 1;
	}

	// check if there's new heartbeat since went into sleep
	unsigned long latest_heartbeat_seqno = WooFGetLatestSeqno(RAFT_HEARTBEAT_WOOF);
	if (WooFInvalid(latest_heartbeat_seqno)) {
		sprintf(log_msg, "couldn't get the latest seqno from %s", RAFT_HEARTBEAT_WOOF);
		log_error(function_tag, log_msg);
		exit(1);
	}
	if (latest_heartbeat_seqno > seq_no) {
		return 1;
	}
	
	// no new heartbeat found, timeout
	sprintf(log_msg, "timeout after %dms at term %lu", timeout, server_state.current_term);
	log_warn(function_tag, log_msg);
	// new term
	RAFT_TERM_ENTRY new_term;
	new_term.term = server_state.current_term + 1;
	new_term.role = RAFT_CANDIDATE;
	unsigned long seq = WooFPut(RAFT_TERM_ENTRIES_WOOF, NULL, &new_term);
	if (WooFInvalid(seq)) {
		log_error(function_tag, "couldn't queue the new term request to the chair");
		exit(1);
	}
	sprintf(log_msg, "asked the chair to increment the current term to %lu", new_term.term);
	log_debug(function_tag, log_msg);
	return 1;

}
