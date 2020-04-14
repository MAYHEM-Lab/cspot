#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "woofc.h"
#include "raft.h"

int review_client_put(WOOF *wf, unsigned long seq_no, void *ptr) {
	RAFT_FUNCTION_LOOP *function_loop = (RAFT_FUNCTION_LOOP *)ptr;

	log_set_tag("review_client_put");
	// log_set_level(LOG_INFO);
	log_set_level(LOG_DEBUG);
	log_set_output(stdout);

	unsigned long latest_client_put = WooFGetLatestSeqno(RAFT_CLIENT_PUT_ARG_WOOF);
	if (WooFInvalid(latest_client_put)) {
		log_error("couldn't get the latest seqno from %s", RAFT_CLIENT_PUT_ARG_WOOF);
		exit(1);
	}
	if (function_loop->last_reviewed_client_put < latest_client_put) {
		// get the server's current term
		unsigned long last_server_state = WooFGetLatestSeqno(RAFT_SERVER_STATE_WOOF);
		if (WooFInvalid(last_server_state)) {
			log_error("couldn't get the latest seqno from %s", RAFT_SERVER_STATE_WOOF);
			exit(1);
		}
		RAFT_SERVER_STATE server_state;
		int err = WooFGet(RAFT_SERVER_STATE_WOOF, &server_state, last_server_state);
		if (err < 0) {
			log_error("couldn't get the server state");
			exit(1);
		}

		unsigned long i;
		RAFT_CLIENT_PUT_ARG arg;
		RAFT_CLIENT_PUT_RESULT result;
		for (i = function_loop->last_reviewed_client_put + 1; i <= latest_client_put; ++i) {
			// read the request
			err = WooFGet(RAFT_CLIENT_PUT_ARG_WOOF, &arg, i);
			if (err < 0) {
				log_error("couldn't get the request at %lu", i);
				exit(1);
			}

			if (server_state.role != RAFT_LEADER) {
				result.redirected = true;
				result.seq_no = 0;
				result.term = server_state.current_term;
				memcpy(result.current_leader, server_state.current_leader, RAFT_WOOF_NAME_LENGTH);
			} else {
				unsigned long seq;
				if (arg.is_config) {
					RAFT_RECONFIG_ARG new_config;
					err = decode_config(arg.data, &new_config.members, new_config.member_woofs);
					if (err < 0) {
						log_error("couldn't decode config from client put request");
						exit(1);
					}
					seq = WooFPut(RAFT_RECONFIG_ARG_WOOF, NULL, &new_config);
					if (WooFInvalid(seq)) {
						log_error("couldn't append new config");
						exit(1);
					}
					log_debug("received a new config, initialized reconfig process");
				} else {
					RAFT_LOG_ENTRY entry;
					entry.term = server_state.current_term;
					memcpy(&(entry.data), &(arg.data), sizeof(RAFT_DATA_TYPE));
					entry.is_config = false;
					seq = WooFPut(RAFT_LOG_ENTRIES_WOOF, NULL, &entry);
					if (WooFInvalid(seq)) {
						log_error("couldn't append raft log");
						exit(1);
					}
				}
				result.redirected = false;
				result.seq_no = seq;
				result.term = server_state.current_term;
			}
			unsigned long seq = WooFPut(RAFT_CLIENT_PUT_RESULT_WOOF, NULL, &result);
			if (WooFInvalid(seq)) {
				log_error("couldn't write client_put result");
				exit(1);
			}
			if (seq != i) {
				log_error("client_put_arg seqno %lu doesn't match result seno %lu", i, seq);
				exit(1);
			}
			function_loop->last_reviewed_client_put = i;
		}
	}
	
	// queue the next review function
	usleep(RAFT_FUNCTION_LOOP_DELAY * 1000);
	sprintf(function_loop->next_invoking, "review_reconfig");
	unsigned long seq = WooFPut(RAFT_FUNCTION_LOOP_WOOF, "review_reconfig", function_loop);
	if (WooFInvalid(seq)) {
		log_error("couldn't queue the next function_loop: review_reconfig");
		exit(1);
	}
	return 1;

}
