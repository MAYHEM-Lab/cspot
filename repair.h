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

void LogInvalidByWooF(LOG *log);
int RBGLogMarkWooFDownstream(GLOG *glog, unsigned long host, char *woof_name, RB *seq_no);
int GLogMarkWooFDownstream(GLOG *glog, unsigned long host, char *woof_name, unsigned long *seq_no, int num_seq_no);
void GLogMarkEventDownstream(GLOG *glog, unsigned ndx, unsigned long cause_host, unsigned long cause_seq_no,
                             char *woof_name, unsigned long woof_seq_no);
void GLogFindRootEvent(GLOG *glog, unsigned long ndx, unsigned long *host, unsigned long *seq_no, unsigned long *root_ndx);
void GLogFindWooFHoles(GLOG *glog, unsigned long host, char *woof_name, Dlist *holes);
void GLogFindAffectedWooF(GLOG *glog, RB *root, int *count_root, RB *casualty, int *count_casualty, RB *progress, int *count_progress);
char *KeyFromWooF(unsigned long host, char *woof);
int WooFFromHval(char *key, unsigned long *host, char *woof);
char *KeyEventCause(unsigned long cause_host, unsigned long cause_seq_no);
char *KeyReadFromCause(unsigned long cause_host, char *woof_name, unsigned long woof_seq_no);

#endif