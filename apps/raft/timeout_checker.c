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
	RAFT_HEARTBEAT_ARG *arg = (RAFT_HEARTBEAT_ARG *)ptr;

	// log_set_level(LOG_INFO);
	log_set_level(LOG_DEBUG);
	log_set_output(stdout);
	
	unsigned long timeout = random_timeout(get_milliseconds());
	// wait for the timeout
	usleep(timeout * 1000);

	// get the last heartbeat
	unsigned long latest_heartbeat_seqno = WooFGetLatestSeqno(RAFT_HEARTBEAT_ARG_WOOF);
	if (WooFInvalid(latest_heartbeat_seqno)) {
		sprintf(log_msg, "couldn't get the latest seqno from %s", RAFT_HEARTBEAT_ARG_WOOF);
		log_error(function_tag, log_msg);
		exit(1);
	}

	// check if there's new heartbeat since went into sleep
	RAFT_HEARTBEAT_ARG heartbeat;
	unsigned long i;
	for (i = seq_no + 1; i <= latest_heartbeat_seqno; ++i) {
		int err = WooFGet(RAFT_HEARTBEAT_ARG_WOOF, &heartbeat, i);
		if (err < 0) {
			sprintf(log_msg, "couldn't get the heartbeat at %lu", i);
			log_error(function_tag, log_msg);
			exit(1);
		}
		if (heartbeat.term >= arg->term) {
			// found a new heartbeat
			return 1;
		}
	}
	
	// no new heartbeat found, timeout
	sprintf(log_msg, "timeout after %dms at term %lu", timeout, arg->term);
	log_warn(function_tag, log_msg);
	// new term
	RAFT_TERM_ENTRY new_term;
	new_term.term = arg->term + 1;
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
