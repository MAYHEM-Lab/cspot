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

int GLogMarkWooFDownstream(GLOG *glog, unsigned long host, char *woof_name, unsigned long woofc_seq_no)
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
        // only root woof can be repaired
        if (ev_array[curr].host == host && (ev_array[curr].type == TRIGGER || ev_array[curr].type == APPEND) &&
            ev_array[curr].woofc_seq_no == woofc_seq_no && strcmp(ev_array[curr].woofc_name, woof_name) == 0)
        {
            ev_array[curr].type |= ROOT;
            GLogMarkEventDownstream(glog, (curr + 1) % log->size, ev_array[curr].host, ev_array[curr].seq_no,
                                    ev_array[curr].woofc_name, ev_array[curr].woofc_seq_no);
            return (0);
        }
        curr = (curr + 1) % log->size;
    }
    return (-1);
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

void GLogMarkEventDownstream(GLOG *glog, unsigned ndx, unsigned long cause_host, unsigned long cause_seq_no,
                             char *woof_name, unsigned long woof_seq_no)
{
    EVENT *ev_array;
    LOG *log;
    int err;
    unsigned long curr;
    unsigned long root_host;
    unsigned long root_seq_no;
    unsigned long root_ndx;

    log = glog->log;
    ev_array = (EVENT *)(((void *)log) + sizeof(LOG));

    curr = ndx;
    while (curr != (log->head + 1) % log->size)
    {
        if (ev_array[curr].type == READ && ev_array[curr].host == cause_host &&
            strcmp(ev_array[curr].woofc_name, woof_name) == 0 && ev_array[curr].woofc_seq_no == woof_seq_no)
        {
            // this is a read-from dependency
#ifdef DEBUG
            printf("EVENT from host %lu seq_no %lu reads from woof %s %lu\n",
                   ev_array[curr].host, ev_array[curr].seq_no, ev_array[curr].woofc_name, ev_array[curr].woofc_seq_no);
            fflush(stdout);
#endif
            GLogFindRootEvent(glog, curr, &root_host, &root_seq_no, &root_ndx);
#ifdef DEBUG
            printf("EVENT's root host %lu seq_no %lu ndx %lu\n", root_host, root_seq_no, root_ndx);
            fflush(stdout);
#endif
            ev_array[root_ndx].type |= ROOT;
            GLogMarkEventDownstream(glog, (root_ndx + 1) % log->size, ev_array[root_ndx].host, ev_array[root_ndx].seq_no,
                                    ev_array[root_ndx].woofc_name, ev_array[root_ndx].woofc_seq_no);
        }
        if (ev_array[curr].cause_host == cause_host && ev_array[curr].cause_seq_no == cause_seq_no)
        {
            if (ev_array[curr].type == TRIGGER || ev_array[curr].type == APPEND)
            {
                ev_array[curr].type |= MARKED;
                GLogMarkEventDownstream(glog, (curr + 1) % log->size, ev_array[curr].host, ev_array[curr].seq_no,
                                        ev_array[curr].woofc_name, ev_array[curr].woofc_seq_no);
            }
            else
            {
                // read-only dependency (GetLatestSeqNo), won't trigger any downstream event
                ev_array[curr].type |= MARKED;
            }
        }
        curr = (curr + 1) % log->size;
    }
}

void GLogFindRootEvent(GLOG *glog, unsigned long ndx, unsigned long *host, unsigned long *seq_no, unsigned long *root_ndx)
{
    EVENT *ev_array;
    LOG *log;
    int err;
    unsigned long curr;
    unsigned long cause_host;
    unsigned long cause_seq_no;

#ifdef DEBUG
    printf("GLogFindRootEvent: called, ndx %lu\n", ndx);
    fflush(stdout);
#endif

    log = glog->log;
    ev_array = (EVENT *)(((void *)log) + sizeof(LOG));
    *host = ev_array[ndx].host;
    *seq_no = ev_array[ndx].seq_no;
    *root_ndx = ndx;
    cause_host = ev_array[ndx].cause_host;
    cause_seq_no = ev_array[ndx].cause_seq_no;

    curr = ndx - 1;
    if (curr >= log->size)
    {
        curr = log->size - 1;
    }
    while (curr != log->tail)
    {
        if (ev_array[curr].seq_no == 0)
        {
            break;
        }
        else if (ev_array[curr].host == cause_host && ev_array[curr].seq_no == cause_seq_no)
        {
            *host = ev_array[curr].host;
            *seq_no = ev_array[curr].seq_no;
            cause_host = ev_array[curr].cause_host;
            cause_seq_no = ev_array[curr].cause_seq_no;
            *root_ndx = curr;
        }
        curr = curr - 1;
        if (curr >= log->size)
        {
            curr = log->size - 1;
        }
    }
}

