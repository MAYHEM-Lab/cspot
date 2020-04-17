#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#include "woofc.h"
#include "raft.h"

typedef struct append_entries_thread_arg {
	int member_id;
	int num_entries_to_send;
	char member_woof[RAFT_WOOF_NAME_LENGTH];
	RAFT_APPEND_ENTRIES_ARG arg;
} APPEND_ENTRIES_THREAD_ARG;

void *append_entries(void *arg) {
	APPEND_ENTRIES_THREAD_ARG *thread_arg = (APPEND_ENTRIES_THREAD_ARG *)arg;
	unsigned long seq = WooFPut(thread_arg->member_woof, NULL, &thread_arg->arg);
	if (WooFInvalid(seq)) {
		log_error("couldn't send append_entries request to %s", thread_arg->member_woof);
	}
	if (thread_arg->arg.entries[0].term == 0) {
		log_debug("sent heartbeat (%lu) to %s", seq, thread_arg->member_woof);
	} else {
		log_debug("sending %d entries to member %d, prev_log_index:%lu, prev_log_term:%lu [%lu]", thread_arg->num_entries_to_send, thread_arg->member_id, thread_arg->arg.prev_log_index, thread_arg->arg.prev_log_term, seq);
	}
	free(arg);
}

int comp_index(const void *a, const void *b) {
	return *(unsigned long *)b - *(unsigned long *)a;
}

