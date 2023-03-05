#ifndef CSPOT_DEBUG_H
#define CSPOT_DEBUG_H


#if defined(__cplusplus)
extern "C" {
#endif

void cspot_print_debug(const char* format, ...);
void cspot_print_fatal(const char* format, ...) __attribute__((noreturn));
void cspot_print_fatal_if(bool val, const char* format, ...);

#define TIMING

#ifdef TIMING

#include <sys/time.h>
#define STARTCLOCK(startp) {struct timeval tm;\
			    gettimeofday(&tm,NULL);\
                            *startp=((double)((double)tm.tv_sec + (double)tm.tv_usec/1000000.0));}
#define STOPCLOCK(stopp) {struct timeval tm;\
		          gettimeofday(&tm,NULL);\
			  *stopp=((double)((double)tm.tv_sec + (double)tm.tv_usec/1000000.0));}
#define DURATION(start,stop) ((double)(stop-start))

#define DEBUG_LOG(...)
#define DEBUG_WARN(...)
#define DEBUG_FATAL(...)
#define DEBUG_FATAL_IF(cond, ...)

#else

#define DEBUG_LOG(...) cspot_print_debug(__VA_ARGS__)
#define DEBUG_WARN(...) cspot_print_debug(__VA_ARGS__)
#define DEBUG_FATAL(...) cspot_print_fatal(__VA_ARGS__)
#define DEBUG_FATAL_IF(cond, ...) cspot_print_fatal_if(cond, __VA_ARGS__)

#define STARTCLOCK()
#define STOPCLOCK()
#define DURATION(start,stop)

#endif

#if defined(__cplusplus)
}
#endif

#endif
