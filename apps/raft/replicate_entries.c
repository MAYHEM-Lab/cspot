#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#include "woofc.h"
#include "raft.h"

char function_tag[] = "replicate_entries";

typedef struct append_entries_thread_arg {
	int id;
	int num_entries_to_send;
	char member_woof[RAFT_WOOF_NAME_LENGTH];
	RAFT_APPEND_ENTRIES_ARG arg;
} APPEND_ENTRIES_THREAD_ARG;

void *append_entries(void *arg) {
	APPEND_ENTRIES_THREAD_ARG *thread_arg = (APPEND_ENTRIES_THREAD_ARG *)arg;
	unsigned long seq = WooFPut(thread_arg->member_woof, NULL, &(thread_arg->arg));
	if (WooFInvalid(seq)) {
		sprintf(log_msg, "couldn't send append_entries request to %s", thread_arg->member_woof);
		log_error(function_tag, log_msg);
	}
	if (thread_arg->arg.entries[0].term == 0) {
		// sprintf(log_msg, "sent heartbeat (%lu) to %s", seq, thread_arg->member_woof);
		// log_debug(function_tag, log_msg);
	} else {
		// TODO: this function will send a second duplicate request before receiving result of the first request
		sprintf(log_msg, "sending %d entries to member %d, prev_log_index %lu, [%lu]", thread_arg->num_entries_to_send, thread_arg->id, thread_arg->arg.prev_log_index, seq);
		log_debug(function_tag, log_msg);
	}
	free(arg);
}

int comp_index(const void *a, const void *b) {
	return *(unsigned long *)b - *(unsigned long *)a;
}