int h_replicate_entries(WOOF *wf, unsigned long seq_no, void *ptr) {
	RAFT_REPLICATE_ENTRIES *arg = (RAFT_REPLICATE_ENTRIES *)ptr;

	log_set_tag("replicate_entries");
	// log_set_level(LOG_INFO);
	log_set_level(LOG_DEBUG);
	log_set_output(stdout);

	// zsys_init() is called automatically when a socket is created
	// not thread safe and can only be called in main thread
	// call it here to avoid being called concurrently in the threads
	zsys_init();

	// get the server's current term and cluster members
	unsigned long last_server_state = WooFGetLatestSeqno(RAFT_SERVER_STATE_WOOF);
	if (WooFInvalid(last_server_state)) {
		log_error("couldn't get the latest seqno from %s", RAFT_SERVER_STATE_WOOF);
		exit(1);
	}
	RAFT_SERVER_STATE server_state;
	if (WooFGet(RAFT_SERVER_STATE_WOOF, &server_state, last_server_state) < 0) {
		log_error("couldn't get the server state");
		exit(1);
	}

	unsigned long min_seqno_to_send = arg->last_log_entry_seqno + 1;
	int i;
	for (i = 0; i < server_state.members; ++i) {
		if (strcmp(server_state.member_woofs[i], server_state.woof_name) == 0) {
			continue; // no need to replicate to itself
		}
		if (server_state.next_index[i] < min_seqno_to_send) {
			min_seqno_to_send = server_state.next_index[i];
		}
	}

	// read all the entries to be sent to members, +1 for the previous entry
	// int num_entries = min(arg->last_log_entry_seqno - min_seqno_to_send + 1, RAFT_MAX_ENTRIES_PER_REQUEST);
	int num_entries = RAFT_MAX_ENTRIES_PER_REQUEST;
	if (arg->last_log_entry_seqno - min_seqno_to_send + 1 < num_entries) {
		num_entries = arg->last_log_entry_seqno - min_seqno_to_send + 1;
	}
	RAFT_LOG_ENTRY *entries = malloc(sizeof(RAFT_LOG_ENTRY) * (num_entries + 1));
	for (i = 0; i < num_entries + 1; ++i) {
		if (min_seqno_to_send - 1 + i > 0) {
			if (WooFGet(RAFT_LOG_ENTRIES_WOOF, &entries[i], min_seqno_to_send - 1 + i) < 0) {
				log_error("couldn't get the log entries at %lu", min_seqno_to_send - 1 + i);
				free(entries);
				exit(1);
			}
		}
	}

	pthread_t *thread_id = malloc(sizeof(pthread_t) * server_state.members);
	memset(thread_id, 0, sizeof(pthread_t) * server_state.members);
	for (i = 0; i < server_state.members; ++i) {
		if (strcmp(server_state.member_woofs[i], server_state.woof_name) == 0) {
			server_state.match_index[i] = arg->last_log_entry_seqno; // if it's leader itself, match_index is set to arg->last_log_entry_seqno
			continue;
		}
		unsigned long num_entries_to_send = arg->last_log_entry_seqno - server_state.next_index[i] + 1;
		if (num_entries_to_send > RAFT_MAX_ENTRIES_PER_REQUEST) {
			num_entries_to_send = RAFT_MAX_ENTRIES_PER_REQUEST;
		}
		if (num_entries_to_send > 0) {
			APPEND_ENTRIES_THREAD_ARG *thread_arg = malloc(sizeof(APPEND_ENTRIES_THREAD_ARG));
			thread_arg->arg.term = server_state.current_term;
			memcpy(thread_arg->arg.leader_woof, server_state.woof_name, RAFT_WOOF_NAME_LENGTH);
			thread_arg->arg.prev_log_index = server_state.next_index[i] - 1;
			int first_entry_index = server_state.next_index[i] - min_seqno_to_send + 1;
			thread_arg->arg.prev_log_term = entries[first_entry_index - 1].term;
			if (thread_arg->arg.prev_log_index == 0) {
				thread_arg->arg.prev_log_term = 0;
			}
			memcpy(thread_arg->arg.entries, entries + first_entry_index, sizeof(RAFT_LOG_ENTRY) * num_entries_to_send);
			thread_arg->arg.leader_commit = server_state.commit_index;
			thread_arg->arg.created_ts = get_milliseconds();
			thread_arg->member_id = i;
			thread_arg->num_entries_to_send = num_entries_to_send;
			// send append_entries request 
			sprintf(thread_arg->member_woof, "%s/%s", server_state.member_woofs[i], RAFT_APPEND_ENTRIES_ARG_WOOF);
			pthread_create(&thread_id[i], NULL, append_entries, (void *)thread_arg);
		}
	}
	free(entries);
	
	// TODO: send heartbeat

	// TODO: update commit_index
	// char log_msg[256];
	// // check if there's new commit_index
	// unsigned long *sorted_match_index = malloc(sizeof(unsigned long) * server_state.members);
	// memcpy(sorted_match_index, replicate_entries->match_index, sizeof(unsigned long) * server_state.members);
	// qsort(sorted_match_index, server_state.members, sizeof(unsigned long), comp_index);
	// for (i = server_state.members / 2; i < server_state.members; ++i) {
	// 	if (sorted_match_index[i] <= server_state.commit_index) {
	// 		break;
	// 	}
	// 	RAFT_LOG_ENTRY entry;
	// 	err = WooFGet(RAFT_LOG_ENTRIES_WOOF, &entry, sorted_match_index[i]);
	// 	if (err < 0) {
	// 		log_error("couldn't get the log_entry at %lu", sorted_match_index[i]);
	// 		free(thread_id);
	// 		free(sorted_match_index);
	// 		exit(1);
	// 	}
	// 	if (entry.term == server_state.current_term && sorted_match_index[i] > server_state.commit_index) {
	// 		// update commit_index
	// 		server_state.commit_index = sorted_match_index[i];
	// 		unsigned long seq = WooFPut(RAFT_SERVER_STATE_WOOF, NULL, &server_state);
	// 		if (WooFInvalid(seq)) {
	// 			log_error("couldn't update commit_index at term %lu", server_state.current_term);
	// 			free(thread_id);
	// 			free(sorted_match_index);
	// 			exit(1);
	// 		}
	// 		log_debug("updated commit_index to %lu at term %lu", server_state.commit_index, server_state.current_term);
	// 		break;
	// 	}
	// }
	// free(sorted_match_index);

	threads_join(server_state.members, thread_id);
	free(thread_id);
	return 1;

}
