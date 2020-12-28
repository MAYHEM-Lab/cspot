#include "czmq.h"
#include "monitor.h"
#include "raft.h"
#include "raft_utils.h"
#include "woofc-access.h"
#include "woofc.h"

#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

int comp_index(const void* a, const void* b) {
    return *(unsigned long*)b - *(unsigned long*)a;
}

int h_update_commit_index(WOOF* wf, unsigned long seq_no, void* ptr) {
    log_set_tag("h_update_commit_index");
    log_set_level(RAFT_LOG_INFO);
    // log_set_level(RAFT_LOG_DEBUG);
    log_set_output(stdout);
    zsys_init();
    monitor_init();
    WooFMsgCacheInit();

    uint64_t begin = get_milliseconds();

    RAFT_UPDATE_COMMIT_INDEX_ARG arg = {0};
    if (monitor_cast(ptr, &arg, sizeof(RAFT_UPDATE_COMMIT_INDEX_ARG)) < 0) {
        log_error("failed to monitor_cast: %s", monitor_error_msg);
        WooFMsgCacheShutdown();
        exit(1);
    }
    seq_no = monitor_seqno(ptr);

    // get the server's current term and cluster members
    RAFT_SERVER_STATE server_state = {0};
    if (WooFGet(RAFT_SERVER_STATE_WOOF, &server_state, 0) < 0) {
        log_error("failed to get the server state");
        WooFMsgCacheShutdown();
        exit(1);
    }

    if (server_state.current_term != arg.term || server_state.role != RAFT_LEADER) {
        log_debug(
            "not a leader at term %" PRIu64 " anymore, current term: %" PRIu64 "", arg.term, server_state.current_term);
        monitor_exit(ptr);
        monitor_join();
        WooFMsgCacheShutdown();
        return 1;
    }

    // update commit_index using qsort
    unsigned long sorted_match_index[server_state.members];
    memcpy(sorted_match_index, server_state.match_index, sizeof(unsigned long) * server_state.members);
    qsort(sorted_match_index, server_state.members, sizeof(unsigned long), comp_index);
    int i;
    for (i = server_state.members / 2; i < server_state.members; ++i) {
        if (sorted_match_index[i] <= server_state.commit_index) {
            break;
        }
        RAFT_LOG_ENTRY entry = {0};
        if (WooFGet(RAFT_LOG_ENTRIES_WOOF, &entry, sorted_match_index[i]) < 0) {
            log_error("failed to get the log_entry at %lu", sorted_match_index[i]);
            WooFMsgCacheShutdown();
            exit(1);
        }
        if (entry.term == server_state.current_term && sorted_match_index[i] > server_state.commit_index) {
            // update commit_index
            server_state.commit_index = sorted_match_index[i];
            log_debug(
                "updated commit_index to %" PRIu64 " at %" PRIu64 "", server_state.commit_index, get_milliseconds());

            // check if joint config is commited
            if (server_state.current_config == RAFT_CONFIG_STATUS_JOINT &&
                server_state.commit_index >= server_state.last_config_seqno) {
                log_info("joint config is committed, appending new config");
                // append the new config
                RAFT_LOG_ENTRY config_entry = {0};
                if (WooFGet(RAFT_LOG_ENTRIES_WOOF, &config_entry, server_state.last_config_seqno) < 0) {
                    log_error("failed to get the commited config %" PRIu64 "", server_state.last_config_seqno);
                    WooFMsgCacheShutdown();
                    exit(1);
                }
                int new_members;
                char new_member_woofs[RAFT_MAX_MEMBERS][RAFT_NAME_LENGTH];
                int joint_config_len = decode_config(config_entry.data.val, &new_members, new_member_woofs);
                decode_config(config_entry.data.val + joint_config_len, &new_members, new_member_woofs);
                log_debug("there are %d members in the new config", new_members);

                // append a config entry to log
                RAFT_LOG_ENTRY new_config_entry = {0};
                new_config_entry.term = server_state.current_term;
                new_config_entry.is_config = RAFT_CONFIG_ENTRY_NEW;
                new_config_entry.is_handler = 0;
                if (encode_config(new_config_entry.data.val, new_members, new_member_woofs) < 0) {
                    log_error("failed to encode the new config to a log entry");
                    WooFMsgCacheShutdown();
                    exit(1);
                }
                unsigned long entry_seq = WooFPut(RAFT_LOG_ENTRIES_WOOF, NULL, &new_config_entry);
                if (WooFInvalid(entry_seq)) {
                    log_error("failed to put the new config as log entry");
                    WooFMsgCacheShutdown();
                    exit(1);
                }
                log_debug("appended the new config as a log entry");
                server_state.members = new_members;
                server_state.current_config = RAFT_CONFIG_STATUS_STABLE;
                server_state.last_config_seqno = entry_seq;
                memcpy(server_state.member_woofs, new_member_woofs, RAFT_MAX_MEMBERS * RAFT_NAME_LENGTH);
                int i;
                for (i = 0; i < RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS; ++i) {
                    server_state.next_index[i] = entry_seq + 1;
                    server_state.match_index[i] = 0;
                    server_state.last_sent_timestamp[i] = 0;
                    server_state.last_sent_request_seq[i] = 0;
                    server_state.acked_request_seq[i] = 0;
                }
                if (member_id(server_state.woof_name, server_state.member_woofs) < 0) {
                    // TODO shutdown
                    log_debug("not in new config, shutdown");
                    server_state.role = RAFT_SHUTDOWN;
                }
                unsigned long seq = WooFPut(RAFT_SERVER_STATE_WOOF, NULL, &server_state);
                if (WooFInvalid(seq)) {
                    log_error("failed to update server config at term %" PRIu64 "", server_state.current_term);
                    WooFMsgCacheShutdown();
                    exit(1);
                }
                log_info("start using new config with %d members", server_state.members);
                if (server_state.role == RAFT_SHUTDOWN) {
                    log_info("server not in the leader config anymore: SHUTDOWN");
                }
            }
            break;
        }
    }


    unsigned long seq = WooFPut(RAFT_SERVER_STATE_WOOF, NULL, &server_state);
    if (WooFInvalid(seq)) {
        log_error("failed to update server state");
        WooFMsgCacheShutdown();
        exit(1);
    }
    monitor_exit(ptr);
    seq = monitor_put(RAFT_MONITOR_NAME, RAFT_UPDATE_COMMIT_INDEX_WOOF, "h_update_commit_index", &arg, 1);
    if (WooFInvalid(seq)) {
        log_error("failed to queue the next h_update_commit_index handler");
        WooFMsgCacheShutdown();
        exit(1);
    }

    monitor_join();
    // printf("handler h_update_commit_index took %lu\n", get_milliseconds() - begin);
    WooFMsgCacheShutdown();
    return 1;
}
