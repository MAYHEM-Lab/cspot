#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "woofc.h"
#include "raft.h"
#include "monitor.h"

int h_client_put(WOOF *wf, unsigned long seq_no, void *ptr) {
	RAFT_CLIENT_PUT_ARG *arg = (RAFT_CLIENT_PUT_ARG *)monitor_cast(ptr);
	seq_no = monitor_seqno(ptr);
	
	log_set_tag("client_put");
	// log_set_level(LOG_INFO);
	log_set_level(LOG_DEBUG);
	log_set_output(stdout);

	// get the server's current term
	RAFT_SERVER_STATE server_state;
	if (get_server_state(&server_state) < 0) {
		log_error("failed to get the server state");
		free(arg);
		exit(1);
	}

	RAFT_CLIENT_PUT_RESULT result;
	if (server_state.role != RAFT_LEADER) {
		result.redirected = 1;
		result.index = 0;
		result.term = server_state.current_term;
		memcpy(result.current_leader, server_state.current_leader, RAFT_WOOF_NAME_LENGTH);
	} else {
		unsigned long entry_seqno;
		RAFT_LOG_ENTRY entry;
		entry.term = server_state.current_term;
		memcpy(&entry.data, &arg->data, sizeof(RAFT_DATA_TYPE));
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
	}
	monitor_exit(ptr);

	// for very slight chance that h_client_put is not queued in the same order of the element appended in RAFT_CLIENT_PUT_ARG_WOOF
	unsigned long latest_result_seqno = WooFGetLatestSeqno(RAFT_CLIENT_PUT_RESULT_WOOF);
	while (latest_result_seqno != seq_no - 1) {
		log_warn("client_put_arg seqno not matching, waiting %lu", seq_no);
		usleep(RAFT_FUNCTION_DELAY * 1000);
		latest_result_seqno = WooFGetLatestSeqno(RAFT_CLIENT_PUT_RESULT_WOOF);
	}

	unsigned long result_seq = WooFPut(RAFT_CLIENT_PUT_RESULT_WOOF, NULL, &result);
	while (WooFInvalid(result_seq)) {
		log_warn("failed to write client_put result, try again");
		usleep(RAFT_FUNCTION_DELAY * 1000);
		result_seq = WooFPut(RAFT_CLIENT_PUT_RESULT_WOOF, NULL, &result);
	}

	if (result_seq != seq_no) {
		log_error("client_put_arg seqno %lu doesn't match result seno %lu", seq_no, result_seq);
		free(arg);
		exit(1);
	}

	free(arg);
	return 1;
}
