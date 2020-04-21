#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "woofc.h"
#include "raft.h"
#include "monitor.h"

int h_request_vote(WOOF *wf, unsigned long seq_no, void *ptr) {
	RAFT_REQUEST_VOTE_ARG *request = (RAFT_REQUEST_VOTE_ARG *)monitor_cast(ptr);

	log_set_tag("request_vote");
	// log_set_level(LOG_INFO);
	log_set_level(LOG_DEBUG);
	log_set_output(stdout);

	// get the server's current term
	unsigned long last_server_state = WooFGetLatestSeqno(RAFT_SERVER_STATE_WOOF);
	if (WooFInvalid(last_server_state)) {
		log_error("failed to get the latest seqno from %s", RAFT_SERVER_STATE_WOOF);
		free(request);
		exit(1);
	}
	RAFT_SERVER_STATE server_state;
	int err = WooFGet(RAFT_SERVER_STATE_WOOF, &server_state, last_server_state);
	if (err < 0) {
		log_error("failed to get the server state");
		free(request);
		exit(1);
	}

	unsigned long i;
	RAFT_REQUEST_VOTE_RESULT result;
	result.candidate_vote_pool_seqno = request->candidate_vote_pool_seqno;
	// if not a member, deny the request and tell it to shut down
	if (member_id(server_state.members, request->candidate_woof, server_state.member_woofs) == -1) {
		result.term = 0; // result term 0 means shutdown
		result.granted = false;
		log_debug("rejected a vote from a candidate not in the config anymore");
	} else if (request->term < server_state.current_term) { // current term is higher than the request
		result.term = server_state.current_term; // server term will always be greater than reviewing term
		result.granted = false;
		log_debug("rejected a vote from lower term %lu at term %lu", request->term, server_state.current_term);
	} else {
		if (request->term > server_state.current_term) {
			// fallback to follower
			log_debug("request term %lu is higher, fall back to follower", request->term);
			server_state.current_term = request->term;
			server_state.role = RAFT_FOLLOWER;
			strcpy(server_state.current_leader, request->candidate_woof);
			memset(server_state.voted_for, 0, RAFT_WOOF_NAME_LENGTH);
			unsigned long seq = WooFPut(RAFT_SERVER_STATE_WOOF, NULL, &server_state);
			if (WooFInvalid(seq)) {
				log_error("failed to fall back to follower at term %lu", request->term);
				free(request);
				exit(1);
			}
			log_info("state changed at term %lu: FOLLOWER", server_state.current_term);
			RAFT_TIMEOUT_CHECKER_ARG timeout_checker_arg;
			timeout_checker_arg.timeout_value = random_timeout(get_milliseconds());
			seq = monitor_put(RAFT_MONITOR_NAME, RAFT_TIMEOUT_CHECKER_WOOF, "h_timeout_checker", &timeout_checker_arg);
			if (WooFInvalid(seq)) {
				log_error("failed to start the timeout checker");
				exit(1);
				free(request);
			}
		}
		
		result.term = server_state.current_term;
		if (server_state.voted_for[0] == 0 || strcmp(server_state.voted_for, request->candidate_woof) == 0) {
			// check if the log is up-to-date
			unsigned long latest_log_entry = WooFGetLatestSeqno(RAFT_LOG_ENTRIES_WOOF);	
			if (WooFInvalid(latest_log_entry)) {
				log_error("failed to get the latest seqno from %s", RAFT_LOG_ENTRIES_WOOF);
				free(request);
				exit(1);
			}
			RAFT_LOG_ENTRY last_log_entry;
			if (latest_log_entry > 0) {
				if (WooFGet(RAFT_LOG_ENTRIES_WOOF, &last_log_entry, latest_log_entry) < 0) {
					log_error("failed to get the latest log entry %lu from %s", latest_log_entry, RAFT_LOG_ENTRIES_WOOF);
					free(request);
					exit(1);
				}
			}
			if (latest_log_entry > 0 && last_log_entry.term > request->last_log_term) {
				// the server has more up-to-dated entries than the candidate
				result.granted = false;
				log_debug("rejected vote from server with outdated log (last entry at term %lu)", request->last_log_term);
			} else if (latest_log_entry > 0 && last_log_entry.term == request->last_log_term && latest_log_entry > request->last_log_index) {
				// both have same term but the server has more entries
				result.granted = false;
				log_debug("rejected vote from server with outdated log (last entry at index %lu)", request->last_log_index);
			} else {
				// the candidate has more up-to-dated log entries
				memcpy(server_state.voted_for, request->candidate_woof, RAFT_WOOF_NAME_LENGTH);
				unsigned long seq = WooFPut(RAFT_SERVER_STATE_WOOF, NULL, &server_state);
				if (WooFInvalid(seq)) {
					log_error("failed to update voted_for at term %lu", server_state.current_term);
					free(request);
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
	char candidate_monitor[RAFT_WOOF_NAME_LENGTH];
	char candidate_result_woof[RAFT_WOOF_NAME_LENGTH];
	sprintf(candidate_monitor, "%s/%s", request->candidate_woof, RAFT_MONITOR_NAME);
	sprintf(candidate_result_woof, "%s/%s", request->candidate_woof, RAFT_REQUEST_VOTE_RESULT_WOOF);
	unsigned long seq = monitor_remote_put(candidate_monitor, candidate_result_woof, "h_count_vote", &result);
	if (WooFInvalid(seq)) {
		log_error("failed to return the vote result to %s", candidate_result_woof);
		free(request);
		exit(1);
	}

	free(request);
	monitor_exit(ptr);
	return 1;

}
