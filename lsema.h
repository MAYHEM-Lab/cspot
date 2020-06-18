#ifndef SEMA_H
#define SEMA_H

#include <pthread.h>
#include <semaphore.h>

typedef sem_t sema;

int InitSem(sema* s, int init_val);
void FreeSem(sema* s);
void P(sema* s);
void V(sema* s);

#endif
