#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "raft.h"

char function_tag[] = "term_chair";

int term_chair(WOOF *wf, unsigned long seq_no, void *ptr)
{
	RAFT_TERM_CHAIR_ARG *arg = (RAFT_TERM_CHAIR_ARG *)ptr;

	log_set_level(LOG_INFO);
	// log_set_level(LOG_DEBUG);
	log_set_output(stdout);
	
	// get the current term
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

	unsigned long last_term_entry = WooFGetLatestSeqno(RAFT_TERM_ENTRIES_WOOF);
	if (WooFInvalid(last_term_entry)) {
		sprintf(log_msg, "couldn't get the latest seqno from %s", RAFT_TERM_ENTRIES_WOOF);
		log_error(function_tag, log_msg);
		exit(1);
	}
	// sprintf(log_msg, "reviewing new term request from %lu to %lu", arg->last_term_seqno + 1, last_term_entry);
	// log_debug(function_tag, log_msg);
	if (arg->last_term_seqno >= last_term_entry) {
		unsigned long seq = WooFPut(RAFT_TERM_CHAIR_ARG_WOOF, "term_chair", arg);
		if (WooFInvalid(seq)) {
			log_error(function_tag, "couldn't queue the next term_chair function handler");
			exit(1);
		}
		return 1;
	}

	// read the queued entries and find out the highest term
	unsigned long next_term = server_state.current_term;
	int next_role = server_state.role;
	unsigned long i;
	RAFT_TERM_ENTRY entry;
	for (i = arg->last_term_seqno + 1; i <= last_term_entry; ++i) {
		err = WooFGet(RAFT_TERM_ENTRIES_WOOF, &entry, i);
		if (err < 0) {
			sprintf(log_msg, "couldn't get the term entry at %lu", i);
			log_error(function_tag, log_msg);
			exit(1);
		}
		if (entry.term > next_term) {
			next_term = entry.term;
			next_role = entry.role;
		} else if (entry.term == next_term) {
			switch (entry.role) {
				case RAFT_LEADER: {
					// leader at the same term has highest priority
					next_role = RAFT_LEADER;
					break;
				}
				case RAFT_FOLLOWER: {
					// not possible being elected as leader for this term
					// if was going to be candidate, there's no need since another has being elected as leader
					next_role = RAFT_FOLLOWER;
					break;
				}
				case RAFT_CANDIDATE: {
					if (next_role != RAFT_LEADER && next_role != RAFT_FOLLOWER) {
						next_role = RAFT_CANDIDATE;
						break;
					}
				}
			}
		}
	}
	arg->last_term_seqno = last_term_entry;

	// if there is a new term
	if (next_term >= server_state.current_term) {
		// update the server's current term
		server_state.current_term = next_term;
		server_state.role = next_role;
		unsigned long seq = WooFPut(RAFT_SERVER_STATE_WOOF, NULL, &server_state);
		if (WooFInvalid(seq)) {
			sprintf(log_msg, "couldn't increment the current term to %lu", next_term);
			log_error(function_tag, log_msg);
			exit(1);
		}
		sprintf(log_msg, "incremented the current term to %lu, role: ", next_term);
		switch (next_role) {
			case RAFT_LEADER: {
				strcat(log_msg, "LEADER");
				break;
			}
			case RAFT_FOLLOWER: {
				strcat(log_msg, "FOLLOWER");
				break;
			}
			case RAFT_CANDIDATE: {
				strcat(log_msg, "CANDIDATE");
				break;
			}
		}
		log_debug(function_tag, log_msg);

		if (next_role == RAFT_CANDIDATE) {
			// TODO: request vote from members
			// initialize the vote progress
			RAFT_COUNT_VOTE_ARG count_vote_arg;
			count_vote_arg.term = server_state.current_term;
			count_vote_arg.granted_votes = 0;
			count_vote_arg.pool_seqno = WooFGetLatestSeqno(RAFT_REQUEST_VOTE_RESULT_WOOF);
			if (WooFInvalid(count_vote_arg.pool_seqno)) {
				sprintf(log_msg, "couldn't get the latest seqno from %s", RAFT_REQUEST_VOTE_RESULT_WOOF);
				log_error(function_tag, log_msg);
				exit(1);
			}
			seq = WooFPut(RAFT_COUNT_VOTE_ARG_WOOF, "count_vote", &count_vote_arg);
			if (WooFInvalid(seq)) {
				sprintf(log_msg, "couldn't queue the count_vote function for term %lu", count_vote_arg.term);
				log_error(function_tag, log_msg);
				exit(1);
			}
			sprintf(log_msg, "queued count_vote function for term %lu", count_vote_arg.term);
			log_debug(function_tag, log_msg);
			
			// requesting votes from members
			RAFT_REQUEST_VOTE_ARG request_vote_arg;
			request_vote_arg.term = server_state.current_term;
			strncpy(request_vote_arg.candidate_woof, server_state.woof_name, RAFT_WOOF_NAME_LENGTH);
			char request_vote_woof[RAFT_WOOF_NAME_LENGTH];		
			int i;
			for (i = 0; i < server_state.members; ++i) {
				sprintf(request_vote_woof, "%s/%s", server_state.member_woofs[i], RAFT_REQUEST_VOTE_ARG_WOOF);
				seq = WooFPut(request_vote_woof, NULL, &request_vote_arg);
				if (WooFInvalid(seq)) {
					sprintf(log_msg, "couldn't request vote from %s", request_vote_woof);
					log_warn(function_tag, log_msg);
					continue;
				}
			}
			RAFT_HEARTBEAT_ARG heartbeat_arg;
			heartbeat_arg.local_timestamp = get_milliseconds();
			heartbeat_arg.timeout = random_timeout(heartbeat_arg.local_timestamp);
			seq = WooFPut(RAFT_HEARTBEAT_ARG_WOOF, NULL, &heartbeat_arg);
			if (WooFInvalid(seq)) {
				sprintf(log_msg, "couldn't put a new heartbeat after requesting votes for term %lu", request_vote_arg.term);
				log_error(function_tag, log_msg);
				exit(1);
			}
			sprintf(log_msg, "requested votes from %d members and put a new heartbeat", server_state.members);
			log_debug(function_tag, log_msg);
		}

		if (next_role == RAFT_LEADER) {
			// TODO: send out heartbeats
		}
	}

	unsigned long seq = WooFPut(RAFT_TERM_CHAIR_ARG_WOOF, "term_chair", arg);
	if (WooFInvalid(seq)) {
		log_error(function_tag, "couldn't queue the next term_chair function handler");
		exit(1);
	}
	return 1;

}
