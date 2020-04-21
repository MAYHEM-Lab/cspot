#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "raft.h"
#include "monitor.h"

int h_count_vote(WOOF *wf, unsigned long seq_no, void *ptr) {
	RAFT_REQUEST_VOTE_RESULT *result = (RAFT_REQUEST_VOTE_RESULT *)monitor_cast(ptr);
	seq_no = monitor_seqno(ptr);

	log_set_tag("count_vote");
	// log_set_level(LOG_INFO);
	log_set_level(LOG_DEBUG);
	log_set_output(stdout);

	unsigned long latest_server_state = WooFGetLatestSeqno(RAFT_SERVER_STATE_WOOF);
	if (WooFInvalid(latest_server_state)) {
		log_error("failed to get the latest seqno from %s", RAFT_SERVER_STATE_WOOF);
		free(result);
		exit(1);
	}
	RAFT_SERVER_STATE server_state;
	if (WooFGet(RAFT_SERVER_STATE_WOOF, &server_state, latest_server_state) < 0) {
		log_error("failed to get the server state");
		free(result);
		exit(1);
	}

	// server's term is higher than the vote's term, ignore it
	if (result->term < server_state.current_term) {
		log_debug("current term %lu is higher than vote's term %lu, ignore the election", server_state.current_term, result->term);
		free(result);
		monitor_exit(ptr);
		return 1;
	}
	// the server is already a leader at vote's term, ifnore the vote
	if (result->term == server_state.current_term && server_state.role == RAFT_LEADER) {
		log_debug("already a leader at term %lu, ignore the election", server_state.current_term);
		free(result);
		monitor_exit(ptr);
		return 1;
	}

	// start counting the votes
	int granted_votes = 0;
	if (result->granted == true) {
		++granted_votes;
	}
	RAFT_REQUEST_VOTE_RESULT vote;
	unsigned long vote_seq;
	for (vote_seq = result->candidate_vote_pool_seqno + 1; vote_seq < seq_no; ++vote_seq) {
		if (WooFGet(RAFT_REQUEST_VOTE_RESULT_WOOF, &vote, vote_seq) < 0) {
			log_error("failed to get the vote result at seqno %lu", vote_seq);
			free(result);
			exit(1);
		}
		if (vote.granted == true && vote.term == result->term) {
			++granted_votes;
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
			log_error("failed to promote itself to leader");
			free(result);
			exit(1);
		}
		log_info("promoted to leader for term %lu", result->term);

		// start replicate_entries handlers
		unsigned long last_log_entry_seqno = WooFGetLatestSeqno(RAFT_LOG_ENTRIES_WOOF);
		if (WooFInvalid(last_log_entry_seqno)) {
			log_error("failed to get the latest seqno from %s", RAFT_LOG_ENTRIES_WOOF);
			free(result);
			exit(1);
		}
		unsigned long last_append_result_seqno = WooFGetLatestSeqno(RAFT_APPEND_ENTRIES_RESULT_WOOF);
		if (WooFInvalid(last_append_result_seqno)) {
			log_error("failed to get the latest seqno from %s", RAFT_APPEND_ENTRIES_RESULT_WOOF);
			free(result);
			exit(1);
		}
		RAFT_REPLICATE_ENTRIES_ARG replicate_entries_arg;
		replicate_entries_arg.term = server_state.current_term;
		replicate_entries_arg.last_seen_result_seqno = last_append_result_seqno;
		int i;
		for (i = 0; i < server_state.members; ++i) {
			replicate_entries_arg.next_index[i] = last_log_entry_seqno + 1;
			replicate_entries_arg.match_index[i] = 0;
			replicate_entries_arg.last_sent_timestamp[i] = 0;
		}
		seq = monitor_put(RAFT_MONITOR_NAME, RAFT_REPLICATE_ENTRIES_WOOF, "h_replicate_entries", &replicate_entries_arg);
		if (WooFInvalid(seq)) {
			log_error("failed to start h_replicate_entries handler");
			free(result);
			exit(1);
		}
	}

	free(result);
	monitor_exit(ptr);
	return 1;
}
