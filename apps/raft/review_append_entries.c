#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "woofc.h"
#include "raft.h"

char function_tag[] = "review_append_entries";

int review_append_entries(WOOF *wf, unsigned long seq_no, void *ptr)
{
	RAFT_REVIEW_APPEND_ENTRIES_ARG *reviewing = (RAFT_REVIEW_APPEND_ENTRIES_ARG *)ptr;

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
		log_error(function_tag, "couldn't get the server's latest state");
		exit(1);
	}

	unsigned long latest_append_request = WooFGetLatestSeqno(RAFT_APPEND_ENTRIES_ARG_WOOF);
	if (WooFInvalid(latest_append_request)) {
		sprintf(log_msg, "couldn't get the latest seqno from %s", RAFT_APPEND_ENTRIES_ARG_WOOF);
		log_error(function_tag, log_msg);
		exit(1);
	}
	
	unsigned long i;
	RAFT_APPEND_ENTRIES_ARG request;
	RAFT_APPEND_ENTRIES_RESULT result;
	memcpy(result.server_woof, server_state.woof_name, RAFT_WOOF_NAME_LENGTH);
	for (i = reviewing->last_request_seqno + 1; i <= latest_append_request; ++i) {
		// read the request
		err = WooFGet(RAFT_APPEND_ENTRIES_ARG_WOOF, &request, i);
		if (err < 0) {
			sprintf(log_msg, "couldn't get the request at %lu", i);
			log_error(function_tag, log_msg);
			exit(1);
		}
		if (get_milliseconds() - request.created_ts > RAFT_TIMEOUT_MIN) {
			sprintf(log_msg, "request %lu took %lums to process", i, get_milliseconds() - request.created_ts);
			log_warn(function_tag, log_msg);
		}
		if (request.term > server_state.current_term) {
			// increment the current term
			RAFT_TERM_ENTRY new_term;
			new_term.term = request.term;
			new_term.role = RAFT_FOLLOWER;
			unsigned long seq = WooFPut(RAFT_TERM_ENTRIES_WOOF, NULL, &new_term);
			if (WooFInvalid(seq)) {
				log_error(function_tag, "couldn't queue the new term request to chair");
				exit(1);
			}
			sprintf(log_msg, "request term %lu is higher than server's term %lu, fall back to follower", request.term, server_state.current_term);
			log_debug(function_tag, log_msg);

			reviewing->last_request_seqno = i - 1;
			seq = WooFPut(RAFT_REVIEW_REQUEST_VOTE_ARG_WOOF, "review_append_entries", reviewing);
			if (WooFInvalid(seq)) {
				log_error(function_tag, "couldn't queue the next review function");
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
			RAFT_HEARTBEAT_ARG heartbeat_arg;
			heartbeat_arg.term = request.term;
			unsigned long seq = WooFPut(RAFT_HEARTBEAT_ARG_WOOF, "timeout_checker", &heartbeat_arg);
			if (WooFInvalid(seq)) {
				sprintf(log_msg, "couldn't put a new heartbeat from term %lu", request.term);
				log_error(function_tag, log_msg);
				exit(1);
			}

			// if it's a request without entry, it's just a heartbeat
			if (request.entries[0].term == 0) {
				// no need to return
				continue;
			}

			// check if the previouse log matches with the leader
			unsigned long latest_entry_seqno = WooFGetLatestSeqno(RAFT_LOG_ENTRIES_WOOF);
			if (latest_entry_seqno < request.prev_log_index) {
				sprintf(log_msg, "no log entry exists at prevLogIndex %lu, latest: %lu", request.prev_log_index, latest_entry_seqno);
				log_debug(function_tag, log_msg);
				result.term = request.term;
				result.success = false;
				// decrement next_index
				result.next_index = request.prev_log_index;
			} else {
				// read the previous log entry
				RAFT_LOG_ENTRY entry;
				err = WooFGet(RAFT_LOG_ENTRIES_WOOF, &entry, request.prev_log_index);
				if (err < 0) {
					sprintf(log_msg, "couldn't get log entry at prevLogIndex %lu", request.prev_log_index);
					log_error(function_tag, log_msg);
					exit(1);
				}
				// term doesn't match
				if (entry.term != request.prev_log_term) {
					sprintf(log_msg, "previous log entry at prevLogIndex %lu doesn't match request.prevLogTerm %lu: %lu", request.prev_log_index, request.prev_log_term, entry.term);
					log_debug(function_tag, log_msg);
					result.term = request.term;
					result.success = false;
					// decrement next_index
					result.next_index = request.prev_log_index;
				} else {
					// if the server has more entries that conflict with the leader, delete them	
					if (latest_entry_seqno > request.prev_log_index) {
						err = WooFTruncate(RAFT_LOG_ENTRIES_WOOF, request.prev_log_index);
						if (err < 0) {
							log_error(function_tag, "couldn't truncate log entries woof");
							exit(1);
						}
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
							sprintf(log_msg, "couldn't append entries[%d]", j);
							log_error(function_tag, log_msg);
							exit(1);
						}
					}
					if (j > 0) {
						sprintf(log_msg, "appended %d entries", j);
						log_debug(function_tag, log_msg);
					}

					// TODO: check commit index
					result.term = request.term;
					result.success = true;
					result.next_index = seq + 1;
					
					// // check if it's still in the same term,
					// // to avoid timeout happen and the server request_vote without using the latest last_log_index and term
					// last_server_state = WooFGetLatestSeqno(RAFT_SERVER_STATE_WOOF);
					// if (WooFInvalid(last_server_state)) {
					// 	sprintf(log_msg, "couldn't get the latest seqno from %s", RAFT_SERVER_STATE_WOOF);
					// 	log_error(function_tag, log_msg);
					// 	exit(1);
					// }
					// err = WooFGet(RAFT_SERVER_STATE_WOOF, &server_state, last_server_state);
					// if (err < 0) {
					// 	log_error(function_tag, "couldn't get the server's latest state");
					// 	exit(1);
					// }
					// if (server_state.current_term != request.term) {
					// 	// reply false to stop leader from commiting the entry
					// 	result.term = server_state.current_term;
					// 	result.success = false;
					// 	result.next_index = request.prev_log_index;
					// }
				}
			}
		}
		
		// return the request
		char leader_result_woof[RAFT_WOOF_NAME_LENGTH];
		sprintf(leader_result_woof, "%s/%s", request.leader_woof, RAFT_APPEND_ENTRIES_RESULT_WOOF);
		unsigned long seq = WooFPut(leader_result_woof, "append_entries_result", &result);
		if (WooFInvalid(seq)) {
			sprintf(log_msg, "couldn't return the append_entries result to %s", leader_result_woof);
			log_error(function_tag, log_msg);
			exit(1);
		}
	}
	// queue the next review function
	usleep(RAFT_LOOP_RATE * 1000);
	reviewing->last_request_seqno = latest_append_request;
	unsigned long seq = WooFPut(RAFT_REVIEW_APPEND_ENTRIES_ARG_WOOF, "review_append_entries", reviewing);
	if (WooFInvalid(seq)) {
		log_error(function_tag, "couldn't queue the next review function");
		exit(1);
	}
	return 1;

}
