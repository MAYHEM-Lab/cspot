#ifndef CSPOT_DEBUG_H
#define CSPOG_DEBUG_H

#if defined(__cplusplus)
extern "C" {
#endif

void cspot_print_debug(const char* format, ...);

#define DEBUG_LOG(...) cspot_print_debug(__VA_ARGS__)

#if defined(__cplusplus)
}
#endif

#endif