#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <debug.h>
#include <exception>
#include <iostream>
#include <unistd.h>

namespace {
auto cur_pid = getpid();
}

void cspot_print_debug(const char* format, ...) {
    std::cerr << "[" << cur_pid << "] " << "[debug] ";
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
    std::cerr << "[" << cur_pid << "] " << "[fatal] ";
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
        std::cerr << "[" << cur_pid << "] " << "[fatal] ";
        va_list myargs;
        va_start(myargs, format);
        [[maybe_unused]] auto ret = vfprintf(stderr, format, myargs);
        if (format[strlen(format) - 1] != '\n') {
            std::cerr << '\n' << std::flush;
        }
        fflush(stderr);
        va_end(myargs);
        std::terminate();
    }
}

void cspot_print_timing(const char* format, ...) {
    char pbuf[1024];
    int len;
    va_list myargs;
    va_start(myargs, format);
    sprintf(pbuf,"%d TIMING ",getpid());
    len = strlen(pbuf);
//    std::cerr << "[" << cur_pid << "] " << "[timing] ";
//    [[maybe_unused]] auto ret = vfprintf(stderr, format, myargs);
    [[maybe_unused]] auto ret = vsprintf(&pbuf[len], format, myargs);
    if (format[strlen(format) - 1] != '\n') {
	len = strlen(pbuf);
	pbuf[len] = '\n';
	pbuf[len+1] = 0;
    }
    std::cout << pbuf << std::flush;
//    fflush(stderr);
//    std::cerr << std::flush;
    va_end(myargs);
}
