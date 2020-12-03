#ifndef CSPOT_DEBUG_H
#define CSPOT_DEBUG_H

#if defined(__cplusplus)
extern "C" {
#endif

void cspot_print_debug(const char* format, ...);
void cspot_print_fatal(const char* format, ...) __attribute__((noreturn));
void cspot_print_fatal_if(bool val, const char* format, ...);

#define DEBUG_LOG(...) cspot_print_debug(__VA_ARGS__)
#define DEBUG_WARN(...) cspot_print_debug(__VA_ARGS__)
#define DEBUG_FATAL(...) cspot_print_fatal(__VA_ARGS__)
#define DEBUG_FATAL_IF(cond, ...) cspot_print_fatal_if(cond, __VA_ARGS__)

#if defined(__cplusplus)
}
#endif

#endif