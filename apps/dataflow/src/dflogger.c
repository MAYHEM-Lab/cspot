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


size_t get_string(const DF_VALUE* value, char value_string[], size_t value_length);

char* value_as_string(const DF_VALUE* value) {
    size_t required_value_size = get_string(value, NULL, 0);
    char* value_string = malloc(required_value_size + 1);
    get_string(value, value_string, required_value_size + 1);
    return value_string;
}

char* values_as_string(const DF_VALUE values[], const unsigned int value_count) {
    size_t required_value_size = 0;
    for (int i = 0; i < value_count; ++i) {
        required_value_size += get_string(&values[i], NULL, 0);
    }
    char* value_separator = ", ";
    size_t value_separator_size = strlen(value_separator) * (value_count - 1);

    size_t total_required_size = required_value_size + value_separator_size + 1;
    char* value_string = malloc(total_required_size);

    get_string(&values[0], value_string, total_required_size);
    for (int i = 1; i < value_count; ++i) {
        strcat(value_string, value_separator);
        size_t single_value_size = get_string(&values[i], NULL, 0);
        char single_value_string[single_value_size + 1];
        get_string(&values[i], single_value_string, single_value_size + 1);
        strcat(value_string, single_value_string);
    }
    return value_string;
}

size_t get_string(const DF_VALUE* value, char value_string[], const size_t value_length) {
    DF_TYPE value_type = value->type;
    if (value_type == DF_BOOLEAN) {
        if (value->value.df_int == 0) {
            return snprintf(value_string, value_length, "\"false\"");
        } else {
            return snprintf(value_string, value_length, "\"true\"");
        }
    } else if (value_type == DF_BYTE || value_type == DF_SHORT || value_type == DF_INTEGER) {
        return snprintf(value_string, value_length, "\"%d\"", value->value.df_int);
    } else if (value_type == DF_LONG) {
        return snprintf(value_string, value_length, "\"%ld\"", value->value.df_long);
    } else if (value_type == DF_UNSIGNED_BYTE || value_type == DF_UNSIGNED_SHORT || value_type == DF_UNSIGNED_INTEGER) {
        return snprintf(value_string, value_length, "\"%u\"", value->value.df_unsigned_int);
    } else if (value_type == DF_UNSIGNED_LONG) {
        return snprintf(value_string, value_length, "\"%lu\"", value->value.df_unsigned_long);
    } else if (value_type == DF_DOUBLE) {
        return snprintf(value_string, value_length, "\"%f\"", value->value.df_double);
    } else if (value_type == DF_STRING) {
        return snprintf(value_string, value_length, "\"%s\"", value->value.df_string);
    } else if (value_type == DF_DATETIME) {
        return snprintf(value_string, value_length, "\"TIMESTAMP - %lu\"", value->value.df_unsigned_long);
    } else if (value_type == DF_UNKNOWN) {
        return snprintf(value_string, value_length, "\"UNKNOWN VALUE - %p\"", value->value.df_object);
    } else {
        return 0;
    }
}