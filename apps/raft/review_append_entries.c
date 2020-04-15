#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "woofc.h"
#include "raft.h"

int review_append_entries(WOOF *wf, unsigned long seq_no, void *ptr) {
	RAFT_FUNCTION_LOOP *function_loop = (RAFT_FUNCTION_LOOP *)ptr;

	log_set_tag("review_append_entries");
	// log_set_level(LOG_INFO);
	log_set_level(LOG_DEBUG);
	log_set_output(stdout);

	unsigned long latest_append_request = WooFGetLatestSeqno(RAFT_APPEND_ENTRIES_ARG_WOOF);
	if (WooFInvalid(latest_append_request)) {
		log_error("couldn't get the latest seqno from %s", RAFT_APPEND_ENTRIES_ARG_WOOF);
		exit(1);
	}
	if (function_loop->last_reviewed_append_entries < latest_append_request) {
		// get the server's current term
		unsigned long last_server_state = WooFGetLatestSeqno(RAFT_SERVER_STATE_WOOF);
		if (WooFInvalid(last_server_state)) {
			log_error("couldn't get the latest seqno from %s", RAFT_SERVER_STATE_WOOF);
			exit(1);
		}
		RAFT_SERVER_STATE server_state;
		int err = WooFGet(RAFT_SERVER_STATE_WOOF, &server_state, last_server_state);
		if (err < 0) {
			log_error("couldn't get the server's latest state");
			exit(1);
		}

		unsigned long i;
		RAFT_APPEND_ENTRIES_ARG request;
		RAFT_APPEND_ENTRIES_RESULT result;
		memcpy(result.server_woof, server_state.woof_name, RAFT_WOOF_NAME_LENGTH);
		for (i = function_loop->last_reviewed_append_entries + 1; i <= latest_append_request; ++i) {
			// read the request
			err = WooFGet(RAFT_APPEND_ENTRIES_ARG_WOOF, &request, i);
			if (err < 0) {
				log_error("couldn't get the request at %lu", i);
				exit(1);
			}
			if (get_milliseconds() - request.created_ts > RAFT_TIMEOUT_MIN) {
				log_warn("request %lu took %lums to process", i, get_milliseconds() - request.created_ts);
			}
			if (request.term > server_state.current_term) {
				// increment the current term
				RAFT_TERM_ENTRY new_term;
				new_term.term = request.term;
				new_term.role = RAFT_FOLLOWER;
				memcpy(new_term.leader, request.leader_woof, RAFT_WOOF_NAME_LENGTH);
				unsigned long seq = WooFPut(RAFT_TERM_ENTRIES_WOOF, NULL, &new_term);
				if (WooFInvalid(seq)) {
					log_error("couldn't queue the new term request to chair");
					exit(1);
				}
				log_debug("request term %lu is higher than server's term %lu, fall back to follower", request.term, server_state.current_term);

				function_loop->last_reviewed_append_entries = i - 1;
				sprintf(function_loop->next_invoking, "review_request_vote");
				seq = WooFPut(RAFT_FUNCTION_LOOP_WOOF, "review_request_vote", function_loop);
				if (WooFInvalid(seq)) {
					log_error("couldn't queue the next function_loop: review_request_vote");
					exit(1);
				}
				return 1;
			} else if (request.term < server_state.current_term) {
				// fail the request from lower term
				result.term = server_state.current_term;
				result.success = false;
				// result.next_index doesn't matter
			} else {
				// it's a valid append_entries request, treat as a heartbeat from leader
				RAFT_HEARTBEAT heartbeat;
				unsigned long seq = WooFPut(RAFT_HEARTBEAT_WOOF, "timeout_checker", &heartbeat);
				if (WooFInvalid(seq)) {
					log_error("couldn't put a new heartbeat from term %lu", request.term);
					exit(1);
				}

				// if it's a request without entry, it's just a heartbeat
				if (request.entries[0].term == 0) {
					function_loop->last_reviewed_append_entries = i;
					continue; // no need to return
				}

				// check if the previouse log matches with the leader
				unsigned long latest_entry_seqno = WooFGetLatestSeqno(RAFT_LOG_ENTRIES_WOOF);
				if (latest_entry_seqno < request.prev_log_index) {
					log_debug("no log entry exists at prev_log_index %lu, latest: %lu", request.prev_log_index, latest_entry_seqno);
					result.term = request.term;
					result.success = false;
					result.next_index = request.prev_log_index; // decrement next_index
				} else {
					// read the previous log entry
					RAFT_LOG_ENTRY entry;
					if (request.prev_log_index > 0) {
						err = WooFGet(RAFT_LOG_ENTRIES_WOOF, &entry, request.prev_log_index);
						if (err < 0) {
							log_error("couldn't get log entry at prev_log_index %lu", request.prev_log_index);
							exit(1);
						}
					}
					// term doesn't match
					if (request.prev_log_index > 0 && entry.term != request.prev_log_term) {
						log_debug("previous log entry at prev_log_index %lu doesn't match request.prev_log_term %lu: %lu [%lu]", request.prev_log_index, request.prev_log_term, entry.term, i);
						result.term = request.term;
						result.success = false;
						result.next_index = request.prev_log_index; // decrement next_index
					} else {
						// if the server has more entries that conflict with the leader, delete them	
						if (latest_entry_seqno > request.prev_log_index) {
							if (request.prev_log_index < server_state.commit_index) {
								log_error("can't delete committed entries, prev_log_index: %lu, commit_index: %lu, fatal error", request.prev_log_index, server_state.commit_index);
								exit(1);
							}
							// TODO: check if the entries after prev_log_index match
							// if so, we don't need to call WooFTruncate()
							err = WooFTruncate(RAFT_LOG_ENTRIES_WOOF, request.prev_log_index);
							if (err < 0) {
								log_error("couldn't truncate log entries woof");
								exit(1);
							}
							log_debug("log truncated to %lu", request.prev_log_index);
						}
						// appending entries
						unsigned long seq;
						int j;
						for (j = 0; j < RAFT_MAX_ENTRIES_PER_REQUEST; j++) {
							if (request.entries[j].term == 0) {
								break; // no entry, finish append
							}
							seq = WooFPut(RAFT_LOG_ENTRIES_WOOF, NULL, &request.entries[j]);
							if (WooFInvalid(seq)) {
								log_error("couldn't append entries[%d]", j);
								exit(1);
							}
							// if this entry is a config entry, update server config
							if (request.entries[j].is_config == true) {
								int new_members;
								char new_member_woofs[RAFT_MAX_SERVER_NUMBER][RAFT_WOOF_NAME_LENGTH];
								err = decode_config(request.entries[j].data, &new_members, new_member_woofs);
								if (err < 0) {
									log_error("couldn't decode config from entry[%lu]", j);
								}
								server_state.members = new_members;
								memcpy(server_state.member_woofs, new_member_woofs, RAFT_MAX_SERVER_NUMBER * RAFT_WOOF_NAME_LENGTH);
								unsigned long seq = WooFPut(RAFT_SERVER_STATE_WOOF, NULL, &server_state);
								if (WooFInvalid(seq)) {
									log_error("couldn't update server config at term %lu", server_state.current_term);
									exit(1);
								}
								log_debug("updated server config to %d members at term %lu", server_state.members, server_state.current_term);
							}
						}
						result.term = request.term;
						result.success = true;
						result.next_index = seq + 1;
						if (j > 0) {
							log_debug("appended %d entries for request [%lu], result.next_index %lu", j, i, result.next_index);
						}

						// check if there's new commit_index
						if (request.leader_commit > server_state.commit_index) {
							// commit_index = min(leader_commit, index of last new entry)
							server_state.commit_index = request.leader_commit;
							if (seq < server_state.commit_index) {
								server_state.commit_index = seq;
								unsigned long seq = WooFPut(RAFT_SERVER_STATE_WOOF, NULL, &server_state);
								if (WooFInvalid(seq)) {
									log_error("couldn't update commit_index to %lu at term %lu", server_state.commit_index, server_state.current_term);
									exit(1);
								}
								log_debug("updated commit_index to %lu at term %lu", server_state.commit_index, server_state.current_term);
							}
						}
					}
				}
			}
			
			// return the request
			char leader_result_woof[RAFT_WOOF_NAME_LENGTH];
			sprintf(leader_result_woof, "%s/%s", request.leader_woof, RAFT_APPEND_ENTRIES_RESULT_WOOF);
			unsigned long seq = WooFPut(leader_result_woof, "append_entries_result", &result);
			if (WooFInvalid(seq)) {
				log_error("couldn't return the append_entries result to %s", leader_result_woof);
				exit(1);
			}
			function_loop->last_reviewed_append_entries = i;
		}
	}

	// queue the next review function
	usleep(RAFT_FUNCTION_LOOP_DELAY * 1000);
	unsigned long seq = WooFPut(RAFT_FUNCTION_LOOP_WOOF, "review_request_vote", function_loop);
	if (WooFInvalid(seq)) {
		log_error("couldn't queue the next function_loop: review_request_vote");
		exit(1);
	}
	return 1;

}
