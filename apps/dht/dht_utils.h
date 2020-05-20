#ifndef DHT_UTILS_H
#define DHT_UTILS_H

#include <stdio.h>

#define DHT_LOG_DEBUG 0
#define DHT_LOG_INFO 1
#define DHT_LOG_WARN 2
#define DHT_LOG_ERROR 3

unsigned long get_milliseconds();
int node_woof_name(char *woof_name);
void log_set_level(int level);
void log_set_output(FILE *file);
void log_set_tag(const char *tag);
void log_debug(const char *message, ...);
void log_info(const char *message, ...);
void log_warn(const char *message, ...);
void log_error(const char *message, ...);

#endif
