#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>

#include "sema.h"
#include "cmq-pkt.h"

sema *Dispatch_mutex;
Dlist *cmq_dispatch_thread_list;

int cmq_dispatch_init()
{
	Dispatch_mutex = (sema *)malloc(sizeof(sema));
	if(Dispatch_mutex == NULL) {
		return(-1);
	}

	cmq_dispatch_thread_list = DlistInit();
	if(cmq_dispatch_thread_list == NULL) {
		FreeSem(Dispatch_mutex);
		return(-1);
	}	

	return(0);
}

	

	
