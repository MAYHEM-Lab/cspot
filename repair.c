#include <stdio.h>
#include <string.h>
#include "mio.h"
#include "log.h"
#include "event.h"
#include "host.h"
#include "dlist.h"

#include <pthread.h>
#include "lsema.h"
#include "repair.h"

void LogInvalidByWooF(LOG *log, unsigned long woofc_seq_no)
{
    EVENT *ev_array;
    int err;
    unsigned long curr;

    ev_array = (EVENT *)(((void *)log) + sizeof(LOG));

    curr = (log->tail + 1) % log->size;
    while (curr != (log->head + 1) % log->size)
    {
        if (ev_array[curr].woofc_seq_no == woofc_seq_no)
        {
            ev_array[curr].type = INVALID;
            LogInvalidByCause(log, (curr + 1) % log->size, ev_array[curr].host, ev_array[curr].seq_no);
            return;
        }
        curr = (curr + 1) % log->size;
    }
}

void LogInvalidByCause(LOG *log, unsigned ndx, unsigned long cause_host, unsigned long cause_seq_no)
{
    EVENT *ev_array;
    int err;
    unsigned long curr;

    ev_array = (EVENT *)(((void *)log) + sizeof(LOG));

    curr = ndx;
    while (curr != (log->head + 1) % log->size)
    {
        if (ev_array[curr].cause_host == cause_host && ev_array[curr].cause_seq_no == cause_seq_no)
        {
            ev_array[curr].type = INVALID;
            LogInvalidByCause(log, (curr + 1) % log->size, ev_array[curr].host, ev_array[curr].seq_no);
        }
        curr = (curr + 1) % log->size;
    }
}

void GLogMarkWooFDownstream(GLOG *glog, unsigned long host, char *woof_name, unsigned long woofc_seq_no)
{
    EVENT *ev_array;
    LOG *log;
    int err;
    unsigned long curr;

    log = glog->log;
    ev_array = (EVENT *)(((void *)log) + sizeof(LOG));

    curr = (log->tail + 1) % log->size;
    while (curr != (log->head + 1) % log->size)
    {
        if (ev_array[curr].host == host && ev_array[curr].woofc_seq_no == woofc_seq_no &&
            strcmp(ev_array[curr].woofc_name, woof_name) == 0)
        {
            ev_array[curr].type |= MARKED;
            GLogMarkEventDownstream(glog, (curr + 1) % log->size, ev_array[curr].host, ev_array[curr].seq_no);
            return;
        }
        curr = (curr + 1) % log->size;
    }
}

void GLogMarkWooFDownstreamRange(GLOG *glog, unsigned long host, char *woof_name, unsigned long woofc_seq_no_begin, unsigned long woofc_seq_no_end)
{
    unsigned long i;

    if (woofc_seq_no_begin > woofc_seq_no_end)
    {
        fprintf(stderr, "GLogMarkWooFDownstreamRange: invalid woofc_seq_no range: %lu %lu\n", woofc_seq_no_begin, woofc_seq_no_end);
        fflush(stderr);
        return;
    }

    for (i = woofc_seq_no_begin; i <= woofc_seq_no_end; i++)
    {
        GLogMarkWooFDownstream(glog, host, woof_name, i);
    }
}

void GLogMarkEventDownstream(GLOG *glog, unsigned ndx, unsigned long cause_host, unsigned long cause_seq_no)
{
    EVENT *ev_array;
    LOG *log;
    int err;
    unsigned long curr;

    log = glog->log;
    ev_array = (EVENT *)(((void *)log) + sizeof(LOG));

    curr = ndx;
    while (curr != (log->head + 1) % log->size)
    {
        if ( //ev_array[curr].type == TRIGGER &&
            ev_array[curr].cause_host == cause_host && ev_array[curr].cause_seq_no == cause_seq_no)
        {
            ev_array[curr].type |= MARKED;
            GLogMarkEventDownstream(glog, (curr + 1) % log->size, ev_array[curr].host, ev_array[curr].seq_no);
        }
        curr = (curr + 1) % log->size;
    }
}

