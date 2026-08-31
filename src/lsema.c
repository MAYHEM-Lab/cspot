#include "lsema.h"

#ifdef USE_POSIX_SEM

#include <errno.h>
#include <stdio.h>

int InitSem(sema *s, int count)
{
    if (sem_init(s, 1, count) < 0) {
        perror("InitSem");
        return -1;
    }

    return 1;
}

void FreeSem(sema *s)
{
    sem_destroy(s);
}

void P(sema *s)
{
    int err;

    do {
        err = sem_wait(s);
    } while (err < 0 && errno == EINTR);

    if (err < 0)
        perror("P");
}

void V(sema *s)
{
    if (sem_post(s) < 0)
        perror("V");
}

int GetSemValue(sema *s, int *val)
{
    return sem_getvalue(s, val);
}

#else   /* ---------- Linux futex implementation ---------- */

#include <errno.h>
#include <limits.h>
#include <linux/futex.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

static int futex_wait(int32_t *addr, int32_t expected)
{
    return syscall(SYS_futex,
                   addr,
                   FUTEX_WAIT,
                   expected,
                   NULL,
                   NULL,
                   0);
}

static int futex_wake(int32_t *addr, int count)
{
    return syscall(SYS_futex,
                   addr,
                   FUTEX_WAKE,
                   count,
                   NULL,
                   NULL,
                   0);
}

int InitSem(sema *s, int init_val)
{
    if (init_val < 0) {
        errno = EINVAL;
        return -1;
    }

    __atomic_store_n(&s->value,
                     init_val,
                     __ATOMIC_RELEASE);

    return 1;
}

void FreeSem(sema *s)
{
    (void)s;
}

void P(sema *s)
{
    int32_t cur;

    for (;;) {

        cur = __atomic_load_n(&s->value,
                              __ATOMIC_ACQUIRE);

        while (cur > 0) {

            if (__atomic_compare_exchange_n(
                    &s->value,
                    &cur,
                    cur - 1,
                    1,              /* weak CAS */
                    __ATOMIC_ACQUIRE,
                    __ATOMIC_RELAXED)) {

                return;
            }
        }

        if (futex_wait(&s->value, 0) < 0) {

            if (errno == EINTR || errno == EAGAIN)
                continue;

            perror("P");
            return;
        }
    }
}

void V(sema *s)
{
    int32_t old;

    old = __atomic_fetch_add(&s->value,
                             1,
                             __ATOMIC_RELEASE);

    if (old == INT_MAX) {

        __atomic_fetch_sub(&s->value,
                           1,
                           __ATOMIC_RELAXED);

        errno = EOVERFLOW;
        perror("V");
        return;
    }

    futex_wake(&s->value, 1);
}

int GetSemValue(sema *s, int *val)
{
    int32_t cur = __atomic_load_n(&s->value, __ATOMIC_ACQUIRE);
    *val = (int)cur;
    return 0;
}

#endif

