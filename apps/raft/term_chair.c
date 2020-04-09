#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "woofc.h"
#include "raft.h"

char function_tag[] = "term_chair";

typedef struct request_vote_thread_arg {
	char member_woof[RAFT_WOOF_NAME_LENGTH];
	RAFT_REQUEST_VOTE_ARG arg;
} REQUEST_VOTE_THREAD_ARG;

void *request_vote(void *arg) {
	REQUEST_VOTE_THREAD_ARG *thread_arg = (REQUEST_VOTE_THREAD_ARG *)arg;
	unsigned long seq = WooFPut(thread_arg->member_woof, NULL, &(thread_arg->arg));
	if (WooFInvalid(seq)) {
		sprintf(log_msg, "couldn't request vote from %s", thread_arg->member_woof);
		log_warn(function_tag, log_msg);
	}
	free(arg);
}

int term_chair(WOOF *wf, unsigned long seq_no, void *ptr)
{
	RAFT_FUNCTION_LOOP *function_loop = (RAFT_FUNCTION_LOOP *)ptr;

	// log_set_level(LOG_INFO);
	log_set_level(LOG_DEBUG);
	log_set_output(stdout);

	// get the current term
	unsigned long latest_server_state = WooFGetLatestSeqno(RAFT_SERVER_STATE_WOOF);
	if (WooFInvalid(latest_server_state)) {
		sprintf(log_msg, "couldn't get the latest seqno from %s", RAFT_SERVER_STATE_WOOF);
		log_error(function_tag, log_msg);
		exit(1);
	}
	RAFT_SERVER_STATE server_state;
	int err = WooFGet(RAFT_SERVER_STATE_WOOF, &server_state, latest_server_state);
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
	// sprintf(log_msg, "reviewing new term request from %lu to %lu", function_loop->last_reviewed_term_chair + 1, last_term_entry);
	// log_debug(function_tag, log_msg);
	if (function_loop->last_reviewed_term_chair >= last_term_entry) {
		usleep(RAFT_FUNCTION_LOOP_DELAY * 1000);
		if (server_state.role == RAFT_LEADER) {
			unsigned long seq = WooFPut(RAFT_FUNCTION_LOOP_WOOF, "replicate_entries", function_loop);
			if (WooFInvalid(seq)) {
				log_error(function_tag, "couldn't queue the next function_loop: replicate_entries");
				exit(1);
			}
		} else {
			unsigned long seq = WooFPut(RAFT_FUNCTION_LOOP_WOOF, "review_append_entries", function_loop);
			if (WooFInvalid(seq)) {
				log_error(function_tag, "couldn't queue the next function_loop: review_append_entries");
				exit(1);
			}
		}
		return 1;
	}

	// read the queued entries and find out the highest term
	unsigned long next_term = server_state.current_term;
	int next_role = server_state.role;
	unsigned long i;
	RAFT_TERM_ENTRY entry;
	for (i = function_loop->last_reviewed_term_chair + 1; i <= last_term_entry; ++i) {
		err = WooFGet(RAFT_TERM_ENTRIES_WOOF, &entry, i);
		if (err < 0) {
			sprintf(log_msg, "couldn't get the term entry at %lu", i);
			log_error(function_tag, log_msg);
			exit(1);
		}
		if (entry.term >= next_term) {
			next_term = entry.term;
			next_role = entry.role;
		}
		function_loop->last_reviewed_term_chair = i;
	}

	// if there is a new term or new role
	if (next_term > server_state.current_term || (next_term == server_state.current_term && next_role != server_state.role)) {
		// if there's a new term, reset voted_for
		if (next_term > server_state.current_term) {
			memset(server_state.voted_for, 0, RAFT_WOOF_NAME_LENGTH);
		}
		// update the server's current term
		server_state.current_term = next_term;
		server_state.role = next_role;
		unsigned long seq = WooFPut(RAFT_SERVER_STATE_WOOF, NULL, &server_state);
		if (WooFInvalid(seq)) {
			sprintf(log_msg, "couldn't increment the current term to %lu", next_term);
			log_error(function_tag, log_msg);
			exit(1);
		}
		switch (next_role) {
			case RAFT_LEADER: {
				sprintf(log_msg, "incremented the current term to %lu, role: LEADER", next_term);
				break;
			}
			case RAFT_FOLLOWER: {
				sprintf(log_msg, "incremented the current term to %lu, role: FOLLOWER", next_term);
				break;
			}
			case RAFT_CANDIDATE: {
				sprintf(log_msg, "incremented the current term to %lu, role: CANDIDATE", next_term);
				break;
			}
		}
		log_info(function_tag, log_msg);

		// new term, new heartbeat
		if (next_role != RAFT_LEADER) { // leader won't timeout
			RAFT_HEARTBEAT heartbeat;
			seq = WooFPut(RAFT_HEARTBEAT_WOOF, "timeout_checker", &heartbeat);
			if (WooFInvalid(seq)) {
				sprintf(log_msg, "couldn't put a new heartbeat for new term %lu", next_term);
				log_error(function_tag, log_msg);
				exit(1);
			}
			// sprintf(log_msg, "put a new heartbeat at term %lu", next_term);
			// log_debug(function_tag, log_msg);
		}

		if (next_role == RAFT_CANDIDATE) {
			// initialize the vote progress
			// remember the latest seqno of request_vote_result before requesting votes
			unsigned long vote_pool_seqno = WooFGetLatestSeqno(RAFT_REQUEST_VOTE_RESULT_WOOF);
			if (WooFInvalid(vote_pool_seqno)) {
				sprintf(log_msg, "couldn't get the latest seqno from %s", RAFT_REQUEST_VOTE_RESULT_WOOF);
				log_error(function_tag, log_msg);
				exit(1);
			}
			
			// get last log entry info
			unsigned long latest_log_entry = WooFGetLatestSeqno(RAFT_LOG_ENTRIES_WOOF);
			if (WooFInvalid(latest_log_entry)) {
				sprintf(log_msg, "couldn't get the latest seqno from %s", RAFT_LOG_ENTRIES_WOOF);
				log_error(function_tag, log_msg);
				exit(1);
			}
			RAFT_LOG_ENTRY last_log_entry;
			int err = WooFGet(RAFT_LOG_ENTRIES_WOOF, &last_log_entry, latest_log_entry);
			if (err < 0) {
				sprintf(log_msg, "couldn't get the latest log entry %lu from %s", latest_log_entry, RAFT_LOG_ENTRIES_WOOF);
				log_error(function_tag, log_msg);
				exit(1);
			}

			// requesting votes from members
			int i;
			pthread_t thread_id[server_state.members];
			for (i = 0; i < server_state.members; ++i) {
				REQUEST_VOTE_THREAD_ARG *thread_arg = malloc(sizeof(REQUEST_VOTE_THREAD_ARG));
				sprintf(thread_arg->member_woof, "%s/%s", server_state.member_woofs[i], RAFT_REQUEST_VOTE_ARG_WOOF);
				thread_arg->arg.term = server_state.current_term;
				strncpy(thread_arg->arg.candidate_woof, server_state.woof_name, RAFT_WOOF_NAME_LENGTH);
				thread_arg->arg.candidate_vote_pool_seqno = vote_pool_seqno;
				thread_arg->arg.last_log_index = latest_log_entry;
				thread_arg->arg.last_log_term = last_log_entry.term;
				pthread_create(&thread_id[i], NULL, request_vote, (void *)thread_arg); 
			}
			for (i = 0; i < server_state.members; ++i) {
				pthread_join(thread_id[i], NULL);
			}
			sprintf(log_msg, "requested vote from %d members", server_state.members);
			log_debug(function_tag, log_msg);
		} else if (next_role == RAFT_LEADER) {
			// reinitialize replicate_entries parameters
			unsigned long last_log_entry = WooFGetLatestSeqno(RAFT_LOG_ENTRIES_WOOF);
			if (WooFInvalid(last_log_entry)) {
				sprintf(log_msg, "couldn't get the latest seqno from %s", RAFT_LOG_ENTRIES_WOOF);
				log_error(function_tag, log_msg);
				exit(1);
			}
			memset(function_loop->replicate_entries.last_sent_index, 0, sizeof(unsigned long) * server_state.members);
			memset(function_loop->replicate_entries.match_index, 0, sizeof(unsigned long) * server_state.members);
			memset(function_loop->replicate_entries.last_timestamp, 0, sizeof(unsigned long) * server_state.members);
			int i;
			for (i = 0; i < server_state.members; i++) {
				function_loop->replicate_entries.next_index[i] = last_log_entry + 1;
			}
			unsigned long last_append_result = WooFGetLatestSeqno(RAFT_APPEND_ENTRIES_RESULT_WOOF);
			if (WooFInvalid(last_append_result)) {
				sprintf(log_msg, "couldn't get the latest seqno from %s", RAFT_APPEND_ENTRIES_RESULT_WOOF);
				log_error(function_tag, log_msg);
				exit(1);
			}
			function_loop->replicate_entries.last_result_seqno = last_append_result;
			// replicate_entries will be queued as next function_loop
		}
	}

	usleep(RAFT_FUNCTION_LOOP_DELAY * 1000);
	if (server_state.role == RAFT_LEADER) {
		unsigned long seq = WooFPut(RAFT_FUNCTION_LOOP_WOOF, "replicate_entries", function_loop);
		if (WooFInvalid(seq)) {
			log_error(function_tag, "couldn't queue the next function_loop: replicate_entries");
			exit(1);
		}
	} else {
		unsigned long seq = WooFPut(RAFT_FUNCTION_LOOP_WOOF, "review_append_entries", function_loop);
		if (WooFInvalid(seq)) {
			log_error(function_tag, "couldn't queue the next function_loop: review_append_entries");
			exit(1);
		}
	}
	return 1;

}
