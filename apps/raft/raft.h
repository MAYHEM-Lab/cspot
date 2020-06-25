#ifndef RAFT_H
#define RAFT_H

#include <pthread.h>
#include <stdio.h>
#include "woofc.h"

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

#define RAFT_WOOF_HISTORY_SIZE 65536
#define RAFT_NAME_LENGTH WOOFNAMESIZE
#define RAFT_MAX_MEMBERS 16
#define RAFT_MAX_OBSERVERS 4
#define RAFT_MAX_ENTRIES_PER_REQUEST 16
#define RAFT_DATA_TYPE_SIZE 4096
// #define RAFT_TIMEOUT_MIN 500
// #define RAFT_TIMEOUT_MAX 1000
#define RAFT_TIMEOUT_MIN 1000 // better greater than zeromq timeout to avoid unnecessary election
#define RAFT_TIMEOUT_MAX 2000
#define RAFT_HEARTBEAT_RATE (RAFT_TIMEOUT_MIN / 5)
#define RAFT_TIMEOUT_CHECKER_DELAY (RAFT_TIMEOUT_MIN / 5)
// #define RAFT_REPLICATE_ENTRIES_DELAY 20
#define RAFT_REPLICATE_ENTRIES_DELAY (RAFT_HEARTBEAT_RATE / 5) // needs to be smaller than RAFT_HEARTBEAT_RATE
#define RAFT_CLIENT_PUT_DELAY 50
#define RAFT_WARNING_LATENCY (RAFT_TIMEOUT_MIN / 2)

#define RAFT_SAMPLING_RATE 0 // number of entries per sample

char raft_error_msg[256];

typedef struct data_type {
	char val[RAFT_DATA_TYPE_SIZE];
} RAFT_DATA_TYPE;

typedef struct raft_log_entry {
	unsigned long term;
	RAFT_DATA_TYPE data;
	int is_config;
	int is_handler;
} RAFT_LOG_ENTRY;

typedef struct raft_log_handler_entry {
	char handler[RAFT_NAME_LENGTH];
	char ptr[RAFT_DATA_TYPE_SIZE - RAFT_NAME_LENGTH];
} RAFT_LOG_HANDLER_ENTRY;

typedef struct raft_server_state {
	char woof_name[RAFT_NAME_LENGTH];
	int role;
	unsigned long current_term;
	char voted_for[RAFT_NAME_LENGTH];
	char current_leader[RAFT_NAME_LENGTH];
	
	int members;
	int observers;
	char member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH];
	int current_config;
	unsigned long last_config_seqno;

	unsigned long commit_index;
	unsigned long next_index[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS];
	unsigned long match_index[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS];
	unsigned long last_sent_index[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS];
	unsigned long last_sent_timestamp[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS];
} RAFT_SERVER_STATE;

typedef struct raft_heartbeat {
	unsigned long term;
	unsigned long timestamp;
} RAFT_HEARTBEAT;

typedef struct raft_timeout_checker_arg {
	unsigned long timeout_value;
} RAFT_TIMEOUT_CHECKER_ARG;

typedef struct raft_append_entries_arg {
	unsigned long term;
	char leader_woof[RAFT_NAME_LENGTH];
	unsigned long prev_log_index;
	unsigned long prev_log_term;
	RAFT_LOG_ENTRY entries[RAFT_MAX_ENTRIES_PER_REQUEST];
	unsigned long leader_commit;
	unsigned long created_ts;
} RAFT_APPEND_ENTRIES_ARG;

typedef struct raft_append_entries_result {
	char server_woof[RAFT_NAME_LENGTH];
	unsigned long term;
	int success;
	unsigned long last_entry_seq;
	unsigned long seqno;
	unsigned long request_created_ts;
} RAFT_APPEND_ENTRIES_RESULT;

typedef struct raft_client_put_request {
	RAFT_DATA_TYPE data;
	unsigned long created_ts;
	int is_handler;
} RAFT_CLIENT_PUT_REQUEST;

typedef struct raft_client_put_arg {
	unsigned long last_seqno;
} RAFT_CLIENT_PUT_ARG;

typedef struct raft_client_put_result {
	int redirected;
	unsigned long index;
	unsigned long term;
	char current_leader[RAFT_NAME_LENGTH];
} RAFT_CLIENT_PUT_RESULT;

typedef struct raft_config_change_arg {
	int members;
	char member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH];

	int observe;
	char observer_woof[RAFT_NAME_LENGTH];
} RAFT_CONFIG_CHANGE_ARG;

typedef struct raft_config_change_result {
	int redirected;
	int success;
	char current_leader[RAFT_NAME_LENGTH];
} RAFT_CONFIG_CHANGE_RESULT;

typedef struct raft_replicate_entries {
	unsigned long term;
	unsigned long last_seen_result_seqno;
	unsigned long last_ts;
} RAFT_REPLICATE_ENTRIES_ARG;

typedef struct raft_request_vote_arg {
	unsigned long term;
	char candidate_woof[RAFT_NAME_LENGTH];
	unsigned long candidate_vote_pool_seqno;
	unsigned long last_log_index;
	unsigned long last_log_term;
	unsigned long created_ts;
} RAFT_REQUEST_VOTE_ARG;

typedef struct raft_request_vote_result {
	unsigned long term;
	int granted;
	unsigned long candidate_vote_pool_seqno;
	unsigned long request_created_ts;
	char granter[RAFT_NAME_LENGTH];
} RAFT_REQUEST_VOTE_RESULT;

int get_server_state(RAFT_SERVER_STATE *server_state);
int raft_is_leader();
int random_timeout(unsigned long seed);
int member_id(char *woof_name, char member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH]);
int encode_config(char *dst, int members, char member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH]);
int decode_config(char *src, int *members, char member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH]);
int compute_joint_config(int old_members, char old_member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH],
	int new_members, char new_member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH],
	int *joint_members, char joint_member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH]);
int threads_join(int members, pthread_t *pids);
int threads_cancel(int members, pthread_t *pids);
	
int raft_create_woofs();
int raft_start_server(int members, char member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH], int observer);

#endif