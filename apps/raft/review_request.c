#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "raft.h"

char function_tag[] = "review_request";

int review_request(WOOF *wf, unsigned long seq_no, void *ptr)
{
	RAFT_REVIEW_REQUEST_ARG *reviewing = (RAFT_REVIEW_REQUEST_ARG *)ptr;

	log_set_level(LOG_INFO);
	// log_set_level(LOG_DEBUG);
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

	// if we're reviewing the old term request, change to the newest term
	if (reviewing->term < server_state.current_term) {
		sprintf(log_msg, "was reviewing term %lu, current term is %lu", reviewing->term, server_state.current_term);
		log_debug(function_tag, log_msg);
		reviewing->term = server_state.current_term;
		memset(reviewing->voted_for, 0, RAFT_WOOF_NAME_LENGTH);
	}

	unsigned long last_vote_request = WooFGetLatestSeqno(RAFT_REQUEST_VOTE_ARG_WOOF);
	if (WooFInvalid(last_vote_request)) {
		sprintf(log_msg, "couldn't get the latest seqno from %s", RAFT_REQUEST_VOTE_ARG_WOOF);
		log_error(function_tag, log_msg);
		exit(1);
	}
	
	unsigned long i;
	RAFT_REQUEST_VOTE_ARG request;
	RAFT_REQUEST_VOTE_RESULT result;
	for (i = reviewing->last_request_seqno + 1; i <= last_vote_request; ++i) {
		// read the request
		err = WooFGet(RAFT_REQUEST_VOTE_ARG_WOOF, &request, i);
		if (err < 0) {
			sprintf(log_msg, "couldn't get the request at %lu", i);
			log_error(function_tag, log_msg);
			exit(1);
		}

		// TODO: return with the current term
		result.term = request.term;
		if (request.term < reviewing->term) { // current term is higher than the request
			result.granted = false;
		} else {
			if (request.term > reviewing->term) {
				// fallback to follower
				RAFT_TERM_ENTRY new_term;
				new_term.term = request.term;
				new_term.role = RAFT_FOLLOWER;
				unsigned long seq = WooFPut(RAFT_TERM_ENTRIES_WOOF, NULL, &new_term);
				if (WooFInvalid(seq)) {
					log_error(function_tag, "couldn't queue the new term request to chair");
					exit(1);
				}

				// update review progress' term
				reviewing->term = request.term;
				memset(reviewing->voted_for, 0, RAFT_WOOF_NAME_LENGTH);
			}
			
			// TODO: check if the log is up-to-date ($5.4)
			if (reviewing->voted_for[0] == 0 || strcmp(reviewing->voted_for, request.candidate_woof) == 0) {
				memcpy(reviewing->voted_for, request.candidate_woof, RAFT_WOOF_NAME_LENGTH);
				result.granted = true;
			} else {
				result.granted = false;
			}
		}
		// return the request
		char candidate_result_wooF[RAFT_WOOF_NAME_LENGTH];
		sprintf(candidate_result_wooF, "%s/%s", request.candidate_woof, RAFT_REQUEST_VOTE_RESULT_WOOF);
		unsigned long seq = WooFPut(candidate_result_wooF, NULL, &result);
		if (WooFInvalid(seq)) {
			sprintf(log_msg, "couldn't return the vote result to %s", candidate_result_wooF);
			log_error(function_tag, log_msg);
			exit(1);
		}
	}
	// queue the next review function
	reviewing->last_request_seqno = last_vote_request;
	unsigned long seq = WooFPut(RAFT_REVIEW_REQUEST_ARG_WOOF, "review_request", reviewing);
	if (WooFInvalid(seq)) {
		log_error(function_tag, "couldn't queue the next review function");
		exit(1);
	}
	return 1;

}
