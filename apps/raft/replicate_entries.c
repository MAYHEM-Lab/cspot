#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "woofc.h"
#include "raft.h"

char function_tag[] = "replicate_entries";

int replicate_entries(WOOF *wf, unsigned long seq_no, void *ptr)
{
	RAFT_REPLICATE_ENTRIES_ARG *arg = (RAFT_REPLICATE_ENTRIES_ARG *)ptr;

	// log_set_level(LOG_INFO);
	log_set_level(LOG_DEBUG);
	log_set_output(stdout);

	// get the server's current term and cluster members
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

	// if it's not leader, wait for term chair for promotion
	if (server_state.role != RAFT_LEADER) {
		log_debug(function_tag, "not a leader, break the loop");
		return 1;
	}

	// get the latest log entry seqno
	unsigned long last_log_index = WooFGetLatestSeqno(RAFT_LOG_ENTRIES_WOOF);
	if (WooFInvalid(last_log_index)) {
		sprintf(log_msg, "couldn't get the latest seqno from %s", RAFT_LOG_ENTRIES_WOOF);
		log_error(function_tag, log_msg);
		exit(1);
	}

	// get next_index of members
	unsigned long latest_next_index = WooFGetLatestSeqno(RAFT_NEXT_INDEX_WOOF);
	if (WooFInvalid(latest_next_index)) {
		sprintf(log_msg, "couldn't get the latest seqno from %s", RAFT_NEXT_INDEX_WOOF);
		log_error(function_tag, log_msg);
		exit(1);
	}
	if (arg->last_seen_seqno < latest_next_index) {
		sprintf(log_msg, "reviewing from %lu to %lu", arg->last_seen_seqno + 1, latest_next_index);
		log_debug(function_tag, log_msg);
		// scan to see what the latest next_indexes are for each member
		unsigned long next_indexes[server_state.members];
		memset(next_indexes, 0, sizeof(unsigned long) * server_state.members);
		unsigned long min_seqno_to_send = last_log_index + 1;
		unsigned long i;
		int j;
		for (i = arg->last_seen_seqno + 1; i <= latest_next_index; i++) {
			RAFT_NEXT_INDEX entry;
			err = WooFGet(RAFT_NEXT_INDEX_WOOF, &entry, i);
			if (err < 0) {
				sprintf(log_msg, "couldn't get next_index entry at %lu", i);
				log_error(function_tag, log_msg);
				exit(1);
			}
			if (entry.term != arg->term) {
				// not for this term
				continue;
			}
			for (j = 0; j < server_state.members; j++) {
				if (memcmp(server_state.member_woofs[j], server_state.woof_name, RAFT_WOOF_NAME_LENGTH) == 0) {
					// no need to replicate to itself
					continue;
				}
				if (entry.next_index[j] > next_indexes[j]) {
					next_indexes[j] = entry.next_index[j];
					// min_seqno_to_send = min(min_seqno_to_send, next_indexes[j]);
					if (next_indexes[j] < min_seqno_to_send) {
						min_seqno_to_send = next_indexes[j];
					}
				}
			}
		}

		// read all the entries to be sent to members, +1 for the previous entry
		// int num_entries = min(last_log_index - min_seqno_to_send + 1, RAFT_MAX_ENTRIES_PER_REQUEST);
		int num_entries = RAFT_MAX_ENTRIES_PER_REQUEST;
		if (last_log_index - min_seqno_to_send + 1 < num_entries) {
			num_entries = last_log_index - min_seqno_to_send + 1;
		}
		RAFT_LOG_ENTRY *entries = malloc(sizeof(RAFT_LOG_ENTRY) * num_entries);
		for (j = 0; j < num_entries + 1; j++) {
			err = WooFGet(RAFT_LOG_ENTRIES_WOOF, &entries[j], min_seqno_to_send - 1 + j);
			if (err < 0) {
				sprintf(log_msg, "couldn't get the log entries at %lu", min_seqno_to_send - 1 + j);
				log_error(function_tag, log_msg);
				exit(1);
			}
		}
		for (j = 0; j < server_state.members; j++) {
			if (memcmp(server_state.member_woofs[j], server_state.woof_name, RAFT_WOOF_NAME_LENGTH) == 0) {
				// no need to replicate to itself
				continue;
			}
			if (next_indexes[j] > 0) { // there are some log entries to send
				unsigned long num_entries_to_send = last_log_index - next_indexes[j] + 1;
				if (num_entries_to_send > RAFT_MAX_ENTRIES_PER_REQUEST) {
					num_entries_to_send = RAFT_MAX_ENTRIES_PER_REQUEST;
				}
				RAFT_APPEND_ENTRIES_ARG append_entries_arg;
				append_entries_arg.term = arg->term;
				memcpy(append_entries_arg.leader_woof, server_state.woof_name, RAFT_WOOF_NAME_LENGTH);
				append_entries_arg.prev_log_index = next_indexes[j] - 1;
				int first_entry_index = next_indexes[j] - min_seqno_to_send + 1;
				append_entries_arg.prev_log_term = entries[first_entry_index - 1].term;
				memcpy(append_entries_arg.entries, entries, sizeof(RAFT_LOG_ENTRY) * num_entries_to_send);
				// TODO: append_entries_arg.leaderCommit

				// send append_entries request
				char member_woof[RAFT_WOOF_NAME_LENGTH];
				sprintf(member_woof, "%s/%s", server_state.member_woofs[j], RAFT_APPEND_ENTRIES_ARG_WOOF);
				unsigned long seq = WooFPut(member_woof, NULL, &append_entries_arg);
				if (WooFInvalid(seq)) {
					sprintf(log_msg, "couldn't send append_entries request with %lu entries to %s", num_entries_to_send, member_woof);
					log_error(function_tag, log_msg);
					exit(1);
				}
				// update last timestamp it send request to member
				arg->last_timestamp[j] = get_milliseconds();
			}
		}
		free(entries);
	}

	int i;
	for (i = 0; i < server_state.members; i++) {
		if (memcmp(server_state.member_woofs[i], server_state.woof_name, RAFT_WOOF_NAME_LENGTH) == 0) {
			// no need to send heartbeat to itself
			continue;
		}
		if (get_milliseconds() - arg->last_timestamp[i] > RAFT_HEARTBEAT_RATE) {
			// didn't send any entry and timeout occurs, send heartbeat
			RAFT_APPEND_ENTRIES_ARG append_entries_arg;
			append_entries_arg.term = arg->term;
			memcpy(append_entries_arg.leader_woof, server_state.woof_name, RAFT_WOOF_NAME_LENGTH);
			memset(append_entries_arg.entries, 0, sizeof(RAFT_LOG_ENTRY) * RAFT_MAX_ENTRIES_PER_REQUEST);
			append_entries_arg.created_ts = get_milliseconds();
			// nothing else matters

			// send append_entries request
			char member_woof[RAFT_WOOF_NAME_LENGTH];
			sprintf(member_woof, "%s/%s", server_state.member_woofs[i], RAFT_APPEND_ENTRIES_ARG_WOOF);
			unsigned long seq = WooFPut(member_woof, NULL, &append_entries_arg);
			if (WooFInvalid(seq)) {
				sprintf(log_msg, "couldn't send empty append_entries request to %s", member_woof);
				log_error(function_tag, log_msg);
				exit(1);
			}
			// update last timestamp it send request to member
			arg->last_timestamp[i] = get_milliseconds();
		}
	}
	
	arg->last_seen_seqno = latest_next_index;
	// queue the next replicate_entries function
	usleep(RAFT_LOOP_RATE * 1000);
	unsigned long seq = WooFPut(RAFT_REPLICATE_ENTRIES_WOOF, "replicate_entries", arg);
	if (WooFInvalid(seq)) {
		log_error(function_tag, "couldn't queue the next replicate_entries function");
		exit(1);
	}
	return 1;

}
