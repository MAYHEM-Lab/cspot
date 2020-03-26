#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "woofc.h"
#include "raft.h"

char function_tag[] = "request_vote";

int request_vote(WOOF *wf, unsigned long seq_no, void *ptr)
{
	RAFT_REQUEST_VOTE_ARG *request = (RAFT_REQUEST_VOTE_ARG *)ptr;

	log_set_level(LOG_DEBUG);
	log_set_output(stdout);
	
	// get the server's current term
	unsigned long last_server_state = WooFGetLatestSeqno(RAFT_SERVER_STATE_WOOF);
	if (WooFInvalid(last_server_state)) {
		sprintf(log_msg, "couldn't get the latest seqno from %s", RAFT_SERVER_STATE_WOOF);
		log_error(function_tag, log_msg);
		exit(1);
	}
	RAFT_SERVER_STATE server_state;
	int err = WooFGet(RAFT_SERVER_STATE_WOOF, &server_state, last_server_state);
	if (err < 0) {
		log_error(function_tag, "couldn't get the server state");
		exit(1);
	}

	RAFT_REQUEST_VOTE_RESULT result;
	result.term = request->term;
	if (request->term < server_state.current_term) { // current term is higher than the request
		result.granted = false;
	} else {
		if (request->term > server_state.current_term) {
			// fallback to follower
			RAFT_TERM_ENTRY new_term;
			new_term.term = request->term;
			new_term.role = RAFT_FOLLOWER;
			new_term.election = false;
			unsigned long seq = WooFPut(RAFT_TERM_ENTRIES_WOOF, NULL, &new_term);
			if (WooFInvalid(seq)) {
				log_error(function_tag, "couldn't queue the new term request to chair");
				exit(1);
			}
		}
		
		// TODO: check if the log is up-to-date ($5.4)
		if (arg->voted_for[0] == 0 || strcmp(arg->voted_for, request.candidate_woof) == 0) {
			memcpy(arg->voted_for, request.candidate_woof, RAFT_WOOF_NAME_LENGTH);
			result.granted = true;
		} else {
			result.granted = false;
		}
	}
	// return the request
	char candidate_result_wooF[RAFT_WOOF_NAME_LENGTH];
	sprintf(candidate_result_wooF, "%s/%s", request->candidate_woof, RAFT_REQUEST_VOTE_RESULT_WOOF);
	unsigned long seq = WooFPut(candidate_result_wooF, NULL, &result);
	if (WooFInvalid(seq)) {
		sprintf(log_msg, "couldn't return the vote result to %s", candidate_result_wooF);
		log_error(function_tag, log_msg);
		exit(1);
	}

	// queue the next review function
	seq = WooFPut(RAFT_REVIEW_REQUEST_ARG_WOOF, "review_request", &arg);
	if (WooFInvalid(seq)) {
		log_error(function_tag, "couldn't queue the next review function");
		exit(1);
	}
	return 1;

}
