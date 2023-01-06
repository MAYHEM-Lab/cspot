//
// Created by lukas on 03.01.2023.
//
#include "dflogger.h"

#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

enum LOG_LEVELS CURRENT_LOG_LEVEL = DEBUG;
const char* PREFIX = "DFHandler";

void log_format(const char* tag, const char* message, va_list args) {
    time_t t = time(NULL);
    struct tm time = *gmtime(&t);
    printf("%s: %d-%02d-%02d %02d:%02d:%02d %s ",
           PREFIX,
           time.tm_year + 1900,
           time.tm_mon + 1,
           time.tm_mday,
           time.tm_hour,
           time.tm_min,
           time.tm_sec,
           tag);
    vprintf(message, args);
    printf("\n");
}

void log_trace(const char* message, ...) {
    if (CURRENT_LOG_LEVEL > TRACE) {
        return;
    }
    va_list args;
    va_start(args, message);
    log_format("[trace]", message, args);
    va_end(args);
}

void log_debug(const char* message, ...) {
    if (CURRENT_LOG_LEVEL > DEBUG) {
        return;
    }
    va_list args;
    va_start(args, message);
    log_format("[debug]", message, args);
    va_end(args);
}

void log_info(const char* message, ...) {
    if (CURRENT_LOG_LEVEL > INFO) {
        return;
    }
    va_list args;
    va_start(args, message);
    log_format("[info] ", message, args);
    va_end(args);
}

void log_warn(const char* message, ...) {
    if (CURRENT_LOG_LEVEL > WARN) {
        return;
    }
    va_list args;
    va_start(args, message);
    log_format("[warn] ", message, args);
    va_end(args);
}

void log_error(const char* message, ...) {
    va_list args;
    va_start(args, message);
    log_format("[error]", message, args);
    va_end(args);
}

char* values_as_string(double* values, unsigned int value_count) {
    size_t required_value_size = 0;
    for (int i = 0; i < value_count; ++i) {
        required_value_size += snprintf(NULL, 0, "\"%f\"", values[i]);
    }
    char* value_separator = ", ";
    size_t value_separator_size = strlen(value_separator) * (value_count - 1);

    size_t total_required_size = required_value_size + value_separator_size + 1;
    char* value_string = malloc(total_required_size);

    snprintf(value_string, total_required_size, "\"%f\"", values[0]);
    for (int i = 1; i < value_count; ++i) {
        size_t single_value_size = snprintf(NULL, 0, "\"%f\"", values[i]);
        char single_value_string[single_value_size + value_separator_size + 1];
        snprintf(single_value_string, single_value_size + value_separator_size + 1, "%s\"%f\"", value_separator, values[i]);
        strcat(value_string, single_value_string);
    }
    return value_string;
}
