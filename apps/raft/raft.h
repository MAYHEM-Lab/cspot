#ifndef RAFT_H
#define RAFT_H

#include "woofc.h"

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>

#define RAFT_LEADER 0
#define RAFT_FOLLOWER 1
#define RAFT_CANDIDATE 2
#define RAFT_OBSERVER 3
#define RAFT_SHUTDOWN 4
#define RAFT_CONFIG_STATUS_STABLE 0
#define RAFT_CONFIG_STATUS_JOINT 1
#define RAFT_CONFIG_ENTRY_NOT 0
#define RAFT_CONFIG_ENTRY_JOINT 1
#define RAFT_CONFIG_ENTRY_NEW 2
#define RAFT_DEBUG_INTERRUPT_WOOF "raft_debug_interrupt.woof"
#define RAFT_LOG_ENTRIES_WOOF "raft_log_entries.woof"
#define RAFT_LOG_HANDLER_ENTRIES_WOOF "raft_log_entries_handler.woof"
#define RAFT_SERVER_STATE_WOOF "raft_server_state.woof"
#define RAFT_HEARTBEAT_WOOF "raft_heartbeat.woof"
#define RAFT_TIMEOUT_CHECKER_WOOF "raft_timeout_checker.woof"
#define RAFT_APPEND_ENTRIES_ARG_WOOF "raft_append_entries_arg.woof"
#define RAFT_APPEND_ENTRIES_RESULT_WOOF "raft_append_entries_result.woof"
#define RAFT_CLIENT_PUT_REQUEST_WOOF "raft_client_put_request.woof"
#define RAFT_CLIENT_PUT_ARG_WOOF "raft_client_put_arg.woof"
#define RAFT_CLIENT_PUT_RESULT_WOOF "raft_client_put_result.woof"
#define RAFT_CONFIG_CHANGE_ARG_WOOF "raft_config_change_arg.woof"
#define RAFT_CONFIG_CHANGE_RESULT_WOOF "raft_config_change_result.woof"
#define RAFT_REPLICATE_ENTRIES_WOOF "raft_replicate_entries.woof"
#define RAFT_REQUEST_VOTE_ARG_WOOF "raft_request_vote_arg.woof"
#define RAFT_REQUEST_VOTE_RESULT_WOOF "raft_request_vote_result.woof"
#define RAFT_MONITOR_NAME "raft"

#define RAFT_WOOF_HISTORY_SIZE_SHORT 256
#define RAFT_WOOF_HISTORY_SIZE_LONG 32768
#define RAFT_NAME_LENGTH WOOFNAMESIZE
#define RAFT_MAX_MEMBERS 16
#define RAFT_MAX_OBSERVERS 4
#define RAFT_MAX_ENTRIES_PER_REQUEST 8
#define RAFT_DATA_TYPE_SIZE 4096

#define RAFT_SAMPLING_RATE 0 // number of entries per sample

char raft_error_msg[256];

typedef struct raft_debug_interrupt_arg {
    uint64_t microsecond;
} RAFT_DEBUG_INTERRUPT_ARG;

typedef struct data_type {
    char val[RAFT_DATA_TYPE_SIZE];
} RAFT_DATA_TYPE;

typedef struct raft_log_entry {
    uint64_t term;
    RAFT_DATA_TYPE data;
    int32_t is_config;
    int32_t is_handler;
} RAFT_LOG_ENTRY;

typedef struct raft_log_handler_entry {
    char handler[RAFT_NAME_LENGTH];
    char ptr[RAFT_DATA_TYPE_SIZE - RAFT_NAME_LENGTH - sizeof(int32_t)];
    int32_t monitored;
} RAFT_LOG_HANDLER_ENTRY;

typedef struct raft_server_state {
    char woof_name[RAFT_NAME_LENGTH];
    int32_t role;
    uint64_t current_term;
    char voted_for[RAFT_NAME_LENGTH];
    char current_leader[RAFT_NAME_LENGTH];
    int32_t timeout_min;
    int32_t timeout_max;
    int32_t replicate_delay;

    int32_t members;
    int32_t observers;
    char member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH];
    int32_t current_config;
    uint64_t last_config_seqno;

    uint64_t commit_index;
    uint64_t next_index[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS];
    uint64_t match_index[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS];
    uint64_t last_sent_index[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS];
    uint64_t last_sent_timestamp[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS];
} RAFT_SERVER_STATE;