void GLogFindReplacedWooF(GLOG *glog, unsigned long host, char *woof_name, Dlist *holes)
{
    EVENT *ev_array;
    LOG *log;
    int err;
    unsigned long curr;
    Hval value;

#ifdef DEBUG
    printf("GLogFindReplacedWooF: called host: %lu, woof_name: %s\n", host, woof_name);
    fflush(stdout);
#endif

    log = glog->log;
    ev_array = (EVENT *)(((void *)log) + sizeof(LOG));

    curr = (log->tail + 1) % log->size;
    while (curr != (log->head + 1) % log->size)
    {
        fflush(stdout);
        if (ev_array[curr].host == host &&
            (ev_array[curr].type == (TRIGGER | MARKED) || ev_array[curr].type == (APPEND | MARKED)) &&
            strcmp(ev_array[curr].woofc_name, woof_name) == 0)
        {
            if (holes->count == 0 || holes->last->value.i64 < ev_array[curr].woofc_seq_no)
            {
                value.i64 = ev_array[curr].woofc_seq_no;
                DlistAppend(holes, value);
            }
        }
        curr = (curr + 1) % log->size;
    }
}

void GLogFindAffectedWooF(GLOG *glog, RB *woof_put, int *count_woof_put, RB *woof_get, int *count_woof_get)
{
    EVENT *ev_array;
    LOG *log;
    unsigned long curr;
    char *key;
    RB *rbc;
    RB *rb;
    Hval hval;

#ifdef DEBUG
    printf("GLogFindAffectedWooF: called\n");
    fflush(stdout);
#endif

    log = glog->log;
    ev_array = (EVENT *)(((void *)log) + sizeof(LOG));

    curr = (log->tail + 1) % log->size;
    while (curr != (log->head + 1) % log->size)
    {
        if (ev_array[curr].type & MARKED)
        {
            if (ev_array[curr].type == (MARKED | TRIGGER) ||
                ev_array[curr].type == (MARKED | APPEND))
            {
                key = KeyFromWooF(ev_array[curr].host, ev_array[curr].woofc_name);
                if (RBFindS(woof_put, key) == NULL)
                {
                    RBInsertS(woof_put, key, (Hval)0);
                }
            }
            else if (ev_array[curr].type == (MARKED | READ))
            {
                key = KeyFromWooF(ev_array[curr].host, ev_array[curr].woofc_name);
                if (RBFindS(woof_get, key) == NULL)
                {
                    RBInsertS(woof_get, key, (Hval)0);
                }
            }
        }
        curr = (curr + 1) % log->size;
    }
    // tidy up
    *count_woof_put = 0;
    *count_woof_get = 0;
    RB_FORWARD(woof_put, rbc)
    {
        (*count_woof_put)++;
        rb = RBFindS(woof_get, rbc->key.key.s);
        if (rb != NULL)
        {
            RBDeleteS(woof_get, rb);
        }
    }
    RB_FORWARD(woof_get, rbc)
    {
        (*count_woof_get)++;
    }
#ifdef DEBUG
    printf("GLogFindAffectedWooF: count_woof_put: %d\n", count_woof_put);
    printf("GLogFindAffectedWooF: count_woof_get: %d\n", count_woof_get);
    fflush(stdout);
#endif
}

char *KeyFromWooF(unsigned long host, char *woof)
{
    char *str;
    int64_t hash = 5381;
    str = malloc(4096 * sizeof(char));
    sprintf(str, "%lu/%s", host, woof);
    return str;
}

int WooFFromHval(char *key, unsigned long *host, char *woof)
{
    char *ptr;
    int split;

#ifdef DEBUG
    printf("WooFFromHval called %s\n", key);
    fflush(stdout);
#endif

    ptr = strtok(key, "/");
    if (ptr == NULL)
    {
        return (-1);
    }
    *host = strtoul(ptr, NULL, 10);
    ptr = strtok(NULL, "/");
    sprintf(woof, "%s", ptr);

#ifdef DEBUG
    printf("WooFFromHval host: %lu, woof: %s\n", *host, woof);
    fflush(stdout);
#endif
    return (1);
}
