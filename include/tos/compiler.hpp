//
// Created by fatih on 5/29/18.
//

#pragma once

#if defined(TOS_ARCH_avr)
    #define TOS_MAIN __attribute__((OS_main))
    #define TOS_TASK __attribute__((OS_task))
#else
    #define TOS_MAIN
    #define TOS_TASK
#endif
#define NORETURN __attribute__((noreturn))
#define ALWAYS_INLINE __attribute__((always_inline)) inline
#define NO_INLINE __attribute__((noinline))

#define TOS_EXPORT __attribute__((visibility("default")))
#define TOS_HIDDEN __attribute__((visibility("hidden")))

#define WEAK __attribute__((weak))

#define PURE __attribute__((pure))

#define TOS_UNREACHABLE() __builtin_unreachable()