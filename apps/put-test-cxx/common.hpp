#ifndef _COMMON_HPP_
#define _COMMON_HPP_

#include <stdint.h>

#include <math.h>
#include <stdio.h>
#include <time.h>
struct put_elem
{
    uint64_t stamps[5];
};

inline uint64_t get_time()
{
    struct timespec spec;

    clock_gettime(CLOCK_REALTIME, &spec);

    return spec.tv_sec * 1000 + spec.tv_nsec / 1000000;
}

#endif