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
void GLogMarkWooFDownstream(GLOG *glog, unsigned long host, char *woof_name, unsigned long woofc_seq_no);
void GLogMarkWooFDownstreamRange(GLOG *glog, unsigned long host, char *woof_name, unsigned long woofc_seq_no_begin, unsigned long woofc_seq_no_end);
void GLogMarkEventDownstream(GLOG *glog, unsigned ndx, unsigned long cause_host, unsigned long cause_seq_no);
void GLogFindReplacedWooF(GLOG *glog, unsigned long host, char *woof_name, Dlist *holes);
void GLogFindAffectedWooF(GLOG *glog, RB *woof_put, int *count_woof_put, RB *woof_get, int *count_woof_get);
char *KeyFromWooF(unsigned long host, char *woof);
int WooFFromHval(char *key, unsigned long *host, char *woof);
#endif