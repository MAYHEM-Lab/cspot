#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "dht.h"
#include "dht_utils.h"

char log_tag[DHT_NAME_LENGTH];
FILE *log_output;
int log_level;

unsigned long get_milliseconds() {
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return (unsigned long)tv.tv_sec * 1000 + (unsigned long)tv.tv_usec / 1000;
}

int node_woof_name(char *woof_name) {
	char *str = getenv("WOOFC_NAMESPACE");
	char namespace[DHT_NAME_LENGTH];
	if (str == NULL) {
		getcwd(namespace, sizeof(namespace));
	} else {
		strncpy(namespace, str, sizeof(namespace));
	}
	char local_ip[25];
	if (WooFLocalIP(local_ip, sizeof(local_ip)) < 0) {
		sprintf(dht_error_msg, "no local IP");
		return -1;
	}
	sprintf(woof_name, "woof://%s%s", local_ip, namespace);
	return 0;
}

void log_set_tag(const char *tag) {
	strcpy(log_tag, tag);
}

void log_set_level(int level) {
	log_level = level;
}

void log_set_output(FILE *file) {
	log_output = file;
}

void log_debug(const char *message, ...) {
	if (log_level > DHT_LOG_DEBUG) {
		return;
	}
	time_t now;
	time(&now);
	va_list argptr;
    va_start(argptr, message);
	fprintf(log_output, "\033[0;34m");
	fprintf(log_output, "DEBUG| %.19s:%.3d [dht:%s]: ", ctime(&now), get_milliseconds()% 1000, log_tag);
    vfprintf(log_output, message, argptr);
	fprintf(log_output, "\033[0m\n");
    va_end(argptr);
}

void log_info(const char *message, ...) {
	if (log_level > DHT_LOG_INFO) {
		return;
	}
	time_t now;
	time(&now);
	va_list argptr;
    va_start(argptr, message);
	fprintf(log_output, "\033[0;32m");
	fprintf(log_output, "INFO | %.19s:%.3d [dht:%s]: ", ctime(&now), get_milliseconds()% 1000, log_tag);
    vfprintf(log_output, message, argptr);
	fprintf(log_output, "\033[0m\n");
    va_end(argptr);
}

void log_warn(const char *message, ...) {
	if (log_level > DHT_LOG_WARN) {
		return;
	}
	time_t now;
	time(&now);
	va_list argptr;
    va_start(argptr, message);
	fprintf(log_output, "\033[0;33m");
	fprintf(log_output, "WARN | %.19s:%.3d [dht:%s]: ", ctime(&now), get_milliseconds()% 1000, log_tag);
    vfprintf(log_output, message, argptr);
	fprintf(log_output, "\033[0m\n");
    va_end(argptr);
}

void log_error(const char *message, ...) {
	if (log_level > DHT_LOG_ERROR) {
		return;
	}
	time_t now;
	time(&now);
	va_list argptr;
    va_start(argptr, message);
	fprintf(log_output, "\033[0;31m");
	fprintf(log_output, "ERROR| %.19s:%.3d [dht:%s]: ", ctime(&now), get_milliseconds()% 1000, log_tag);
    vfprintf(log_output, message, argptr);
	fprintf(log_output, "\033[0m\n");
    va_end(argptr);
}
