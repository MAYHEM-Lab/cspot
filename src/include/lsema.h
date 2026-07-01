#ifndef SEMA_H
#define SEMA_H

#define USE_POSIX_SEM // for backward woof compat

#include <stdint.h>

#ifdef USE_POSIX_SEM

#include <semaphore.h>
typedef sem_t sema;

#else

typedef struct {
    int32_t value;
} sema;

#endif

int InitSem(sema *s, int init_val);
void FreeSem(sema *s);
void P(sema *s);
void V(sema *s);
int GetSemValue(sema *s, int *val);

#endif

