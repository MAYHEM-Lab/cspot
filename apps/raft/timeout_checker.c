#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "raft.h"

char function_tag[] = "timeout_checker";

int timeout_checker(WOOF *wf, unsigned long seq_no, void *ptr)
{
	RAFT_TIMEOUT_ARG *arg = (RAFT_TIMEOUT_ARG *)ptr;

	log_set_level(LOG_INFO);
	// log_set_level(LOG_DEBUG);
	log_set_output(stdout);
	
	// get the last heartbeat
	unsigned long seq = WooFGetLatestSeqno(RAFT_HEARTBEAT_ARG_WOOF);
	if (WooFInvalid(seq)) {
		sprintf(log_msg, "couldn't get the latest seqno from %s", RAFT_HEARTBEAT_ARG_WOOF);
		log_error(function_tag, log_msg);
		exit(1);
	}
	RAFT_HEARTBEAT_ARG last_heartbeat;
	int err = WooFGet(RAFT_HEARTBEAT_ARG_WOOF, &last_heartbeat, seq);
	if (err < 0) {
		log_error(function_tag, "couldn't get the last heartbeat");
		exit(1);
	}

	unsigned long local_timestamp = get_milliseconds();
	if ((local_timestamp - last_heartbeat.local_timestamp) > last_heartbeat.timeout) {
		sprintf(log_msg, "timeout after %dms", last_heartbeat.timeout);
		log_info(function_tag, log_msg);

		// TODO: new term
		// new term
		RAFT_SERVER_STATE server_state;
		seq = WooFGetLatestSeqno(RAFT_SERVER_STATE_WOOF);
		if (WooFInvalid(seq)) {
			sprintf(log_msg, "couldn't get the latest seqno from %s", RAFT_SERVER_STATE_WOOF);
			log_error(function_tag, log_msg);
			exit(1);
		}
		err = WooFGet(RAFT_SERVER_STATE_WOOF, &server_state, seq);
		if (err < 0) {
			log_error(function_tag, "couldn't get the current server state");
			exit(1);
		}
		
		RAFT_TERM_ENTRY new_term;
		new_term.term = server_state.current_term + 1;
		new_term.role = RAFT_CANDIDATE;

		seq = WooFPut(RAFT_TERM_ENTRIES_WOOF, NULL, &new_term);
		if (WooFInvalid(seq)) {
			log_error(function_tag, "couldn't queue the new term request to the chair");
			exit(1);
		}
		sprintf(log_msg, "asked the chair to increment the current term to %lu", new_term.term);
		log_debug(function_tag, log_msg);

		// new heartbeat
		RAFT_HEARTBEAT_ARG new_heartbeat;
		new_heartbeat.local_timestamp = get_milliseconds();
		new_heartbeat.timeout = random_timeout(new_heartbeat.local_timestamp);
		seq = WooFPut(RAFT_HEARTBEAT_ARG_WOOF, NULL, &new_heartbeat);
		if (WooFInvalid(seq)) {
			log_error(function_tag, "couldn't put the new heartbeat");
			exit(1);
		}
	}

	seq = WooFPut(RAFT_TIMEOUT_ARG_WOOF, "timeout_checker", arg);
	if (WooFInvalid(seq)) {
		log_error(function_tag, "couldn't queue the next timeout function");
		exit(1);
	}
	return 1;

}
