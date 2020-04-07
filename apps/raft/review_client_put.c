#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "woofc.h"
#include "raft.h"

char function_tag[] = "review_client_put";

int review_client_put(WOOF *wf, unsigned long seq_no, void *ptr)
{
	RAFT_FUNCTION_LOOP *function_loop = (RAFT_FUNCTION_LOOP *)ptr;

	// log_set_level(LOG_INFO);
	log_set_level(LOG_DEBUG);
	log_set_output(stdout);

	unsigned long latest_client_put = WooFGetLatestSeqno(RAFT_CLIENT_PUT_ARG_WOOF);
	if (WooFInvalid(latest_client_put)) {
		sprintf(log_msg, "couldn't get the latest seqno from %s", RAFT_CLIENT_PUT_ARG_WOOF);
		log_error(function_tag, log_msg);
		exit(1);
	}
	if (function_loop->last_reviewed_client_put < latest_client_put) {
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
			log_error(function_tag, "couldn't get the server state");
			exit(1);
		}

		unsigned long i;
		RAFT_CLIENT_PUT_ARG arg;
		RAFT_CLIENT_PUT_RESULT result;
		for (i = function_loop->last_reviewed_client_put + 1; i <= latest_client_put; ++i) {
			// read the request
			err = WooFGet(RAFT_CLIENT_PUT_ARG_WOOF, &arg, i);
			if (err < 0) {
				sprintf(log_msg, "couldn't get the request at %lu", i);
				log_error(function_tag, log_msg);
				exit(1);
			}

			if (server_state.role != RAFT_LEADER) {
				result.success = false;
				result.seq_no = 0;
				result.term = server_state.current_term;
			} else {
				RAFT_LOG_ENTRY entry;
				entry.term = server_state.current_term;
				memcpy(&(entry.data), &(arg.data), sizeof(DATA_TYPE));
				unsigned long seq = WooFPut(RAFT_LOG_ENTRIES_WOOF, NULL, &entry);
				if (WooFInvalid(seq)) {
					log_error(function_tag, "couldn't append raft log");
					exit(1);
				}
				result.success = true;
				result.seq_no = seq;
				result.term = server_state.current_term;
			}
			unsigned long seq = WooFPut(RAFT_CLIENT_PUT_RESULT_WOOF, NULL, &result);
			if (WooFInvalid(seq)) {
				log_error(function_tag, "couldn't write client_put result");
				exit(1);
			}
			if (seq != i) {
				sprintf(log_msg, "client_put_arg seqno %lu doesn't match result seno %lu", i, seq);
				log_error(function_tag, log_msg);
				exit(1);
			}
			function_loop->last_reviewed_client_put = i;
		}
	}
	
	// queue the next review function
	usleep(RAFT_FUNCTION_LOOP_DELAY * 1000);
	sprintf(function_loop->next_invoking, "term_chair");
	unsigned long seq = WooFPut(RAFT_FUNCTION_LOOP_WOOF, "term_chair", function_loop);
	if (WooFInvalid(seq)) {
		log_error(function_tag, "couldn't queue the next function_loop: term_chair");
		exit(1);
	}
	return 1;

}
