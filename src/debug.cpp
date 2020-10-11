#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <debug.h>
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