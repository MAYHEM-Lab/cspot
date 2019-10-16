#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include <pthread.h>
#include <semaphore.h>

#include "lsema.h"


int InitSem(sema *s, int count)
{
	int err;

	err = sem_init(s,1,count);

	if(err < 0) {
		perror("InitSem mutex");
		return(-1);
	}

	return(1);
}

void FreeSem(sema *s)
{
	sem_destroy(s);
}

void P(sema *s)
{
	int err;


	err = sem_wait(s);
	while((err < 0) && (errno == EINTR)) {
		err = sem_wait(s);
	}

	if(err < 0) {
		perror("P failed\n");
		return;
	}

	return;
}

void V(sema *s)
{
	
	sem_post(s);

	return;
}

