#ifndef WOOFC
#define WOOFC

#include "mio.h"
#include "lsema.h"

struct woof_stc
{
	char filename[4096];
	MIO *mio;
	int (*handler)(struct woof_stc *woof,
				   unsigned long long seq_no,
				   void *element);
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

WOOF *WooFCreate(char *name,
				 int (*handler)(WOOF *woof, unsigned long long seq_no, void *el),
				 unsigned long element_size,
				 unsigned long history_size);

int WooFPut(WOOF *wf, void *element);

#define DEFAULT_WOOF_DIR "cspot"

#endif
