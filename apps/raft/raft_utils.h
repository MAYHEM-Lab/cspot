#ifndef RAFT_UTILS_H
#define RAFT_UTILS_H

#include "stdio.h"

#define RAFT_LOG_DEBUG 0
#define RAFT_LOG_INFO 1
#define RAFT_LOG_WARN 2
#define RAFT_LOG_ERROR 3

unsigned long get_milliseconds();
int node_woof_name(char *node_woof);
void log_set_tag(const char *tag);
void log_set_level(int level);
void log_set_output(FILE *file);
void log_debug(const char *message, ...);
void log_info(const char *message, ...);
void log_warn(const char *message, ...);
void log_error(const char *message, ...);

#endif