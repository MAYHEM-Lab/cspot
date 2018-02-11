#ifndef WOOFC
#define WOOFC

#include "mio.h"
#include "lsema.h"
#include "log.h"

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
};

typedef struct woof_shared_stc WOOF_SHARED;

struct woof_stc {
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

int WooFCreate(char *name,
               unsigned long element_size,
               unsigned long history_size);
WOOF *WooFOpen(char *name);
unsigned long WooFPut(char *wf_name, char *wf_handler, void *element);
int WooFGet(char *wf_name, void *element, unsigned long seq_no);
unsigned long WooFAppend(WOOF *wf, char *wf_handler, void *element);
int WooFRead(WOOF *wf, void *element, unsigned long seq_no);
int WooFReadTail(WOOF *wf, void *elements, int element_count);
unsigned long WooFGetLatestSeqno(char *wf_name);
unsigned long WooFLatestSeqno(WOOF *wf);
unsigned long WooFEarliest(WOOF *wf);
unsigned long WooFLatest(WOOF *wf);
unsigned long WooFBack(WOOF *wf, unsigned long ndx, unsigned long elements);
unsigned long WooFForward(WOOF *wf, unsigned long ndx, unsigned long elements);
int WooFHandlerDone(char *wf_name, unsigned long seq_no);
int WooFInvalid(unsigned long seq_no);

#define DEFAULT_WOOF_DIR "./cspot/"
#define DEFAULT_CSPOT_HOST_DIR "./cspot-host/"
#define DEFAULT_HOST_ID (0)
#define DEFAULT_WOOF_LOG_SIZE (10000)
#define WOOFNAMESIZE (256)

#endif

