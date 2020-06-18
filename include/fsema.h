#ifndef SEMA_H
#define SEMA_H

#include <pthread.h>
#include <sys/file.h>

struct sem {
    char mname[256];
    char wname[256];
    int value;
    int waiters;
};

typedef struct sem sema;

int InitSem(sema* s, int init_val);
void FreeSem(sema* s);
void P(sema* s);
void V(sema* s);

#endif