typedef struct raft_heartbeat {
    uint64_t term;
    uint64_t timestamp;
} RAFT_HEARTBEAT;

typedef struct raft_timeout_checker_arg {
    uint64_t timeout_value;
} RAFT_TIMEOUT_CHECKER_ARG;

typedef struct raft_append_entries_arg {
    uint64_t term;
    char leader_woof[RAFT_NAME_LENGTH];
    uint64_t prev_log_index;
    uint64_t prev_log_term;
    RAFT_LOG_ENTRY entries[RAFT_MAX_ENTRIES_PER_REQUEST];
    uint64_t leader_commit;
    uint64_t created_ts;
} RAFT_APPEND_ENTRIES_ARG;

typedef struct raft_append_entries_result {
    char server_woof[RAFT_NAME_LENGTH];
    uint64_t term;
    int32_t success;
    uint64_t last_entry_seq;
    uint64_t seqno;
    uint64_t request_created_ts;
} RAFT_APPEND_ENTRIES_RESULT;

typedef struct raft_client_put_request {
    RAFT_DATA_TYPE data;
    uint64_t created_ts;
    int32_t is_handler;
} RAFT_CLIENT_PUT_REQUEST;

typedef struct raft_client_put_arg {
    uint64_t last_seqno;
} RAFT_CLIENT_PUT_ARG;

typedef struct raft_client_put_result {
    int32_t redirected;
    uint64_t index;
    uint64_t term;
    char current_leader[RAFT_NAME_LENGTH];
} RAFT_CLIENT_PUT_RESULT;

typedef struct raft_config_change_arg {
    int32_t members;
    char member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH];

    int32_t observe;
    char observer_woof[RAFT_NAME_LENGTH];
} RAFT_CONFIG_CHANGE_ARG;

typedef struct raft_config_change_result {
    int32_t redirected;
    int32_t success;
    char current_leader[RAFT_NAME_LENGTH];
} RAFT_CONFIG_CHANGE_RESULT;

typedef struct raft_replicate_entries {
    uint64_t term;
    uint64_t last_seen_result_seqno;
    uint64_t last_ts;
} RAFT_REPLICATE_ENTRIES_ARG;

typedef struct raft_request_vote_arg {
    uint64_t term;
    char candidate_woof[RAFT_NAME_LENGTH];
    uint64_t candidate_vote_pool_seqno;
    uint64_t last_log_index;
    uint64_t last_log_term;
    uint64_t created_ts;
} RAFT_REQUEST_VOTE_ARG;

typedef struct raft_request_vote_result {
    uint64_t term;
    int32_t granted;
    uint64_t candidate_vote_pool_seqno;
    uint64_t request_created_ts;
    char granter[RAFT_NAME_LENGTH];
} RAFT_REQUEST_VOTE_RESULT;

int get_server_state(RAFT_SERVER_STATE* server_state);
uint64_t random_timeout(unsigned long seed, int min, int max);
int32_t member_id(char* woof_name, char member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH]);
int encode_config(char* dst, int members, char member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH]);
int decode_config(char* src, int* members, char member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH]);
int compute_joint_config(int old_members,
                         char old_member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH],
                         int new_members,
                         char new_member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH],
                         int* joint_members,
                         char joint_member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH]);
int threads_join(int members, pthread_t* pids);
int threads_cancel(int members, pthread_t* pids);

int raft_create_woofs();
int raft_start_server(int members,
                      char woof_name[RAFT_NAME_LENGTH],
                      char member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH],
                      int observer,
                      int timeout_min,
                      int timeout_max,
                      int replicate_delay);

typedef struct raft_blocked_nodes {
    char blocked_nodes[32][256];
} RAFT_BLOCKED_NODES;
typedef struct raftfailure_rate {
    int failed_percentage;
} RAFT_FAILURE_RATE;

#define RAFT_BLOCKED_NODES_WOOF "blocked_nodes.woof"
#define RAFT_FAILURE_RATE_WOOF "failure_rate.woof"
int raft_is_blocked(char* target, char* self, RAFT_BLOCKED_NODES blocked_nodes, RAFT_FAILURE_RATE failure_rate);

#endif