#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "raft.h"
#include "monitor.h"

int h_count_vote(WOOF *wf, unsigned long seq_no, void *ptr) {
	RAFT_REQUEST_VOTE_RESULT *result = (RAFT_REQUEST_VOTE_RESULT *)ptr;

	log_set_tag("count_vote");
	// log_set_level(LOG_INFO);
	log_set_level(LOG_DEBUG);
	log_set_output(stdout);

	unsigned long latest_server_state = WooFGetLatestSeqno(RAFT_SERVER_STATE_WOOF);
	if (WooFInvalid(latest_server_state)) {
		log_error("couldn't get the latest seqno from %s", RAFT_SERVER_STATE_WOOF);
		exit(1);
	}
	RAFT_SERVER_STATE server_state;
	if (WooFGet(RAFT_SERVER_STATE_WOOF, &server_state, latest_server_state) < 0) {
		log_error("couldn't get the server state");
		exit(1);
	}

	// server's term is higher than the vote's term, ignore it
	if (result->term < server_state.current_term) {
		log_debug("current term %lu is higher than vote's term %lu, ignore the election", server_state.current_term, result->term);
		monitor_exit(wf, seq_no);
		return 1;
	}
	// the server is already a leader at vote's term, ifnore the vote
	if (result->term == server_state.current_term && server_state.role == RAFT_LEADER) {
		log_debug("already a leader at term %lu, ignore the election", server_state.current_term);
		monitor_exit(wf, seq_no);
		return 1;
	}

	// start counting the votes
	int granted_votes = 0;
	if (result->granted == true) {
		granted_votes++;
	}
	RAFT_REQUEST_VOTE_RESULT vote;
	unsigned long i;
	for (i = result->candidate_vote_pool_seqno + 1; i < seq_no; ++i) {
		if (WooFGet(RAFT_REQUEST_VOTE_RESULT_WOOF, &vote, i) < 0) {
			log_error("couldn't get the vote result at seqno %lu", i);
			exit(1);
		}
		if (vote.granted == true && vote.term == result->term) {
			granted_votes++;
			if (granted_votes > server_state.members / 2) {
				break;
			}
		}
	}
	log_info("counted %d granted votes for term %lu", granted_votes, result->term);

	// if the majority granted, promoted to leader
	if (granted_votes > server_state.members / 2) {
		server_state.current_term = result->term;
		server_state.role = RAFT_LEADER;
		memcpy(server_state.current_leader, server_state.woof_name, RAFT_WOOF_NAME_LENGTH);
		memcpy(server_state.voted_for, server_state.woof_name, RAFT_WOOF_NAME_LENGTH);
		unsigned long seq = WooFPut(RAFT_SERVER_STATE_WOOF, NULL, &server_state);
		if (WooFInvalid(seq)) {
			log_error("couldn't promote itself to leader");
			exit(1);
		}
		log_info("promoted to leader for term %lu", result->term);
	}
	monitor_exit(wf, seq_no);
	return 1;
}
