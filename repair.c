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

void GLogFindAffectedWooF(GLOG *glog, RB *woof_rw, int *count_woof_rw, RB *woof_ro, int *count_woof_ro)
{
    EVENT *ev_array;
    LOG *log;
    unsigned long curr;
    char *key;
    RB *rbc;
    RB *rb;

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
                if (RBFindS(woof_rw, key) == NULL)
                {
                    RBInsertS(woof_rw, key, (Hval)0);
                }
            }
            else if (ev_array[curr].type == (MARKED | LATEST_SEQNO))
            {
                key = KeyFromWooF(ev_array[curr].host, ev_array[curr].woofc_name);
                if (RBFindS(woof_ro, key) == NULL)
                {
                    RBInsertS(woof_ro, key, (Hval)0);
                }
            }
        }
        curr = (curr + 1) % log->size;
    }
    // tidy up
    *count_woof_rw = 0;
    *count_woof_ro = 0;
    RB_FORWARD(woof_rw, rbc)
    {
        (*count_woof_rw)++;
        rb = RBFindS(woof_ro, rbc->key.key.s);
        if (rb != NULL)
        {
            RBDeleteS(woof_ro, rb);
        }
    }
    RB_FORWARD(woof_ro, rbc)
    {
        (*count_woof_ro)++;
    }
#ifdef DEBUG
    printf("GLogFindAffectedWooF: count_woof_rw: %d\n", count_woof_rw);
    printf("GLogFindAffectedWooF: count_woof_ro: %d\n", count_woof_ro);
    fflush(stdout);
#endif
}

int GLogFindLatestSeqnoAsker(GLOG *glog, unsigned long host, char *woof_ro, RB *asker, RB *mapping_count)
{
    EVENT *ev_array;
    LOG *log;
    unsigned long curr;
    char *key;
    Hval hval;
    RB *rb;
    RB *rbc;
    unsigned long *mapping;

#ifdef DEBUG
    printf("GLogFindLatestSeqnoAsker: called on woof %s\n", woof_ro);
    fflush(stdout);
#endif

    log = glog->log;
    ev_array = (EVENT *)(((void *)log) + sizeof(LOG));

    curr = (log->tail + 1) % log->size;
    while (curr != (log->head + 1) % log->size)
    {
        if (ev_array[curr].host == host &&
            ev_array[curr].type == (MARKED | LATEST_SEQNO) &&
            strcmp(ev_array[curr].woofc_name, woof_ro) == 0)
        {
            key = KeyFromWooF(ev_array[curr].cause_host, ev_array[curr].woofc_handler);
            // printf("woofc_handler: %s     woofc_ndx: %lu\n", key, ev_array[curr].woofc_ndx);
            rb = RBFindS(asker, key);
            // TODO: use woofc_handler as woof_name of who calls WooFGetLatestSeqno
            if (rb == NULL)
            {
                hval.i = 1;
                RBInsertS(asker, key, hval);
            }
            else
            {
                rb->value.i++;
            }
        }
        curr = (curr + 1) % log->size;
    }

    RB_FORWARD(asker, rb)
    {
        printf("Asker: %s: %d\n", rb->key.key.s, rb->value.i);
        mapping = malloc(rb->value.i * 2 * sizeof(unsigned long));
        if (mapping == NULL)
        {
            fprintf(stderr, "GLogFindLatestSeqnoAsker: couldn't allocate space for seq_no mapping\n");
            fflush(stderr);
            return (-1);
        }
        hval.i = 0;
        RBInsertS(mapping_count, rb->key.key.s, hval);
        rb->value.i64 = (int64_t)mapping;
    }

    // second pass
    curr = (log->tail + 1) % log->size;
    while (curr != (log->head + 1) % log->size)
    {
        if (ev_array[curr].type == (MARKED | LATEST_SEQNO) && strcmp(ev_array[curr].woofc_name, woof_ro) == 0)
        {
            key = KeyFromWooF(ev_array[curr].cause_host, ev_array[curr].woofc_handler);
            // printf("woofc_handler: %s     woofc_ndx: %lu\n", key, ev_array[curr].woofc_ndx);
            rb = RBFindS(asker, key);
            if (rb == NULL)
            {
                fprintf(stderr, "GLogFindLatestSeqnoAsker: couldn't find entry %s in asker RB tree\n", key);
                fflush(stderr);
                return (-1);
            }
            rbc = RBFindS(mapping_count, key);
            if (rbc == NULL)
            {
                fprintf(stderr, "GLogFindLatestSeqnoAsker: couldn't find entry %s in mapping_count RB tree\n", key);
                fflush(stderr);
                return (-1);
            }
            mapping = (unsigned long *)rb->value.i64;
            mapping[rbc->value.i] = ev_array[curr].woofc_ndx;
            mapping[rbc->value.i + 1] = ev_array[curr].woofc_seq_no;
            rbc->value.i += 2;
        }
        curr = (curr + 1) % log->size;
    }

    ////////// TODO:
    // RB_FORWARD(asker, rb)
    // {
    //     printf("Asker: %s: %d\n", rb->key.key.s, rb->value.i);
    //     rbc = RBFindS(mapping_count, rb->key.key.s);
    //     if (rbc == NULL)
    //     {
    //         fprintf(stderr, "GLogFindLatestSeqnoAsker: couldn't find entry %s in mapping_count RB tree\n", key);
    //         fflush(stderr);
    //         return (-1);
    //     }
    //     mapping = (unsigned long *)rb->value.i64;
    //     int i;
    //     for (i = 0; i < rbc->value.i; i += 2)
    //     {
    //         printf("  %lu -> %lu\n", mapping[i], mapping[i + 1]);
    //     }
    // }
    // fflush(stdout);
    return (0);
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
    char tmp[4096];
    char *ptr;
    int split;

    memset(tmp, 0, sizeof(tmp));
    strncpy(tmp, key, strlen(key));

#ifdef DEBUG
    printf("WooFFromHval called %s\n", key);
    fflush(stdout);
#endif

    ptr = strtok(tmp, "/");
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
