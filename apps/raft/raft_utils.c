#include "raft_utils.h"

#include "raft.h"
#include "unistd.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

char log_tag[1024];
int log_level;
FILE* log_output;

uint64_t get_milliseconds() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000;
}

void node_woof_namespace(char* woof_namespace) {
    char* str = getenv("WOOFC_NAMESPACE");
    char namespace[RAFT_NAME_LENGTH];
    if (str == NULL) {
        getcwd(namespace, sizeof(namespace));
    } else {
        strncpy(namespace, str, sizeof(namespace));
    }
    strcpy(woof_namespace, namespace);
}

void log_set_tag(const char* tag) {
    strcpy(log_tag, tag);
}

void log_set_level(int level) {
    log_level = level;
}

void log_set_output(FILE* file) {
    log_output = file;
}

void log_debug(const char* message, ...) {
    if (log_output == NULL || log_level > RAFT_LOG_DEBUG) {
        return;
    }
    time_t now;
    time(&now);
    va_list argptr;
    va_start(argptr, message);
    fprintf(log_output, "\033[0;34m");
    fprintf(log_output, "DEBUG| %.19s:%.3d [raft:%s]: ", ctime(&now), (int)(get_milliseconds() % 1000), log_tag);
    vfprintf(log_output, message, argptr);
    fprintf(log_output, "\033[0m\n");
    va_end(argptr);
}

void log_info(const char* message, ...) {
    if (log_output == NULL || log_level > RAFT_LOG_INFO) {
        return;
    }
    time_t now;
    time(&now);
    va_list argptr;
    va_start(argptr, message);
    fprintf(log_output, "\033[0;32m");
    fprintf(log_output, "INFO | %.19s:%.3d [raft:%s]: ", ctime(&now), (int)(get_milliseconds() % 1000), log_tag);
    vfprintf(log_output, message, argptr);
    fprintf(log_output, "\033[0m\n");
    va_end(argptr);
}

void log_warn(const char* message, ...) {
    if (log_output == NULL || log_level > RAFT_LOG_WARN) {
        return;
    }
    time_t now;
    time(&now);
    va_list argptr;
    va_start(argptr, message);
    fprintf(log_output, "\033[0;33m");
    fprintf(log_output, "WARN | %.19s:%.3d [raft:%s]: ", ctime(&now), (int)(get_milliseconds() % 1000), log_tag);
    vfprintf(log_output, message, argptr);
    fprintf(log_output, "\033[0m\n");
    va_end(argptr);
}

void log_error(const char* message, ...) {
    if (log_output == NULL || log_level > RAFT_LOG_ERROR) {
        return;
    }
    time_t now;
    time(&now);
    va_list argptr;
    va_start(argptr, message);
    fprintf(log_output, "\033[0;31m");
    fprintf(log_output, "ERROR| %.19s:%.3d [raft:%s]: ", ctime(&now), (int)(get_milliseconds() % 1000), log_tag);
    vfprintf(log_output, message, argptr);
    fprintf(log_output, "\033[0m\n");
    va_end(argptr);
}

int read_config(FILE* fp,
                int* timeout_min,
                int* timeout_max,
                char* name,
                int* members,
                char member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_NAME_LENGTH]) {
    char buffer[256];
    if (fgets(buffer, sizeof(buffer), fp) == NULL) {
        sprintf(raft_error_msg, "wrong format of config file\n");
        return -1;
    }
    strcpy(name, buffer);
    if (fgets(buffer, sizeof(buffer), fp) == NULL) {
        sprintf(raft_error_msg, "wrong format of config file\n");
        return -1;
    }
    *timeout_min = (int)strtol(buffer, (char**)NULL, 10);
    if (fgets(buffer, sizeof(buffer), fp) == NULL) {
        sprintf(raft_error_msg, "wrong format of config file\n");
        return -1;
    }
    *timeout_max = (int)strtol(buffer, (char**)NULL, 10);
    if (fgets(buffer, sizeof(buffer), fp) == NULL) {
        sprintf(raft_error_msg, "wrong format of config file\n");
        return -1;
    }
    *members = (int)strtol(buffer, (char**)NULL, 10);
    if (*members == 0) {
        sprintf(raft_error_msg, "wrong format of config file\n");
        return -1;
    }
    memset(member_woofs, 0, (RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS) * RAFT_NAME_LENGTH);
    int i;
    for (i = 0; i < *members; ++i) {
        if (fgets(buffer, sizeof(buffer), fp) == NULL) {
            sprintf(raft_error_msg, "wrong format of config file\n");
            return -1;
        }
        buffer[strcspn(buffer, "\n")] = 0;
        if (buffer[strlen(buffer) - 1] == '/') {
            buffer[strlen(buffer) - 1] = 0;
        }
        strcpy(member_woofs[i], buffer);
    }
    return 0;
}