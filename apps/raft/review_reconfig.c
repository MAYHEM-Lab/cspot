#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "woofc.h"
#include "raft.h"

// only invoked when being a leader
int review_reconfig(WOOF *wf, unsigned long seq_no, void *ptr) {
	RAFT_FUNCTION_LOOP *function_loop = (RAFT_FUNCTION_LOOP *)ptr;
	RAFT_REVIEW_RECONFIG *arg = &(function_loop->review_config);

	log_set_tag("review_reconfig");
	// log_set_level(LOG_INFO);
	log_set_level(LOG_DEBUG);
	log_set_output(stdout);

	if (arg->current_config == RAFT_CONFIG_STABLE) {
		unsigned long latest_reconfig_arg = WooFGetLatestSeqno(RAFT_RECONFIG_ARG_WOOF);
		if (WooFInvalid(latest_reconfig_arg)) {
			log_error("couldn't get the latest seqno from %s", RAFT_RECONFIG_ARG_WOOF);
			exit(1);
		}
		if (latest_reconfig_arg > arg->last_reviewed_config) { // there's a new config
			// get the new config
			RAFT_RECONFIG_ARG new_config;
			int err = WooFGet(RAFT_RECONFIG_ARG_WOOF, &new_config, arg->last_reviewed_config + 1);
			if (err < 0) {
				log_error("couldn't get the new config at %lu", arg->last_reviewed_config + 1);
				exit(1);
			}
			// check the old config
			unsigned long latest_server_state = WooFGetLatestSeqno(RAFT_SERVER_STATE_WOOF);
			if (WooFInvalid(latest_server_state)) {
				log_error("couldn't get the latest seqno from %s", RAFT_SERVER_STATE_WOOF);
				exit(1);
			}
			RAFT_SERVER_STATE server_state;
			err = WooFGet(RAFT_SERVER_STATE_WOOF, &server_state, latest_server_state);
			if (err < 0) {
				log_error("couldn't get the latest server state");
				exit(1);
			}
			// compute the joint config
			int joint_members;
			char joint_member_woofs[RAFT_MAX_SERVER_NUMBER][RAFT_WOOF_NAME_LENGTH];
			err = compute_joint_config(server_state.members, server_state.member_woofs, new_config.members, new_config.member_woofs, &joint_members, joint_member_woofs);
			if (err < 0) {
				log_error("couldn't compute the joint config");
				exit(1);
			}
			// append a config entry to log
			RAFT_LOG_ENTRY entry;
			entry.term = server_state.current_term;
			entry.is_config = true;
			err = encode_config(joint_members, joint_member_woofs, &entry.data);
			if (err < 0) {
				log_error("couldn't encode the joint config to a log entry");
				exit(1);
			}
			unsigned long seq = WooFPut(RAFT_LOG_ENTRIES_WOOF, NULL, &entry);
			if (WooFInvalid(seq)) {
				log_error("couldn't put the joint config as log entry");
				exit(1);
			}
			log_debug("appended the joint config as a log entry");

			arg->current_config = RAFT_CONFIG_JOINT;
			arg->c_joint_seqno = seq;
			arg->c_new_seqno = 0;
			arg->joint_members = joint_members;
			arg->new_config_seqno = arg->last_reviewed_config + 1;
			arg->last_reviewed_config = arg->last_reviewed_config + 1;
		}
	} else if (arg->current_config == RAFT_CONFIG_JOINT) {
		// see if the joint config is committed
		unsigned long latest_server_state = WooFGetLatestSeqno(RAFT_SERVER_STATE_WOOF);
		if (WooFInvalid(latest_server_state)) {
			log_error("couldn't get the latest seqno from %s", RAFT_SERVER_STATE_WOOF);
			exit(1);
		}
		RAFT_SERVER_STATE server_state;
		int err = WooFGet(RAFT_SERVER_STATE_WOOF, &server_state, latest_server_state);
		if (err < 0) {
			log_error("couldn't get the latest server state");
			exit(1);
		}
		// if the joint config is committed, append the new config
		if (server_state.commit_index >= arg->c_joint_seqno) {
			// get the new config
			RAFT_RECONFIG_ARG new_config;
			err = WooFGet(RAFT_RECONFIG_ARG_WOOF, &new_config, arg->new_config_seqno);
			if (err < 0) {
				log_error("couldn't get the new config at %lu", arg->new_config_seqno);
				exit(1);
			}
			// append the new config
			RAFT_LOG_ENTRY entry;
			entry.term = server_state.current_term;
			entry.is_config = true;
			err = encode_config(new_config.members, new_config.member_woofs, &entry.data);
			if (err < 0) {
				log_error("couldn't encode the new config to a log entry");
				exit(1);
			}
			unsigned long seq = WooFPut(RAFT_LOG_ENTRIES_WOOF, NULL, &entry);
			if (WooFInvalid(seq)) {
				log_error("couldn't put the new config as log entry");
				exit(1);
			}
			log_debug("the joint config is committed. Appended the new config as a log entry");

			arg->current_config = RAFT_CONFIG_NEW;
			arg->c_new_seqno = seq;
		}
	} else if (arg->current_config == RAFT_CONFIG_NEW) {
		// check if the new config is committed
		unsigned long latest_server_state = WooFGetLatestSeqno(RAFT_SERVER_STATE_WOOF);
		if (WooFInvalid(latest_server_state)) {
			log_error("couldn't get the latest seqno from %s", RAFT_SERVER_STATE_WOOF);
			exit(1);
		}
		RAFT_SERVER_STATE server_state;
		int err = WooFGet(RAFT_SERVER_STATE_WOOF, &server_state, latest_server_state);
		if (err < 0) {
			log_error("couldn't get the latest server state");
			exit(1);
		}
		// if the new config is committed, notify servers only in old config to shut down
		if (server_state.commit_index >= arg->c_new_seqno) {
			arg->current_config = RAFT_CONFIG_STABLE;
			log_info("the new config is committed. safe to shutdown servers in the old config");
			bool in_config = false;
			int i;
			for (i = 0; i < server_state.members; ++i) {
				if (strcmp(server_state.woof_name, server_state.member_woofs[i]) == 0) {
					in_config = true;
					break;
				}
			}
			// if not in new config, step down
			if (!in_config) {
				RAFT_TERM_ENTRY term_entry;
				term_entry.term = server_state.current_term + 1;
				term_entry.role = RAFT_SHUTDOWN;
				memset(term_entry.leader, 0, RAFT_WOOF_NAME_LENGTH);
				unsigned long seq = WooFPut(RAFT_TERM_ENTRIES_WOOF, NULL, &term_entry);
				if (WooFInvalid(seq)) {
					log_error("couldn't step down");
					exit(1);
				}
			}
		} 
	}
	
	// queue the next review function
	usleep(RAFT_FUNCTION_LOOP_DELAY * 1000);
	sprintf(function_loop->next_invoking, "term_chair");
	unsigned long seq = WooFPut(RAFT_FUNCTION_LOOP_WOOF, "term_chair", function_loop);
	if (WooFInvalid(seq)) {
		log_error("couldn't queue the next function_loop: term_chair");
		exit(1);
	}
	return 1;

}
