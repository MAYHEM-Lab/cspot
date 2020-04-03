#ifndef RAFT_H
#define RAFT_H

#include <sys/time.h>
#include <stdio.h>

#define RAFT_LEADER 0
#define RAFT_FOLLOWER 1
#define RAFT_CANDIDATE 2
#define RAFT_LOG_ENTRIES_WOOF "raft_log_entries_woof"
#define RAFT_SERVER_STATE_WOOF "raft_server_state_woof"
#define RAFT_REQUEST_VOTE_ARG_WOOF "raft_request_vote_arg_woof"
#define RAFT_REQUEST_VOTE_RESULT_WOOF "raft_request_vote_result_woof"
#define RAFT_REVIEW_REQUEST_VOTE_ARG_WOOF "raft_review_request_vote_arg_woof"
#define RAFT_APPEND_ENTRIES_ARG_WOOF "raft_append_entries_arg_woof"
#define RAFT_APPEND_ENTRIES_RESULT_WOOF "raft_append_entries_result_woof"
#define RAFT_REVIEW_APPEND_ENTRIES_ARG_WOOF "raft_review_append_entries_arg_woof"
#define RAFT_TERM_ENTRIES_WOOF "raft_term_entries_woof"
#define RAFT_REPLICATE_ENTRIES_WOOF "raft_replicate_entries_woof"
#define RAFT_TERM_CHAIR_ARG_WOOF "raft_chair_arg_woof"
#define RAFT_HEARTBEAT_ARG_WOOF "raft_heartbeat_arg_woof"
#define RAFT_WOOF_HISTORY_SIZE 65536
#define RAFT_WOOF_NAME_LENGTH 256
#define RAFT_MAX_SERVER_NUMBER 16
#define RAFT_MAX_ENTRIES_PER_REQUEST 8
#define RAFT_TIMEOUT_MIN 150
#define RAFT_TIMEOUT_MAX 300
#define RAFT_HEARTBEAT_RATE (RAFT_TIMEOUT_MIN / 2)
#define RAFT_LOOP_RATE 10
#define LOG_DEBUG 0
#define LOG_INFO 1
#define LOG_WARN 2
#define LOG_ERROR 3

typedef enum {false, true} bool;

typedef struct raft_log_entry {
	unsigned long term;
	int val;
} RAFT_LOG_ENTRY;

typedef struct raft_server_state {
	char woof_name[RAFT_WOOF_NAME_LENGTH];
	int role;
	unsigned long current_term;
	int members;
	char member_woofs[RAFT_MAX_SERVER_NUMBER][RAFT_WOOF_NAME_LENGTH];
} RAFT_SERVER_STATE;

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

typedef struct raft_review_request_vote_arg {
	unsigned long last_request_seqno;
	unsigned long term;
	char voted_for[RAFT_WOOF_NAME_LENGTH];
} RAFT_REVIEW_REQUEST_VOTE_ARG;

typedef struct raft_append_entries_arg {
	unsigned long term;
	char leader_woof[RAFT_WOOF_NAME_LENGTH];
	unsigned long prev_log_index;
	unsigned long prev_log_term;
	RAFT_LOG_ENTRY entries[RAFT_MAX_ENTRIES_PER_REQUEST];
	unsigned long created_ts;
} RAFT_APPEND_ENTRIES_ARG;

typedef struct raft_append_entries_result {
	char server_woof[RAFT_WOOF_NAME_LENGTH];
	unsigned long term;
	unsigned long next_index;
	bool success;
} RAFT_APPEND_ENTRIES_RESULT;

typedef struct raft_review_append_entries_arg {
	unsigned long last_request_seqno;
} RAFT_REVIEW_APPEND_ENTRIES_ARG;

typedef struct raft_term_entry {
	unsigned long term;
	int role;
} RAFT_TERM_ENTRY;

typedef struct raft_replicate_entries_arg {
	unsigned long term;
	unsigned long next_index[RAFT_MAX_SERVER_NUMBER];
	unsigned long match_index[RAFT_MAX_SERVER_NUMBER];
	unsigned long last_timestamp[RAFT_MAX_SERVER_NUMBER];
	unsigned long last_result_seqno;
} RAFT_REPLICATE_ENTRIES_ARG;

typedef struct raft_term_chair_arg {
	unsigned long last_term_seqno;
} RAFT_TERM_CHAIR_ARG;

typedef struct raft_heartbeat_arg {
	unsigned long term;
} RAFT_HEARTBEAT_ARG;

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

