#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "woofc.h"
#include "raft.h"
#include "monitor.h"

int h_reconfig(WOOF *wf, unsigned long seq_no, void *ptr) {
	RAFT_RECONFIG_ARG *arg = (RAFT_RECONFIG_ARG *)ptr;

	log_set_tag("reconfig");
	// log_set_level(LOG_INFO);
	log_set_level(LOG_DEBUG);
	log_set_output(stdout);

	unsigned long latest_server_state = WooFGetLatestSeqno(RAFT_SERVER_STATE_WOOF);
	if (WooFInvalid(latest_server_state)) {
		log_error("couldn't get the latest seqno from %s", RAFT_SERVER_STATE_WOOF);
		exit(1);
	}
	RAFT_SERVER_STATE server_state;
	if (WooFGet(RAFT_SERVER_STATE_WOOF, &server_state, latest_server_state) < 0) {
		log_error("couldn't get the latest server state");
		exit(1);
	}

	if (server_state.current_config != RAFT_CONFIG_STABLE){
		log_debug("server is currently reconfiguring and not stable, rejected reconfiguring request");
		monitor_exit(wf, seq_no);
		return 1;
	}
	log_debug("processing new config with %d members", arg->members);

	// compute the joint config
	int joint_members;
	char joint_member_woofs[RAFT_MAX_SERVER_NUMBER][RAFT_WOOF_NAME_LENGTH];
	if (compute_joint_config(server_state.members, server_state.member_woofs, arg->members, arg->member_woofs, &joint_members, joint_member_woofs) < 0) {
		log_error("couldn't compute the joint config");
		exit(1);
	}
	log_debug("there are %d members in the joint config", joint_members);

	// append a config entry to log
	RAFT_LOG_ENTRY entry;
	entry.term = server_state.current_term;
	entry.is_config = true;
	if (encode_config(entry.data.val, joint_members, joint_member_woofs) < 0) {
		log_error("couldn't encode the joint config to a log entry");
		exit(1);
	}
	if (encode_config(entry.data.val + strlen(entry.data.val), arg->members, arg->member_woofs) < 0) {
		log_error("couldn't encode the new config to a log entry");
		exit(1);
	}
	unsigned long entry_seq = WooFPut(RAFT_LOG_ENTRIES_WOOF, NULL, &entry);
	if (WooFInvalid(entry_seq)) {
		log_error("couldn't put the joint config as log entry");
		exit(1);
	}
	log_debug("appended the joint config as a log entry");
	server_state.members = joint_members;
	server_state.current_config = RAFT_CONFIG_JOINT;
	server_state.last_config_seqno = entry_seq;
	memcpy(server_state.member_woofs, joint_member_woofs, RAFT_MAX_SERVER_NUMBER * RAFT_WOOF_NAME_LENGTH);
	unsigned long seq = WooFPut(RAFT_SERVER_STATE_WOOF, NULL, &server_state);
	if (WooFInvalid(seq)) {
		log_error("couldn't update server config at term %lu", server_state.current_term);
		exit(1);
	}
	log_debug("updated server config to %d members at term %lu", server_state.members, server_state.current_term);

	monitor_exit(wf, seq_no);
	return 1;
}
