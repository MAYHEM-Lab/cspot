#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "woofc.h"
#include "raft.h"

int review_request_vote(WOOF *wf, unsigned long seq_no, void *ptr) {
	RAFT_FUNCTION_LOOP *function_loop = (RAFT_FUNCTION_LOOP *)ptr;

	log_set_tag("review_request_vote");
	// log_set_level(LOG_INFO);
	log_set_level(LOG_DEBUG);
	log_set_output(stdout);

	unsigned long latest_vote_request = WooFGetLatestSeqno(RAFT_REQUEST_VOTE_ARG_WOOF);
	if (WooFInvalid(latest_vote_request)) {
		log_error("couldn't get the latest seqno from %s", RAFT_REQUEST_VOTE_ARG_WOOF);
		exit(1);
	}
	if (function_loop->last_reviewed_request_vote < latest_vote_request) {
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

		unsigned long i;
		RAFT_REQUEST_VOTE_ARG request;
		RAFT_REQUEST_VOTE_RESULT result;
		for (i = function_loop->last_reviewed_request_vote + 1; i <= latest_vote_request; ++i) {
			// read the request
			err = WooFGet(RAFT_REQUEST_VOTE_ARG_WOOF, &request, i);
			if (err < 0) {
				log_error("couldn't get the request at %lu", i);
				exit(1);
			}

			result.candidate_vote_pool_seqno = request.candidate_vote_pool_seqno;
			if (request.term < server_state.current_term) { // current term is higher than the request
				result.term = server_state.current_term; // server term will always be greater than reviewing term
				result.granted = false;
				log_debug("rejected vote from lower term %lu at term %lu", request.term, server_state.current_term);
			} else {
				if (request.term > server_state.current_term) {
					// fallback to follower
					RAFT_TERM_ENTRY new_term;
					new_term.term = request.term;
					new_term.role = RAFT_FOLLOWER;
					unsigned long seq = WooFPut(RAFT_TERM_ENTRIES_WOOF, NULL, &new_term);
					if (WooFInvalid(seq)) {
						log_error("couldn't queue the new term request to chair");
						exit(1);
					}
					log_debug("request term %lu is higher than the current term %lu, fall back to follower", request.term, server_state.current_term);

					function_loop->last_reviewed_request_vote = i - 1;
					sprintf(function_loop->next_invoking, "review_client_put");
					seq = WooFPut(RAFT_FUNCTION_LOOP_WOOF, "review_client_put", function_loop);
					if (WooFInvalid(seq)) {
						log_error("couldn't queue the next function_loop: review_client_put");
						exit(1);
					}
					return 1;
				}
				
				result.term = request.term;
				if (server_state.voted_for[0] == 0 || strcmp(server_state.voted_for, request.candidate_woof) == 0) {
					// check if the log is up-to-date ($5.4)
					// we don't need to worry about append_entries running in parallel
					// because if we're receiveing append_entries request, it means the majority has voted and this vote doesn't matter
					unsigned long latest_log_entry = WooFGetLatestSeqno(RAFT_LOG_ENTRIES_WOOF);	
					if (WooFInvalid(latest_log_entry)) {
						log_error("couldn't get the latest seqno from %s", RAFT_LOG_ENTRIES_WOOF);
						exit(1);
					}
					RAFT_LOG_ENTRY last_log_entry;
					if (latest_log_entry > 0) {
						int err = WooFGet(RAFT_LOG_ENTRIES_WOOF, &last_log_entry, latest_log_entry);
						if (err < 0) {
							log_error("couldn't get the latest log entry %lu from %s", latest_log_entry, RAFT_LOG_ENTRIES_WOOF);
							exit(1);
						}
					}
					if (latest_log_entry > 0 && last_log_entry.term > request.last_log_term) {
						// the server has more up-to-dated entries than the candidate
						result.granted = false;
						log_debug("rejected vote from server with outdated log (last entry at term %lu)", request.last_log_term);
					} else if (latest_log_entry > 0 && last_log_entry.term == request.last_log_term && latest_log_entry > request.last_log_index) {
						// both have same term but the server has more entries
						result.granted = false;
						log_debug("rejected vote from server with outdated log (last entry at index %lu)", request.last_log_index);
					} else {
						// the candidate has more up-to-dated log entries
						memcpy(server_state.voted_for, request.candidate_woof, RAFT_WOOF_NAME_LENGTH);
						unsigned long seq = WooFPut(RAFT_SERVER_STATE_WOOF, NULL, &server_state);
						if (WooFInvalid(seq)) {
							log_error("couldn't update voted_for at term %lu", server_state.current_term);
							exit(1);
						}
						result.granted = true;
						log_debug("granted vote at term %lu", server_state.current_term);
					}
				} else {
					result.granted = false;
					log_debug("rejected vote from since already voted at term %lu", server_state.current_term);
				}
			}
			// return the request
			char candidate_result_woof[RAFT_WOOF_NAME_LENGTH];
			sprintf(candidate_result_woof, "%s/%s", request.candidate_woof, RAFT_REQUEST_VOTE_RESULT_WOOF);
			if (result.granted == true) {
				unsigned long seq = WooFPut(candidate_result_woof, "count_vote", &result);
				if (WooFInvalid(seq)) {
					log_error("couldn't return the vote result to %s", candidate_result_woof);
					exit(1);
				}
			} else {
				unsigned long seq = WooFPut(candidate_result_woof, NULL, &result);
				if (WooFInvalid(seq)) {
					log_error("couldn't return the vote result to %s", candidate_result_woof);
					exit(1);
				}
			}
			function_loop->last_reviewed_request_vote = i;
		}
	}
	
	
	// queue the next review function
	usleep(RAFT_FUNCTION_LOOP_DELAY * 1000);
	sprintf(function_loop->next_invoking, "review_client_put");
	unsigned long seq = WooFPut(RAFT_FUNCTION_LOOP_WOOF, "review_client_put", function_loop);
	if (WooFInvalid(seq)) {
		log_error("couldn't queue the next function_loop: review_client_put");
		exit(1);
	}
	return 1;

}
