#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <sys/mman.h>

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
	sprintf(s->mname,"./linux/msema%0.10f",r);
	sprintf(s->wname,"./linux/wsema%0.10f",r);
//	sprintf(s->mname,"/tmp/msema%0.10f",r);
//	sprintf(s->wname,"/tmp/wsema%0.10f",r);
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
		perror("GetLock");
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
//			msync((void *)s,sizeof(*s),MS_SYNC);
			flock(mutex,LOCK_UN);
			flock(wait,LOCK_EX);
			flock(mutex,LOCK_EX);
			s->waiters--;
//			msync((void *)s,sizeof(*s),MS_SYNC);
		} else {
			if(s->value == 0) {
//				msync((void *)s,sizeof(*s),MS_SYNC);
				flock(wait,LOCK_EX);
			}
			break;
		}
	}

//	msync((void *)s,sizeof(*s),MS_SYNC);
	flock(mutex,LOCK_UN);
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
//			msync((void *)s,sizeof(*s),MS_SYNC);
			flock(wait,LOCK_UN);
			flock(mutex,LOCK_UN);
			flock(mutex,LOCK_EX);
		}
	}

	if(s->value == 1) {
//		msync((void *)s,sizeof(*s),MS_SYNC);
		flock(wait,LOCK_UN);
	}
//	msync((void *)s,sizeof(*s),MS_SYNC);
	flock(mutex,LOCK_UN);

	close(mutex);
	close(wait);

	return;
		
		
}

