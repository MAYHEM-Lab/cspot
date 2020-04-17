#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "woofc.h"
#include "raft.h"
#include "monitor.h"

int h_append_entries(WOOF *wf, unsigned long seq_no, void *ptr) {
	RAFT_APPEND_ENTRIES_ARG *request = (RAFT_APPEND_ENTRIES_ARG *)ptr;

	log_set_tag("append_entries");
	// log_set_level(LOG_INFO);
	log_set_level(LOG_DEBUG);
	log_set_output(stdout);

	// get the server's current term
	unsigned long last_server_state = WooFGetLatestSeqno(RAFT_SERVER_STATE_WOOF);
	if (WooFInvalid(last_server_state)) {
		log_error("couldn't get the latest seqno from %s", RAFT_SERVER_STATE_WOOF);
		exit(1);
	}
	RAFT_SERVER_STATE server_state;
	if (WooFGet(RAFT_SERVER_STATE_WOOF, &server_state, last_server_state) < 0) {
		log_error("couldn't get the server's latest state");
		exit(1);
	}

	RAFT_APPEND_ENTRIES_RESULT result;
	memcpy(result.server_woof, server_state.woof_name, RAFT_WOOF_NAME_LENGTH);
	if (get_milliseconds() - request->created_ts > RAFT_HEARTBEAT_RATE) {
		log_warn("request %lu took %lums to process", seq_no, get_milliseconds() - request->created_ts);
	}
	if (request->term < server_state.current_term) {
		// fail the request from lower term
		result.term = server_state.current_term;
		result.success = false;
		// result.next_index doesn't matter
	} else {
		if (request->term > server_state.current_term) {
			// fall back to follower
			log_debug("request term %lu is higher than server's term %lu, fall back to follower", request->term, server_state.current_term);
			server_state.current_term = request->term;
			server_state.role = RAFT_FOLLOWER;
			memcpy(server_state.current_leader, request->leader_woof, RAFT_WOOF_NAME_LENGTH);
			memset(server_state.voted_for, 0, RAFT_WOOF_NAME_LENGTH);
			unsigned long seq = WooFPut(RAFT_SERVER_STATE_WOOF, NULL, &server_state);
			if (WooFInvalid(seq)) {
				log_error("couldn't fall back to follower at term %lu", request->term);
				exit(1);
			}
		} 
		// it's a valid append_entries request, treat as a heartbeat from leader
		RAFT_HEARTBEAT heartbeat;
		unsigned long seq = WooFPut(RAFT_HEARTBEAT_WOOF, "timeout_checker", &heartbeat);
		if (WooFInvalid(seq)) {
			log_error("couldn't put a new heartbeat from term %lu", request->term);
			exit(1);
		}
		log_debug("received a heartbeat (%lu)", seq_no);

		// if it's a request without entry, it's just a heartbeat
		if (request->entries[0].term == 0) {
			monitor_exit(wf, seq_no);
			return 1;
		}

		// check if the previous log matches with the leader
		unsigned long latest_entry_seqno = WooFGetLatestSeqno(RAFT_LOG_ENTRIES_WOOF);
		if (latest_entry_seqno < request->prev_log_index) {
			log_debug("no log entry exists at prev_log_index %lu, latest: %lu", request->prev_log_index, latest_entry_seqno);
			result.term = request->term;
			result.success = false;
			result.next_index = request->prev_log_index; // decrement next_index
		} else {
			// read the previous log entry
			RAFT_LOG_ENTRY entry;
			if (request->prev_log_index > 0) {
				if (WooFGet(RAFT_LOG_ENTRIES_WOOF, &entry, request->prev_log_index) < 0) {
					log_error("couldn't get log entry at prev_log_index %lu", request->prev_log_index);
					exit(1);
				}
			}
			// term doesn't match
			if (request->prev_log_index > 0 && entry.term != request->prev_log_term) {
				log_debug("previous log entry at prev_log_index %lu doesn't match request->prev_log_term %lu: %lu [%lu]", request->prev_log_index, request->prev_log_term, entry.term, seq_no);
				result.term = request->term;
				result.success = false;
				result.next_index = request->prev_log_index; // decrement next_index
			} else {
				// if the server has more entries that conflict with the leader, delete them	
				if (latest_entry_seqno > request->prev_log_index) {
					if (request->prev_log_index < server_state.commit_index) {
						log_error("can't delete committed entries, prev_log_index: %lu, commit_index: %lu, fatal error", request->prev_log_index, server_state.commit_index);
						exit(1);
					}
					// TODO: check if the entries after prev_log_index match
					// if so, we don't need to call WooFTruncate()
					if (WooFTruncate(RAFT_LOG_ENTRIES_WOOF, request->prev_log_index) < 0) {
						log_error("couldn't truncate log entries woof");
						exit(1);
					}
					log_debug("log truncated to %lu", request->prev_log_index);
				}
				// appending entries
				unsigned long entry_seq;
				int i;
				for (i = 0; i < RAFT_MAX_ENTRIES_PER_REQUEST; ++i) {
					if (request->entries[i].term == 0) {
						break; // no entry, finish append
					}
					entry_seq = WooFPut(RAFT_LOG_ENTRIES_WOOF, NULL, &request->entries[i]);
					if (WooFInvalid(entry_seq)) {
						log_error("couldn't append entries[%d]", i);
						exit(1);
					}
					// if this entry is a config entry, update server config
					if (request->entries[i].is_config == true) {
						int new_members;
						char new_member_woofs[RAFT_MAX_SERVER_NUMBER][RAFT_WOOF_NAME_LENGTH];
						if (decode_config(request->entries[i].data.val, &new_members, new_member_woofs) < 0) {
							log_error("couldn't decode config from entry[%lu]", i);
						}
						server_state.members = new_members;
						memcpy(server_state.member_woofs, new_member_woofs, RAFT_MAX_SERVER_NUMBER * RAFT_WOOF_NAME_LENGTH);
						unsigned long server_state_seq = WooFPut(RAFT_SERVER_STATE_WOOF, NULL, &server_state);
						if (WooFInvalid(server_state_seq)) {
							log_error("couldn't update server config at term %lu", server_state.current_term);
							exit(1);
						}
						log_debug("updated server config to %d members at term %lu", server_state.members, server_state.current_term);
					}
				}
				result.term = request->term;
				result.success = true;
				result.next_index = seq + 1;
				if (i > 0) {
					log_debug("appended %d entries for request [%lu], result.next_index %lu", i, seq_no, result.next_index);
				}

				// check if there's new commit_index
				if (request->leader_commit > server_state.commit_index) {
					// commit_index = min(leader_commit, index of last new entry)
					server_state.commit_index = request->leader_commit;
					if (entry_seq < server_state.commit_index) {
						server_state.commit_index = entry_seq;
					}
					unsigned long server_state_seq = WooFPut(RAFT_SERVER_STATE_WOOF, NULL, &server_state);
					if (WooFInvalid(server_state_seq)) {
						log_error("couldn't update commit_index to %lu at term %lu", server_state.commit_index, server_state.current_term);
						exit(1);
					}
					log_debug("updated commit_index to %lu at term %lu", server_state.commit_index, server_state.current_term);
				}
			}
		}
	}
		
	// return the request
	char leader_result_woof[RAFT_WOOF_NAME_LENGTH];
	sprintf(leader_result_woof, "%s/%s", request->leader_woof, RAFT_APPEND_ENTRIES_RESULT_WOOF);
	unsigned long seq = WooFPut(leader_result_woof, "append_entries_result", &result);
	if (WooFInvalid(seq)) {
		log_error("couldn't return the append_entries result to %s", leader_result_woof);
		exit(1);
	}

	monitor_exit(wf, seq_no);
	return 1;
}
