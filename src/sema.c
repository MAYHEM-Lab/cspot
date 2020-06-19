
#include "sema.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int InitSem(sema* s, int count) {
    if (s == NULL) {
        return (-1);
    }
    s->value = count;
    s->waiters = 0;
    pthread_cond_init(&(s->wait), NULL);
    pthread_mutex_init(&(s->lock), NULL);

    return (1);
}

void FreeSem(sema* s) {
    free(s);
}

void P(sema* s) {
    pthread_mutex_lock(&(s->lock));

    s->value--;

    while (s->value < 0) {
        /*
         * maintain semaphore invariant
         */
        if (s->waiters < (-1 * s->value)) {
            s->waiters++;
            pthread_cond_wait(&(s->wait), &(s->lock));
            s->waiters--;
        } else {
            break;
        }
    }

    pthread_mutex_unlock(&(s->lock));

    return;
}

void V(sema* s) {

    pthread_mutex_lock(&(s->lock));

    s->value++;

    if (s->value <= 0) {
        pthread_cond_signal(&(s->wait));
    }

    pthread_mutex_unlock(&(s->lock));
}
