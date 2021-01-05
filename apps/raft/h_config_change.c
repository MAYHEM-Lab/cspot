#include "raft.h"
#include "raft_utils.h"
#include "woofc-access.h"
#include "woofc.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

int h_config_change(WOOF* wf, unsigned long seq_no, void* ptr) {
    RAFT_CONFIG_CHANGE_ARG* arg = (RAFT_CONFIG_CHANGE_ARG*)ptr;
    log_set_tag("config_change");
    log_set_level(RAFT_LOG_INFO);
    log_set_level(RAFT_LOG_DEBUG);
    log_set_output(stdout);
    WooFMsgCacheInit();

    raft_lock(RAFT_LOCK_SERVER);
    RAFT_SERVER_STATE server_state = {0};
    if (WooFGet(RAFT_SERVER_STATE_WOOF, &server_state, 0) < 0) {
        log_error("failed to get the latest server state");
        WooFMsgCacheShutdown();
        raft_unlock(RAFT_LOCK_SERVER);
        exit(1);
    }
    int client_put_delay = server_state.timeout_min / 10;

    RAFT_CONFIG_CHANGE_RESULT result = {0};
    if (arg->observe) {
        if (member_id(arg->observer_woof, server_state.member_woofs) != -1) {
            log_debug("%s is already observing", arg->observer_woof);
            result.redirected = 0;
            result.success = 1;
            strcpy(result.current_leader, server_state.current_leader);
        } else {
            strcpy(server_state.member_woofs[RAFT_MAX_MEMBERS + server_state.observers], arg->observer_woof);
            server_state.observers += 1;
            unsigned long seq = WooFPut(RAFT_SERVER_STATE_WOOF, NULL, &server_state);
            if (WooFInvalid(seq)) {
                log_error("failed to add observer to server at term %" PRIu64 "", server_state.current_term);
                WooFMsgCacheShutdown();
                raft_unlock(RAFT_LOCK_SERVER);
                exit(1);
            }
            log_info("%s starts observing", arg->observer_woof);
            result.redirected = 0;
            result.success = 1;
            strcpy(result.current_leader, server_state.current_leader);
        }
    } else if (server_state.role != RAFT_LEADER) {
        log_debug("not a leader, reply with the current leader");
        result.redirected = 1;
        result.success = 0;
        strcpy(result.current_leader, server_state.current_leader);
    } else {
        if (server_state.current_config != RAFT_CONFIG_STATUS_STABLE) {
            log_debug("server is currently reconfiguring and not stable, rejected reconfiguring request");
            result.redirected = 0;
            result.success = 0;
            strcpy(result.current_leader, server_state.current_leader);
        } else {
            log_debug("processing new config with %d members", arg->members);

            // compute the joint config
            int joint_members;
            char joint_member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH];
            if (compute_joint_config(server_state.members,
                                     server_state.member_woofs,
                                     arg->members,
                                     arg->member_woofs,
                                     &joint_members,
                                     joint_member_woofs) < 0) {
                log_error("failed to compute the joint config");
                WooFMsgCacheShutdown();
                raft_unlock(RAFT_LOCK_SERVER);
                exit(1);
            }
            log_debug("there are %d members in the joint config", joint_members);

            // append a config entry to log
            RAFT_LOG_ENTRY entry = {0};
            entry.term = server_state.current_term;
            entry.is_config = RAFT_CONFIG_ENTRY_JOINT;
            entry.is_handler = 0;
            if (encode_config(entry.data.val, joint_members, joint_member_woofs) < 0) {
                log_error("failed to encode the joint config to a log entry");
                WooFMsgCacheShutdown();
                raft_unlock(RAFT_LOCK_SERVER);
                exit(1);
            }
            if (encode_config(entry.data.val + strlen(entry.data.val), arg->members, arg->member_woofs) < 0) {
                log_error("failed to encode the new config to a log entry");
                WooFMsgCacheShutdown();
                raft_unlock(RAFT_LOCK_SERVER);
                exit(1);
            }
            raft_lock(RAFT_LOCK_LOG);
            unsigned long entry_seq = WooFPut(RAFT_LOG_ENTRIES_WOOF, NULL, &entry);
            raft_unlock(RAFT_LOCK_LOG);
            if (WooFInvalid(entry_seq)) {
                log_error("failed to put the joint config as log entry");
                WooFMsgCacheShutdown();
                raft_unlock(RAFT_LOCK_SERVER);
                exit(1);
            }
            log_debug("appended the joint config as a log entry");

            // remove observers that are in the joint config
            int i;
            for (i = 0; i < server_state.observers; ++i) {
                int observer_id = member_id(server_state.member_woofs[RAFT_MAX_MEMBERS + i], joint_member_woofs);
                if (observer_id != -1 && observer_id < RAFT_MAX_MEMBERS) {
                    log_debug("removed %s from observer list", server_state.member_woofs[RAFT_MAX_MEMBERS + i]);
                    memset(server_state.member_woofs[RAFT_MAX_MEMBERS + i], 0, RAFT_NAME_LENGTH);
                }
            }
            server_state.observers = 0;
            for (i = 0; i < RAFT_MAX_OBSERVERS; ++i) {
                if (server_state.member_woofs[RAFT_MAX_MEMBERS + i][0] != 0) {
                    strcpy(server_state.member_woofs[RAFT_MAX_MEMBERS + server_state.observers],
                           server_state.member_woofs[RAFT_MAX_MEMBERS + i]);
                    server_state.observers += 1;
                }
            }

            // use the joint config
            server_state.members = joint_members;
            server_state.current_config = RAFT_CONFIG_STATUS_JOINT;
            server_state.last_config_seqno = entry_seq;
            memcpy(server_state.member_woofs, joint_member_woofs, RAFT_MAX_MEMBERS * RAFT_NAME_LENGTH);
            for (i = 0; i < RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS; ++i) {
                server_state.next_index[i] = entry_seq + 1;
                server_state.match_index[i] = 0;
                server_state.last_sent_timestamp[i] = 0;
            }
            unsigned long seq = WooFPut(RAFT_SERVER_STATE_WOOF, NULL, &server_state);
            if (WooFInvalid(seq)) {
                log_error("failed to update server config at term %" PRIu64 "", server_state.current_term);
                WooFMsgCacheShutdown();
                raft_unlock(RAFT_LOCK_SERVER);
                exit(1);
            }
            log_info("start using joint config with %d members at term %" PRIu64 "",
                     server_state.members,
                     server_state.current_term);
            result.redirected = 0;
            result.success = 1;
            strcpy(result.current_leader, server_state.current_leader);
        }
    }

    // for very slight chance that h_client_put is not queued in the same order of the element appended in
    // RAFT_CLIENT_PUT_ARG_WOOF
    unsigned long latest_result_seqno = WooFGetLatestSeqno(RAFT_CONFIG_CHANGE_RESULT_WOOF);
    while (latest_result_seqno != seq_no - 1) {
        log_warn("config_change result seqno not matching, waiting %lu", seq_no);
        usleep(client_put_delay * 1000);
        latest_result_seqno = WooFGetLatestSeqno(RAFT_CONFIG_CHANGE_RESULT_WOOF);
    }

    unsigned long result_seq = WooFPut(RAFT_CONFIG_CHANGE_RESULT_WOOF, NULL, &result);
    while (WooFInvalid(result_seq)) {
        log_warn("failed to write reconfig result, try again");
        usleep(client_put_delay * 1000);
        result_seq = WooFPut(RAFT_CONFIG_CHANGE_RESULT_WOOF, NULL, &result);
    }
    raft_unlock(RAFT_LOCK_SERVER);

    if (result_seq != seq_no) {
        log_error("config_change seqno %lu doesn't match result seno %lu", seq_no, result_seq);
        WooFMsgCacheShutdown();
        exit(1);
    }

    WooFMsgCacheShutdown();
    return 1;
}
