#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "woofc.h"
#include "raft.h"

char function_tag[] = "review_request_vote";

int review_request_vote(WOOF *wf, unsigned long seq_no, void *ptr)
{
	RAFT_REVIEW_REQUEST_VOTE_ARG *reviewing = (RAFT_REVIEW_REQUEST_VOTE_ARG *)ptr;

	log_set_level(LOG_INFO);
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

	// if we're reviewing the old term request, change to the newest term
	if (reviewing->term < server_state.current_term) {
		sprintf(log_msg, "was reviewing term %lu, current term is %lu", reviewing->term, server_state.current_term);
		log_debug(function_tag, log_msg);
		reviewing->term = server_state.current_term;
		memset(reviewing->voted_for, 0, RAFT_WOOF_NAME_LENGTH);
	}

	unsigned long latest_vote_request = WooFGetLatestSeqno(RAFT_REQUEST_VOTE_ARG_WOOF);
	if (WooFInvalid(latest_vote_request)) {
		sprintf(log_msg, "couldn't get the latest seqno from %s", RAFT_REQUEST_VOTE_ARG_WOOF);
		log_error(function_tag, log_msg);
		exit(1);
	}
	
	unsigned long i;
	RAFT_REQUEST_VOTE_ARG request;
	RAFT_REQUEST_VOTE_RESULT result;
	for (i = reviewing->last_request_seqno + 1; i <= latest_vote_request; ++i) {
		// read the request
		err = WooFGet(RAFT_REQUEST_VOTE_ARG_WOOF, &request, i);
		if (err < 0) {
			sprintf(log_msg, "couldn't get the request at %lu", i);
			log_error(function_tag, log_msg);
			exit(1);
		}

		result.candidate_vote_pool_seqno = request.candidate_vote_pool_seqno;
		if (request.term < reviewing->term) { // current term is higher than the request
			result.term = server_state.current_term; // server term will always be greater than reviewing term
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
				sprintf(log_msg, "request term %lu is higher than reviewing term %lu, fall back to follower", request.term, reviewing->term);
				log_debug(function_tag, log_msg);

				// update review progress' term
				reviewing->term = request.term;
				memset(reviewing->voted_for, 0, RAFT_WOOF_NAME_LENGTH);
			}
			
			result.term = reviewing->term;
			if (reviewing->voted_for[0] == 0 || strcmp(reviewing->voted_for, request.candidate_woof) == 0) {
				// check if the log is up-to-date ($5.4)
				// we don't need to worry about append_entries running in parallel
				// because if we're receiveing append_entries request, it means the majority has voted and this vote doesn't matter
				unsigned long latest_log_entry = WooFGetLatestSeqno(RAFT_LOG_ENTRIES_WOOF);
				if (WooFInvalid(latest_log_entry)) {
					sprintf(log_msg, "couldn't get the latest seqno from %s", RAFT_LOG_ENTRIES_WOOF);
					log_error(function_tag, log_msg);
					exit(1);
				}
				RAFT_LOG_ENTRY last_log_entry;
				int err = WooFGet(RAFT_LOG_ENTRIES_WOOF, &last_log_entry, latest_log_entry);
				if (err < 0) {
					sprintf(log_msg, "couldn't get the latest log entry %lu from %s", latest_log_entry, RAFT_LOG_ENTRIES_WOOF);
					log_error(function_tag, log_msg);
					exit(1);
				}
				if (last_log_entry.term > request.last_log_term) {
					// the server has more up-to-dated entries than the candidate
					result.granted = false;
				} else if (last_log_entry.term == request.last_log_term && latest_log_entry > request.last_log_index) {
					// both have same term but the server has more entries
					result.granted = false;
				} else {
					// the candidate has more up-to-dated log entries
					memcpy(reviewing->voted_for, request.candidate_woof, RAFT_WOOF_NAME_LENGTH);
					result.granted = true;
				}
			} else {
				result.granted = false;
			}
		}
		// return the request
		char candidate_result_woof[RAFT_WOOF_NAME_LENGTH];
		sprintf(candidate_result_woof, "%s/%s", request.candidate_woof, RAFT_REQUEST_VOTE_RESULT_WOOF);
		unsigned long seq = WooFPut(candidate_result_woof, "count_vote", &result);
		if (WooFInvalid(seq)) {
			sprintf(log_msg, "couldn't return the vote result to %s", candidate_result_woof);
			log_error(function_tag, log_msg);
			exit(1);
		}
	}
	// queue the next review function
	usleep(RAFT_LOOP_RATE * 1000);
	reviewing->last_request_seqno = latest_vote_request;
	unsigned long seq = WooFPut(RAFT_REVIEW_REQUEST_VOTE_ARG_WOOF, "review_request_vote", reviewing);
	if (WooFInvalid(seq)) {
		log_error(function_tag, "couldn't queue the next review function");
		exit(1);
	}
	return 1;

}
