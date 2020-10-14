#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <debug.h>
#include <exception>
#include <iostream>

void cspot_print_debug(const char* format, ...) {
    std::cerr << "[debug] ";
    va_list myargs;
    va_start(myargs, format);
    [[maybe_unused]] auto ret = vfprintf(stderr, format, myargs);
    if (format[strlen(format) - 1] != '\n') {
        std::cerr << '\n';
    }
    fflush(stderr);
    va_end(myargs);
}


void cspot_print_fatal(const char* format, ...) {
    std::cerr << "[fatal] ";
    va_list myargs;
    va_start(myargs, format);
    [[maybe_unused]] auto ret = vfprintf(stderr, format, myargs);
    if (format[strlen(format) - 1] != '\n') {
        std::cerr << '\n';
    }
    fflush(stderr);
    va_end(myargs);
    std::terminate();
}

void cspot_print_fatal_if(bool val, const char* format, ...) {
    if (val) {
        std::cerr << "[fatal] ";
        va_list myargs;
        va_start(myargs, format);
        [[maybe_unused]] auto ret = vfprintf(stderr, format, myargs);
        if (format[strlen(format) - 1] != '\n') {
            std::cerr << '\n';
        }
        fflush(stderr);
        va_end(myargs);
        std::terminate();
    }
}
