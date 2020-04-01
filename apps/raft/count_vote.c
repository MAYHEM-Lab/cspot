#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "woofc.h"
#include "raft.h"

char function_tag[] = "count_vote";

int count_vote(WOOF *wf, unsigned long seq_no, void *ptr)
{
	RAFT_COUNT_VOTE_ARG *count_vote_arg = (RAFT_COUNT_VOTE_ARG *)ptr;
	// log_set_level(LOG_INFO);
	log_set_level(LOG_DEBUG);
	log_set_output(stdout);

	unsigned long latest_server_state = WooFGetLatestSeqno(RAFT_SERVER_STATE_WOOF);
	if (WooFInvalid(latest_server_state)) {
		sprintf(log_msg, "couldn't get the latest seqno from %s", RAFT_SERVER_STATE_WOOF);
		log_error(function_tag, log_msg);
		exit(1);
	}
	RAFT_SERVER_STATE server_state;
	int err = WooFGet(RAFT_SERVER_STATE_WOOF, &server_state, latest_server_state);
	if (err < 0) {
		log_error(function_tag, "couldn't get the server state");
		exit(1);
	}

	// server's term is higher than the election, ignore it
	if (count_vote_arg->term < server_state.current_term) {
		sprintf(log_msg, "current term %lu is higher than election's term %lu, ignore the election", server_state.current_term, count_vote_arg->term);
		log_debug(function_tag, log_msg);
		return 1;
	}

	// start counting the votes
	unsigned long last_vote_result = WooFGetLatestSeqno(RAFT_REQUEST_VOTE_RESULT_WOOF);
	if (WooFInvalid(last_vote_result)) {
		sprintf(log_msg, "couldn't get the latest seqno from %s", RAFT_REQUEST_VOTE_RESULT_WOOF);
		log_error(function_tag, log_msg);
		exit(1);
	}
	if (count_vote_arg->pool_seqno < last_vote_result) {
		RAFT_REQUEST_VOTE_RESULT vote;
		unsigned long i;
		// sprintf(log_msg, "counting votes from seqno %lu to %lu", count_vote_arg->pool_seqno + 1, last_vote_result);
		// log_debug(function_tag, log_msg);
		for (i = count_vote_arg->pool_seqno + 1; i <= last_vote_result; ++i) {
			err = WooFGet(RAFT_REQUEST_VOTE_RESULT_WOOF, &vote, i);
			if (err < 0) {
				sprintf(log_msg, "couldn't get the vote at seqno %lu", i);
				log_error(function_tag, log_msg);
				exit(1);
			}
			if (vote.term > count_vote_arg->term) {
				// there is a vote with higher rank, no need to keep counting vote for this term
				sprintf(log_msg, "found a vote with higher term %lu, stop counting for term %lu", vote.term, count_vote_arg->term);
				log_debug(function_tag, log_msg);
				return 1; 
			}
			if (vote.granted == true && vote.term == count_vote_arg->term) {
				count_vote_arg->granted_votes++;
			}
		}
		sprintf(log_msg, "counted %d granted votes for term %lu", count_vote_arg->granted_votes, count_vote_arg->term);
		log_debug(function_tag, log_msg);

		// if the majority granted, promoted to leader
		if (count_vote_arg->granted_votes > server_state.members / 2) {
			RAFT_TERM_ENTRY new_term;
			new_term.term = count_vote_arg->term;
			new_term.role = RAFT_LEADER;
			unsigned long seq = WooFPut(RAFT_TERM_ENTRIES_WOOF, NULL, &new_term);
			if (WooFInvalid(seq)) {
				log_error(function_tag, "couldn't queue the new term request to chair");
				exit(1);
			}
			sprintf(log_msg, "asked the term chair to be promoted to leader for term %lu", count_vote_arg->term);
			log_info(function_tag, log_msg);
			return 1;
		}
	}
	// else keep counting
	count_vote_arg->pool_seqno = last_vote_result;
	unsigned long seq = WooFPut(RAFT_COUNT_VOTE_ARG_WOOF, "count_vote", count_vote_arg);
	if (WooFInvalid(seq)) {
		sprintf(log_msg, "couldn't queue the next count_vote function for term %lu", count_vote_arg->term);
		log_error(function_tag, log_msg);
		exit(1);
	}
	// sprintf(log_msg, "queued the next count_vote function for term %lu", count_vote_arg->term);
	// log_debug(function_tag, log_msg);
	return 1;
}
