#ifndef CMQ_DISPATH_H
#define CMQ_DISPATCH_H

#include "sema.h"
#include "dlist.h"
#include <pthread.h>

// allocated by thread and sent in registration
struct cmq_dispatch_stc
{
	sema semaphore;
	int sd;
}

typedef struct cmq_dispatch_stc CMQDISPATCH;

extern sema *Dispatch_mutex;
extern Dlist *cmq_dispatch_thread_list;
		

#endif

