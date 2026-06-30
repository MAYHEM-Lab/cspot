#include "lsema.h"

#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int oldInitSem(sema* s, int count) {
    int err;

    err = sem_init(s, 1, count);

    if (err < 0) {
        perror("InitSem mutex");
        return (-1);
    }

    return (1);
}

int InitSem(sema* s, int count) {
    if (sem_init(s, 1, count) < 0) {
        int e = errno;
        fprintf(stdout, "InitSem failed count=%d errno=%d: %s\n",
                count, e, strerror(e));
        return -1;
    }

    return 1;
}

void FreeSem(sema* s) {
    sem_destroy(s);
}

void P(sema* s) {
    int err;

    do {
        errno = 0;
        err = sem_wait(s);
    } while (err < 0 && errno == EINTR);

    if (err < 0) {
        int e = errno;
        fprintf(stdout, "P failed: errno=%d: %s\n", e, strerror(e));
        return;
    }
}

void oldP(sema* s) {
    int err;

    err = sem_wait(s);
    while ((err < 0) && (errno == EINTR)) {
        err = sem_wait(s);
    }

    if (err < 0) {
        perror("P failed\n");
        return;
    }

    return;
}

void V(sema* s) {
    if (sem_post(s) < 0) {
        int e = errno;
        fprintf(stdout, "V failed: errno=%d: %s\n", e, strerror(e));
    }
}

void oldV(sema* s) {

    sem_post(s);

    return;
}
