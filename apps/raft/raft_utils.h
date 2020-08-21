#ifndef RAFT_UTILS_H
#define RAFT_UTILS_H

#include "raft.h"
#include "stdio.h"

#define RAFT_LOG_DEBUG 0
#define RAFT_LOG_INFO 1
#define RAFT_LOG_WARN 2
#define RAFT_LOG_ERROR 3

uint64_t get_milliseconds();
void node_woof_namespace(char* woof_namespace);
void log_set_tag(const char* tag);
void log_set_level(int level);
void log_set_output(FILE* file);
void log_debug(const char* message, ...);
void log_info(const char* message, ...);
void log_warn(const char* message, ...);
void log_error(const char* message, ...);
int read_config(FILE* fp,
                int* timeout_min,
                int* timeout_max,
                int *replicate_delay,
                char* name,
                int* members,
                char member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH]);

#endif