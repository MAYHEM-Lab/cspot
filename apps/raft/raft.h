#ifndef RAFT_H
#define RAFT_H

#include <sys/time.h>
#include <stdio.h>

#define RAFT_LEADER 0
#define RAFT_FOLLOWER 1
#define RAFT_CANDIDATE 2
#define RAFT_LOG_ENTRIES_WOOF "raft_log_entries.woof"
#define RAFT_SERVER_STATE_WOOF "raft_server_state.woof"
#define RAFT_TERM_ENTRIES_WOOF "raft_term_entries.woof"
#define RAFT_APPEND_ENTRIES_ARG_WOOF "raft_append_entries_arg.woof"
#define RAFT_APPEND_ENTRIES_RESULT_WOOF "raft_append_entries_result.woof"
#define RAFT_REQUEST_VOTE_ARG_WOOF "raft_request_vote_arg.woof"
#define RAFT_REQUEST_VOTE_RESULT_WOOF "raft_request_vote_result.woof"
#define RAFT_FUNCTION_LOOP_WOOF "raft_function_loop.woof"
#define RAFT_HEARTBEAT_WOOF "raft_heartbeat.woof"
#define RAFT_CLIENT_PUT_ARG_WOOF "raft_client_put_arg.woof"
#define RAFT_CLIENT_PUT_RESULT_WOOF "client_put_result.woof"
#define RAFT_WOOF_HISTORY_SIZE 65536
#define RAFT_WOOF_NAME_LENGTH 256
#define RAFT_MAX_SERVER_NUMBER 16
#define RAFT_MAX_ENTRIES_PER_REQUEST 8
#define RAFT_TIMEOUT_MIN 150
#define RAFT_TIMEOUT_MAX 300
#define RAFT_HEARTBEAT_RATE (RAFT_TIMEOUT_MIN / 2)
#define RAFT_FUNCTION_LOOP_DELAY 0
#define LOG_DEBUG 0
#define LOG_INFO 1
#define LOG_WARN 2
#define LOG_ERROR 3

typedef enum {false, true} bool;
typedef struct data_type {
	char val[1024];
} DATA_TYPE;

typedef struct raft_log_entry {
	unsigned long term;
	DATA_TYPE data;
} RAFT_LOG_ENTRY;

typedef struct raft_server_state {
	char woof_name[RAFT_WOOF_NAME_LENGTH];
	int role;
	unsigned long current_term;
	char voted_for[RAFT_WOOF_NAME_LENGTH];
	unsigned long commit_index;
	int members;
	char member_woofs[RAFT_MAX_SERVER_NUMBER][RAFT_WOOF_NAME_LENGTH];
} RAFT_SERVER_STATE;

typedef struct raft_term_entry {
	unsigned long term;
	int role;
} RAFT_TERM_ENTRY;

typedef struct raft_request_vote_arg {
	unsigned long term;
	char candidate_woof[RAFT_WOOF_NAME_LENGTH];
	unsigned long candidate_vote_pool_seqno;
	unsigned long last_log_index;
	unsigned long last_log_term;
} RAFT_REQUEST_VOTE_ARG;

typedef struct raft_request_vote_result {
	unsigned long term;
	bool granted;
	unsigned long candidate_vote_pool_seqno;
} RAFT_REQUEST_VOTE_RESULT;

typedef struct raft_append_entries_arg {
	unsigned long term;
	char leader_woof[RAFT_WOOF_NAME_LENGTH];
	unsigned long prev_log_index;
	unsigned long prev_log_term;
	RAFT_LOG_ENTRY entries[RAFT_MAX_ENTRIES_PER_REQUEST];
	unsigned long leader_commit;
	unsigned long created_ts;
} RAFT_APPEND_ENTRIES_ARG;

typedef struct raft_append_entries_result {
	char server_woof[RAFT_WOOF_NAME_LENGTH];
	unsigned long term;
	unsigned long next_index;
	bool success;
} RAFT_APPEND_ENTRIES_RESULT;

typedef struct raft_review_client_put {
	unsigned long last_request_seqno;
} RAFT_REVIEW_CLIENT_PUT;

typedef struct raft_replicate_entries {
	unsigned long next_index[RAFT_MAX_SERVER_NUMBER];
	unsigned long match_index[RAFT_MAX_SERVER_NUMBER];
	unsigned long commit_index[RAFT_MAX_SERVER_NUMBER];
	unsigned long last_timestamp[RAFT_MAX_SERVER_NUMBER];
	unsigned long last_result_seqno;
} RAFT_REPLICATE_ENTRIES;

typedef struct raft_function_loop {
	unsigned long last_reviewed_append_entries;
	unsigned long last_reviewed_request_vote;
	unsigned long last_reviewed_client_put;
	unsigned long last_reviewed_term_chair;
	RAFT_REPLICATE_ENTRIES replicate_entries;
	char next_invoking[RAFT_WOOF_NAME_LENGTH]; // for debugging purpose
} RAFT_FUNCTION_LOOP;

typedef struct raft_heartbeat {

} RAFT_HEARTBEAT;

typedef struct raft_client_put_arg {
	DATA_TYPE data;
} RAFT_CLIENT_PUT_ARG;

typedef struct raft_client_put_result {
	bool success;
	unsigned long seq_no;
	unsigned long term;
} RAFT_CLIENT_PUT_RESULT;

int random_timeout(unsigned long seed);
unsigned long get_milliseconds();
int node_woof_name(char *node_woof);

char log_msg[1024];
void log_set_level(int level);
void log_set_output(FILE *file);
void log_info(const char *tag, const char *message);
void log_warn(const char *tag, const char *message);
void log_error(const char *tag, const char *message);
void log_debug(const char *tag, const char *message);

#endif

