#ifndef CSPOT_DEBUG_H
#define CSPOT_DEBUG_H


#if defined(__cplusplus)
extern "C" {
#endif

void cspot_print_debug(const char* format, ...);
void cspot_print_fatal(const char* format, ...) __attribute__((noreturn));
void cspot_print_fatal_if(bool val, const char* format, ...);

void cspot_print_timing(const char* format, ...);

#define QUIET

//#define TRACK
//#define TIMING
//#define DEBUG

#ifdef QUIET
#undef TIMING
#undef DEBUG
#undef TRACK
#endif

#ifdef TIMING

#include <sys/time.h>
#define STARTCLOCK(startp) {struct timeval tm;\
			    gettimeofday(&tm,NULL);\
                            *startp=((double)((double)tm.tv_sec + (double)tm.tv_usec/1000000.0));}
#define STOPCLOCK(stopp) {struct timeval tm;\
		          gettimeofday(&tm,NULL);\
			  *stopp=((double)((double)tm.tv_sec + (double)tm.tv_usec/1000000.0));}
#define DURATION(start,stop) ((double)(stop-start))
#define TIMING_PRINT(...) cspot_print_timing(__VA_ARGS__)

#define DEBUG_LOG(...)
#define DEBUG_WARN(...)
#define DEBUG_FATAL(...)
#define DEBUG_FATAL_IF(cond, ...)

#else

#ifdef DEBUG
#define DEBUG_LOG(...) cspot_print_debug(__VA_ARGS__)
#define DEBUG_WARN(...) cspot_print_debug(__VA_ARGS__)
#define DEBUG_FATAL(...) cspot_print_fatal(__VA_ARGS__)
#define DEBUG_FATAL_IF(cond, ...) cspot_print_fatal_if(cond, __VA_ARGS__)
#else

#define DEBUG_LOG(...)
#define DEBUG_WARN(...)
#define DEBUG_FATAL(...)
#define DEBUG_FATAL_IF(cond, ...)

#endif

#define STARTCLOCK(startp)
#define STOPCLOCK(stopp)
#define DURATION(start,stop)
#define TIMING_PRINT(...) 

#endif

#if defined(__cplusplus)
}
#endif

#endif
