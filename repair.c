#include <stdio.h>

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

void GLogMarkWooFDownstream(GLOG *glog, unsigned long host, unsigned long woofc_seq_no)
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
        if (ev_array[curr].host == host && ev_array[curr].woofc_seq_no == woofc_seq_no)
        {
            ev_array[curr].type = MARKED;
            GLogMarkEventDownstream(glog, (curr + 1) % log->size, ev_array[curr].host, ev_array[curr].seq_no);
            return;
        }
        curr = (curr + 1) % log->size;
    }
}

void GLogMarkWooFDownstreamRange(GLOG *glog, unsigned long host, unsigned long woofc_seq_no_begin, unsigned long woofc_seq_no_end)
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
        GLogMarkWooFDownstream(glog, host, i);
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
        if (//ev_array[curr].type == TRIGGER &&
            ev_array[curr].cause_host == cause_host && ev_array[curr].cause_seq_no == cause_seq_no)
        {
            ev_array[curr].type = MARKED;
            GLogMarkEventDownstream(glog, (curr + 1) % log->size, ev_array[curr].host, ev_array[curr].seq_no);
        }
        curr = (curr + 1) % log->size;
    }
}

void GLogFindMarkedWooF(GLOG *glog, unsigned long host, Dlist *holes)
{
    EVENT *ev_array;
    LOG *log;
    int err;
    unsigned long curr;
    Hval value;

    log = glog->log;
    ev_array = (EVENT *)(((void *)log) + sizeof(LOG));

    curr = (log->tail + 1) % log->size;
    while (curr != (log->head + 1) % log->size)
    {
        if (ev_array[curr].host == host && ev_array[curr].type == MARKED)
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