int replicate_entries(WOOF *wf, unsigned long seq_no, void *ptr)
{
	RAFT_FUNCTION_LOOP *function_loop = (RAFT_FUNCTION_LOOP *)ptr;
	RAFT_REPLICATE_ENTRIES *arg = &(function_loop->replicate_entries);

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

	// get the latest log_entry seqno
	unsigned long last_log_index = WooFGetLatestSeqno(RAFT_LOG_ENTRIES_WOOF);
	if (WooFInvalid(last_log_index)) {
		sprintf(log_msg, "couldn't get the latest seqno from %s", RAFT_LOG_ENTRIES_WOOF);
		log_error(function_tag, log_msg);
		exit(1);
	}

	// get the latest append_entries_result seqno
	unsigned long latest_result_seqno = WooFGetLatestSeqno(RAFT_APPEND_ENTRIES_RESULT_WOOF);
	if (WooFInvalid(latest_result_seqno)) {
		sprintf(log_msg, "couldn't get the latest seqno from %s", RAFT_APPEND_ENTRIES_RESULT_WOOF);
		log_error(function_tag, log_msg);
		exit(1);
	}
	pthread_t *thread_id = malloc(sizeof(pthread_t) * server_state.members);
	memset(thread_id, 0, sizeof(pthread_t) * server_state.members);
	unsigned long min_seqno_to_send = last_log_index + 1;

	unsigned long seq;
	for (seq = arg->last_result_seqno + 1; seq <= latest_result_seqno; ++seq) {
		RAFT_APPEND_ENTRIES_RESULT result;
		int err = WooFGet(RAFT_APPEND_ENTRIES_RESULT_WOOF, &result, seq);
		if (err < 0) {
			sprintf(log_msg, "couldn't get append_entries_result at %lu", seq);
			log_error(function_tag, log_msg);
			free(thread_id);
			exit(1);
		}
		if (result.term != server_state.current_term) {
			arg->last_result_seqno = seq;
			continue;
		}
		int i;
		for (i = 0; i < server_state.members; ++i) {
			if (strcmp(result.server_woof, server_state.member_woofs[i]) == 0) {
				arg->next_index[i] = result.next_index;
				sprintf(log_msg, "updated next_index[%d] to %lu", i, arg->next_index[i]);
				log_debug(function_tag, log_msg);
				if (arg->next_index[i] < min_seqno_to_send) {
					min_seqno_to_send = arg->next_index[i];
				}
				if (result.success == true) {
					arg->match_index[i] = result.next_index - 1;
				}
				break;
			}
		}
		arg->last_result_seqno = seq;
	}

	// read all the entries to be sent to members, +1 for the previous entry
	// int num_entries = min(last_log_index - min_seqno_to_send + 1, RAFT_MAX_ENTRIES_PER_REQUEST);
	int num_entries = RAFT_MAX_ENTRIES_PER_REQUEST;
	if (last_log_index - min_seqno_to_send + 1 < num_entries) {
		num_entries = last_log_index - min_seqno_to_send + 1;
	}
	RAFT_LOG_ENTRY *entries = malloc(sizeof(RAFT_LOG_ENTRY) * (num_entries + 1));
	int i;
	for (i = 0; i < num_entries + 1; ++i) {
		err = WooFGet(RAFT_LOG_ENTRIES_WOOF, &entries[i], min_seqno_to_send - 1 + i);
		if (err < 0) {
			sprintf(log_msg, "couldn't get the log entries at %lu", min_seqno_to_send - 1 + i);
			log_error(function_tag, log_msg);
			free(thread_id);
			free(entries);
			exit(1);
		}
	}
	for (i = 0; i < server_state.members; ++i) {
		if (strcmp(server_state.member_woofs[i], server_state.woof_name) == 0) {
			arg->match_index[i] = last_log_index; // if it's leader itself, match_index is set to last_log_index
			continue;
		}
		unsigned long num_entries_to_send = last_log_index - arg->next_index[i] + 1;
		if (num_entries_to_send > RAFT_MAX_ENTRIES_PER_REQUEST) {
			num_entries_to_send = RAFT_MAX_ENTRIES_PER_REQUEST;
		}
		if (num_entries_to_send > 0) {
			APPEND_ENTRIES_THREAD_ARG *thread_arg = malloc(sizeof(APPEND_ENTRIES_THREAD_ARG));
			thread_arg->arg.term = server_state.current_term;
			memcpy(thread_arg->arg.leader_woof, server_state.woof_name, RAFT_WOOF_NAME_LENGTH);
			thread_arg->arg.prev_log_index = arg->next_index[i] - 1;
			int first_entry_index = arg->next_index[i] - min_seqno_to_send + 1;
			thread_arg->arg.prev_log_term = entries[first_entry_index - 1].term;
			memcpy(thread_arg->arg.entries, entries, sizeof(RAFT_LOG_ENTRY) * num_entries_to_send);
			thread_arg->arg.leader_commit = server_state.commit_index;
			thread_arg->arg.created_ts = get_milliseconds();
			thread_arg->id = i;
			thread_arg->num_entries_to_send = num_entries_to_send;
			// send append_entries request 
			sprintf(thread_arg->member_woof, "%s/%s", server_state.member_woofs[i], RAFT_APPEND_ENTRIES_ARG_WOOF);
			pthread_create(&thread_id[i], NULL, append_entries, (void *)thread_arg);
			// update last timestamp it send request to member
			arg->last_timestamp[i] = get_milliseconds();
		}
	}
	free(entries);

	for (i = 0; i < server_state.members; ++i) {
		if (strcmp(server_state.member_woofs[i], server_state.woof_name) == 0) {
			// no need to send heartbeat to itself
			continue;
		}
		if (get_milliseconds() - arg->last_timestamp[i] > RAFT_HEARTBEAT_RATE) {
			// didn't send any entry and timeout occurs, send heartbeat
			APPEND_ENTRIES_THREAD_ARG *thread_arg = malloc(sizeof(APPEND_ENTRIES_THREAD_ARG));
			thread_arg->arg.term = server_state.current_term;
			memcpy(thread_arg->arg.leader_woof, server_state.woof_name, RAFT_WOOF_NAME_LENGTH);
			memset(thread_arg->arg.entries, 0, sizeof(RAFT_LOG_ENTRY) * RAFT_MAX_ENTRIES_PER_REQUEST);
			thread_arg->arg.created_ts = get_milliseconds();
			thread_arg->id = i;
			thread_arg->num_entries_to_send = 0;
			// nothing else matters
			sprintf(thread_arg->member_woof, "%s/%s", server_state.member_woofs[i], RAFT_APPEND_ENTRIES_ARG_WOOF);
			pthread_create(&thread_id[i], NULL, append_entries, (void *)thread_arg);
			// update last timestamp it send request to member
			arg->last_timestamp[i] = get_milliseconds();
		}
	}

	// check if there's new commit_index
// sprintf(log_msg, "match_index: ");
// for (i = 0; i < server_state.members; ++i) {
// 	sprintf(log_msg + strlen(log_msg), "%lu ", arg->match_index[i]);
// }
// log_error(function_tag, log_msg);
	unsigned long *sorted_match_index = malloc(sizeof(unsigned long) * server_state.members);
	memcpy(sorted_match_index, arg->match_index, sizeof(unsigned long) * server_state.members);
	qsort(sorted_match_index, server_state.members, sizeof(unsigned long), comp_index);
// sprintf(log_msg, "sorted : ");
// for (i = 0; i < server_state.members; ++i) {
// 	sprintf(log_msg + strlen(log_msg), "%lu ", sorted_match_index[i]);
// }
// log_error(function_tag, log_msg);
	for (i = server_state.members / 2; i < server_state.members; ++i) {
		if (sorted_match_index[i] <= server_state.commit_index) {
			break;
		}
		RAFT_LOG_ENTRY entry;
		err = WooFGet(RAFT_LOG_ENTRIES_WOOF, &entry, sorted_match_index[i]);
		if (err < 0) {
			sprintf(log_msg, "couldn't get the log_entry at %lu", sorted_match_index[i]);
			log_error(function_tag, log_msg);
			free(thread_id);
			free(sorted_match_index);
			exit(1);
		}
		if (entry.term == server_state.current_term && sorted_match_index[i] > server_state.commit_index) {
			// update commit_index
			server_state.commit_index = sorted_match_index[i];
			unsigned long seq = WooFPut(RAFT_SERVER_STATE_WOOF, NULL, &server_state);
			if (WooFInvalid(seq)) {
				sprintf(log_msg, "couldn't update commit_index at term %lu", server_state.current_term);
				log_error(function_tag, log_msg);
				free(sorted_match_index);
				exit(1);
			}
			sprintf(log_msg, "updated commit_index to %lu at term %lu", server_state.commit_index, server_state.current_term);
			log_debug(function_tag, log_msg);
			break;
		}
	}
	free(sorted_match_index);
	
	// queue the next replicate_entries function
	usleep(RAFT_FUNCTION_LOOP_DELAY * 1000);
	seq = WooFPut(RAFT_FUNCTION_LOOP_WOOF, "review_append_entries", function_loop);
	if (WooFInvalid(seq)) {
		log_error(function_tag, "couldn't queue the next function_loop: review_append_entries");
		free(thread_id);
		exit(1);
	}
	for (i = 0; i < server_state.members; ++i) {
		if (thread_id[i] != 0) {
// sprintf(log_msg, "join on thread %d", i);
// log_debug(function_tag, log_msg);
			pthread_join(thread_id[i], NULL);
		}
	}
// log_debug(function_tag, "joined");
	free(thread_id);
	return 1;

}
