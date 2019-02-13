#ifndef WOOFC
#define WOOFC

#define REPAIR

#include "mio.h"
#include "lsema.h"
#include "log.h"
#include "dlist.h"

struct woof_shared_stc
{
	char filename[2048];
	sema mutex;
	sema tail_wait;
	unsigned long long seq_no;
	unsigned long history_size;
	unsigned long head;
	unsigned long tail;
	unsigned long element_size;
#ifdef DONEFLAG
	unsigned long done;
#endif
#ifdef REPAIR
	int repair_mode;
#endif
};

typedef struct woof_shared_stc WOOF_SHARED;

#define NORMAL (0)
#define REPAIRING (1)
#define SHADOW (2)
#define READONLY (4)
#define REMOVE (8)

struct woof_stc
{
	WOOF_SHARED *shared;
	MIO *mio;
	unsigned long ino; // for caching
};

typedef struct woof_stc WOOF;

struct element_stc
{
	unsigned long busy;
	unsigned long long seq_no;
};

typedef struct element_stc ELID;

int WooFCreate(char *name, unsigned long element_size, unsigned long history_size);
WOOF *WooFOpen(char *name);
unsigned long WooFPut(char *wf_name, char *wf_handler, void *element);
int WooFGet(char *wf_name, void *element, unsigned long seq_no);
unsigned long WooFAppendWithCause(WOOF *wf, char *hand_name, void *element, unsigned long cause_host, unsigned long long cause_seq_no);
int WooFRead(WOOF *wf, void *element, unsigned long seq_no);
int WooFReadTail(WOOF *wf, void *elements, int element_count);
unsigned long WooFGetLatestSeqno(char *wf_name);
unsigned long WooFGetLatestSeqnoWithCause(char *wf_name, char *cause_woof_name, unsigned long cause_woof_latest_seq_no);
unsigned long WooFLatestSeqno(WOOF *wf);
unsigned long WooFLatestSeqnoWithCause(WOOF *wf, unsigned long cause_host, char *cause_woof_name, unsigned long cause_woof_latest_seq_no);
unsigned long WooFEarliest(WOOF *wf);
unsigned long WooFLatest(WOOF *wf);
unsigned long WooFBack(WOOF *wf, unsigned long ndx, unsigned long elements);
unsigned long WooFForward(WOOF *wf, unsigned long ndx, unsigned long elements);
int WooFHandlerDone(char *wf_name, unsigned long seq_no);
int WooFInvalid(unsigned long seq_no);
void WooFDrop(WOOF *wf);
int WooFTruncate(char *name, unsigned long seq_no);

#ifdef REPAIR
WOOF *WooFOpenOriginal(char *name);
int WooFRepairProgress(char *wf_name, unsigned long cause_host, char *cause_woof, int mapping_count, unsigned long *mapping);
int WooFRepair(char *wf_name, Dlist *seq_no);
int WooFShadowCreate(char *name, char *original_name, unsigned long element_size, unsigned long history_size, Dlist *seq_no);
int WooFShadowForward(WOOF *wf);
int WooFReplace(WOOF *dst, WOOF *src, unsigned long ndx, unsigned long size);
unsigned long WooFIndexFromSeqno(WOOF *wf, unsigned long seq_no);
void WooFPrintMeta(FILE *fd, char *name);
void WooFDump(FILE *fd, char *name);
#endif

#define DEFAULT_WOOF_DIR "./cspot/"
#define DEFAULT_CSPOT_HOST_DIR "./cspot-host/"
#define DEFAULT_HOST_ID (0)
// #define DEFAULT_WOOF_LOG_SIZE (10000)
#define DEFAULT_WOOF_LOG_SIZE (300000)
#define WOOFNAMESIZE (256)

#endif
