#ifndef REPAIR_H
#define REPAIR_H

#include <stdio.h>

#include "mio.h"
#include "log.h"
#include "event.h"
#include "host.h"
#include "dlist.h"

#include <pthread.h>
#include "lsema.h"

void LogInvalidByWooF(LOG *log, unsigned long woofc_seq_no);
void LogInvalidByCause(LOG *log, unsigned ndx, unsigned long cause_host, unsigned long cause_seq_no);
void GLogMarkWooFDownstream(GLOG *glog, unsigned long host, unsigned long woofc_seq_no);
void GLogMarkWooFDownstreamRange(GLOG *glog, unsigned long host, unsigned long woofc_seq_no_begin, unsigned long woofc_seq_no_end);
void GLogMarkEventDownstream(GLOG *glog, unsigned ndx, unsigned long cause_host, unsigned long cause_seq_no);
void GLogFindMarkedWooF(GLOG *glog, unsigned long host, Dlist *holes);
#endif