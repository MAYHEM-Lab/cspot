#ifndef WOOFC
#define WOOFC

#include "mio.h"
#include "lsema.h"
#include "log.h"

struct woof_stc
{
	char filename[2048];
	char handler_name[2048];
	MIO *mio;
	sema mutex;
	sema tail_wait;
	unsigned long long seq_no;
	unsigned long history_size;
	unsigned long head;
	unsigned long tail;
	unsigned long element_size;
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
int WooFPut(char *wf_name, char *wf_handler, void *element);
int WooFGet(WOOF *wf, void *element, unsigned long ndx);
int WooFGetTail(WOOF *wf, void *elements, int element_count);
unsigned long WooFEarliest(WOOF *wf);
unsigned long WooFLatest(WOOF *wf);
unsigned long WooFNext(WOOF *wf, unsigned long ndx);

#define DEFAULT_WOOF_DIR "./cspot/"
#define DEFAULT_WOOF_LOG_SIZE (10000)
#define WOOFNAMESIZE (25)

#endif