void GLogFindWooFHoles(GLOG *glog, unsigned long host, char *woof_name, Dlist *holes)
{
    EVENT *ev_array;
    LOG *log;
    int err;
    unsigned long curr;
    Hval value;

#ifdef DEBUG
    printf("GLogFindWooFHoles: called host: %lu, woof_name: %s\n", host, woof_name);
    fflush(stdout);
#endif

    log = glog->log;
    ev_array = (EVENT *)(((void *)log) + sizeof(LOG));

    curr = (log->tail + 1) % log->size;
    while (curr != (log->head + 1) % log->size)
    {
        if (ev_array[curr].host == host &&
            (ev_array[curr].type == (TRIGGER | MARKED) || ev_array[curr].type == (APPEND | MARKED) ||
             ev_array[curr].type == (TRIGGER | ROOT)) &&
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

void GLogFindAffectedWooF(GLOG *glog, RB *root, int *count_root, RB *casualty, int *count_casualty, RB *progress, int *count_progress)
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
        if (ev_array[curr].type & ROOT)
        {
            key = KeyFromWooF(ev_array[curr].host, ev_array[curr].woofc_name);
            if (RBFindS(root, key) == NULL)
            {
                RBInsertS(root, key, (Hval)0);
            }
        }
        else if (ev_array[curr].type & MARKED)
        {
            if (ev_array[curr].type == (MARKED | TRIGGER) ||
                ev_array[curr].type == (MARKED | APPEND))
            {
                key = KeyFromWooF(ev_array[curr].host, ev_array[curr].woofc_name);
                if (RBFindS(casualty, key) == NULL)
                {
                    RBInsertS(casualty, key, (Hval)0);
                }
            }
            else if (ev_array[curr].type == (MARKED | LATEST_SEQNO))
            {
                key = KeyFromWooF(ev_array[curr].host, ev_array[curr].woofc_name);
                if (RBFindS(progress, key) == NULL)
                {
                    RBInsertS(progress, key, (Hval)0);
                }
            }
        }
        curr = (curr + 1) % log->size;
    }
    // tidy up
    *count_root = 0;
    *count_casualty = 0;
    *count_progress = 0;
    RB_FORWARD(root, rbc)
    {
        (*count_root)++;
        rb = RBFindS(progress, rbc->key.key.s);
        if (rb != NULL)
        {
            RBDeleteS(progress, rb);
        }
        rb = RBFindS(casualty, rbc->key.key.s);
        if (rb != NULL)
        {
            RBDeleteS(casualty, rb);
        }
    }
    RB_FORWARD(casualty, rbc)
    {
        (*count_casualty)++;
        rb = RBFindS(progress, rbc->key.key.s);
        if (rb != NULL)
        {
            RBDeleteS(progress, rb);
        }
    }
    RB_FORWARD(progress, rbc)
    {
        (*count_progress)++;
    }
#ifdef DEBUG
    printf("GLogFindAffectedWooF: count_root: %d\n", count_root);
    printf("GLogFindAffectedWooF: count_casualty: %d\n", count_casualty);
    printf("GLogFindAffectedWooF: count_progress: %d\n", count_progress);
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
            mapping[2 * rbc->value.i] = ev_array[curr].woofc_ndx;
            mapping[2 * rbc->value.i + 1] = ev_array[curr].woofc_seq_no;
            rbc->value.i += 1;
        }
        curr = (curr + 1) % log->size;
    }

#ifdef DEBUG
    RB_FORWARD(asker, rb)
    {
        rbc = RBFindS(mapping_count, rb->key.key.s);
        if (rbc == NULL)
        {
            fprintf(stderr, "GLogFindLatestSeqnoAsker: couldn't find entry %s in mapping_count RB tree\n", key);
            fflush(stderr);
            return (-1);
        }
        printf("GLogFindLatestSeqnoAsker: Asker %s %d\n", rb->key.key.s, rbc->value.i);
        mapping = (unsigned long *)rb->value.i64;
        int i;
        for (i = 0; i < rbc->value.i; i++)
        {
            printf("GLogFindLatestSeqnoAsker:  %lu -> %lu\n", mapping[2 * i], mapping[2 * i + 1]);
        }
    }
    fflush(stdout);
#endif
    return (0);
}

char *KeyFromWooF(unsigned long host, char *woof)
{
    char *str;
    int64_t hash = 5381;
    str = malloc(4096 * sizeof(char));
    sprintf(str, "%lu_%s", host, woof);
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

    ptr = strtok(tmp, "_");
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
