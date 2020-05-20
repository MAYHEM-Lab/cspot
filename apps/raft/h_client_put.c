#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "woofc.h"
#include "raft.h"
#include "raft_utils.h"
#include "monitor.h"

int h_client_put(WOOF *wf, unsigned long seq_no, void *ptr) {
	RAFT_CLIENT_PUT_ARG *arg = (RAFT_CLIENT_PUT_ARG *)monitor_cast(ptr);
	seq_no = monitor_seqno(ptr);
	
	log_set_tag("client_put");
	// log_set_level(LOG_INFO);
	log_set_level(RAFT_LOG_DEBUG);
	log_set_output(stdout);

	// get the server's current term
	RAFT_SERVER_STATE server_state;
	if (get_server_state(&server_state) < 0) {
		log_error("failed to get the server state");
		free(arg);
		exit(1);
	}

	unsigned long last_request = WooFGetLatestSeqno(RAFT_CLIENT_PUT_REQUEST_WOOF);
	if (WooFInvalid(last_request)) {
		log_error("failed to get the latest seqno from %s", RAFT_CLIENT_PUT_RESULT_WOOF);
		free(arg);
		exit(1);
	}

	unsigned long i;
	for (i = arg->last_seqno + 1; i <= last_request; ++i) {
		RAFT_CLIENT_PUT_REQUEST request;
		if (WooFGet(RAFT_CLIENT_PUT_REQUEST_WOOF, &request, i) < 0) {
			log_error("failed to get client_put request at %lu", i);
			free(arg);
			exit(1);
		}
		RAFT_CLIENT_PUT_RESULT result;
		if (server_state.role != RAFT_LEADER) {
			result.redirected = 1;
			result.index = 0;
			result.term = server_state.current_term;
			memcpy(result.current_leader, server_state.current_leader, RAFT_NAME_LENGTH);
		} else {
			unsigned long entry_seqno;
			RAFT_LOG_ENTRY entry;
			entry.term = server_state.current_term;
			memcpy(&entry.data, &request.data, sizeof(RAFT_DATA_TYPE));
			entry.is_config = 0;
			entry_seqno = WooFPut(RAFT_LOG_ENTRIES_WOOF, NULL, &entry);
			if (WooFInvalid(entry_seqno)) {
				log_error("failed to append raft log");
				free(arg);
				exit(1);
			}
			result.redirected = 0;
			result.index = entry_seqno;
			result.term = server_state.current_term;
			if (entry_seqno % RAFT_SAMPLING_RATE == 0) {
				log_debug("entry %lu was created at %lu", entry_seqno, request.created_ts);
			}
		}
		// make sure the result's seq_no matches with request's seq_no
		unsigned long latest_result_seqno = WooFGetLatestSeqno(RAFT_CLIENT_PUT_RESULT_WOOF);
		if (latest_result_seqno < i - 1) {
			log_warn("client_put_result at %lu, client_put_request at %lu, padding %lu results", latest_result_seqno, i, i - latest_result_seqno - 1);
			unsigned long j;
			for (j = latest_result_seqno + 1; j <= i - 1; ++j) {	
				RAFT_CLIENT_PUT_RESULT result;
				unsigned long result_seq = WooFPut(RAFT_CLIENT_PUT_RESULT_WOOF, NULL, &result);
				if (WooFInvalid(result_seq)) {
					log_error("failed to put padded result");
					free(arg);
					exit(1);
				}
			}
		}

		unsigned long result_seq = WooFPut(RAFT_CLIENT_PUT_RESULT_WOOF, NULL, &result);
		while (WooFInvalid(result_seq)) {
			log_error("failed to write client_put_result");
			free(arg);
			exit(1);
		}
		if (result_seq != i) {
			log_error("client_put_request seq_no %lu doesn't match result seq_no %lu", i, result_seq);
			free(arg);
			exit(1);
		}

		arg->last_seqno = i;
	}
	monitor_exit(ptr);

	usleep(RAFT_CLIENT_PUT_DELAY * 1000);
	unsigned long seq = monitor_put(RAFT_MONITOR_NAME, RAFT_CLIENT_PUT_ARG_WOOF, "h_client_put", arg);
	if (WooFInvalid(seq)) {
		log_error("failed to queue the next h_client_put handler");
		free(arg);
		exit(1);
	}
	free(arg);
	return 1;
}
