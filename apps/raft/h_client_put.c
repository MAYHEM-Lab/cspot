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
	unsigned long last_server_state = WooFGetLatestSeqno(RAFT_SERVER_STATE_WOOF);
	if (WooFInvalid(last_server_state)) {
		log_error("failed to get the latest seqno from %s", RAFT_SERVER_STATE_WOOF);
		free(arg);
		exit(1);
	}
	RAFT_SERVER_STATE server_state;
	if (WooFGet(RAFT_SERVER_STATE_WOOF, &server_state, last_server_state) < 0) {
		log_error("failed to get the server state");
		free(arg);
		exit(1);
	}

	RAFT_CLIENT_PUT_RESULT result;
	if (server_state.role != RAFT_LEADER) {
		result.redirected = true;
		result.seq_no = 0;
		result.term = server_state.current_term;
		memcpy(result.current_leader, server_state.current_leader, RAFT_WOOF_NAME_LENGTH);
	} else {
		unsigned long seq;
		if (arg->is_config) {
			RAFT_RECONFIG_ARG reconfig;
			if (decode_config(arg->data.val, &reconfig.members, reconfig.member_woofs) < 0) {
				log_error("failed to decode config from client put request");
				free(arg);
				exit(1);
			}
			seq = WooFPut(RAFT_RECONFIG_ARG_WOOF, NULL, &reconfig);
			if (WooFInvalid(seq)) {
				log_error("failed to append new config");
				free(arg);
				exit(1);
			}
			log_debug("received a new config, initialized reconfig process");
		} else {
			RAFT_LOG_ENTRY entry;
			entry.term = server_state.current_term;
			memcpy(&entry.data, &arg->data, sizeof(RAFT_DATA_TYPE));
			entry.is_config = false;
			seq = WooFPut(RAFT_LOG_ENTRIES_WOOF, NULL, &entry);
			if (WooFInvalid(seq)) {
				log_error("failed to append raft log");
				free(arg);
				exit(1);
			}
		}
		result.redirected = false;
		result.seq_no = seq;
		result.term = server_state.current_term;
	}

	monitor_exit(ptr);
	unsigned long latest_result_seqno = WooFGetLatestSeqno(RAFT_CLIENT_PUT_RESULT_WOOF);
	while (latest_result_seqno != seq_no - 1) {
		log_error("client_put_arg seqno not matching, waiting %lu", seq_no);
		usleep(RAFT_FUNCTION_DELAY * 1000);
		latest_result_seqno = WooFGetLatestSeqno(RAFT_CLIENT_PUT_RESULT_WOOF);
	}

	unsigned long result_seq = WooFPut(RAFT_CLIENT_PUT_RESULT_WOOF, NULL, &result);
	if (WooFInvalid(result_seq)) {
		log_error("failed to write client_put result");
		free(arg);
		exit(1);
	}
	if (result_seq != seq_no) {
		log_error("client_put_arg seqno %lu doesn't match result seno %lu", seq_no, result_seq);
		exit(1);
	}

	return 1;
}
