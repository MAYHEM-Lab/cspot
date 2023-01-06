//
// Created by lukas on 03.01.2023.
//

#ifndef CSPOT_DFDEBUG_H
#define CSPOT_DFDEBUG_H

enum LOG_LEVELS
{
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR
};

void log_trace(const char* message, ...);
void log_debug(const char* message, ...);
void log_info(const char* message, ...);
void log_warn(const char* message, ...);
void log_error(const char* message, ...);

char* values_as_string(double values[], unsigned int value_count);

#endif // CSPOT_DFDEBUG_H
