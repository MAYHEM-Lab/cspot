
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <pthread.h>
#include "fsema.h"


int InitSem(sema *s, int count)
{
	double r = drand48();

	if(s == NULL) {
		return(-1);
	}

	memset(s->mname,0,sizeof(s->mname));
	memset(s->wname,0,sizeof(s->wname));
	sprintf(s->mname,"/tmp/msema%0.10f",r);
	sprintf(s->wname,"/tmp/wsema%0.10f",r);
	s->value = count;
	s->waiters = 0;

	return(1);
}

void FreeSem(sema *s)
{
	unlink(s->wname);
	unlink(s->mname);
}

int GetLock(char *name)
{
	int fd = open(name,O_RDWR|O_CREAT,0600);
	if(fd < 0) {
		fprintf(stderr,"PANIC: %s\n",name);
		exit(1);
	}
	return(fd);
}

void P(sema *s)
{
	int mutex = GetLock(s->mname);
	int wait = GetLock(s->wname);

	flock(mutex,LOCK_EX);
	

	s->value--;

	while(s->value < 0) {
		/*
		 * maintain semaphore invariant
		 */
		if(s->waiters < (-1 * s->value)) {
			s->waiters++;
			flock(mutex,LOCK_UN);
			flock(wait,LOCK_EX);
			flock(mutex,LOCK_EX);
			s->waiters--;
		} else {
			if(s->value == 0) {
				flock(wait,LOCK_EX);
			}
			break;
		}
	}

	close(mutex);
	close(wait);
	return;
}

void V(sema *s)
{
	int mutex = GetLock(s->mname);
	int wait = GetLock(s->wname);
	
	flock(mutex,LOCK_EX);

	s->value++;

	if(s->value <= 0)
	{
		while(s->waiters > (-1 * s->value)) {
			flock(wait,LOCK_UN);
			flock(mutex,LOCK_UN);
			flock(mutex,LOCK_EX);
		}
	}

	if(s->value == 1) {
		flock(wait,LOCK_UN);
	}
	flock(mutex,LOCK_UN);

	close(mutex);
	close(wait);

	return;
		
		
}

