#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/file.h>

#include "mio.h"
#include "miotest.h"

int main(int argc, char **argv)
{
	MIO *mio;
	MARG *marg;

	mio = MIOOpen("./osx/test.mio","w+",4096);
	if(mio == NULL) {
		fprintf(stderr,"couldn't create mio\n");
		fflush(stderr);
		exit(1);
	}


	marg = (MARG *)MIOAddr(mio);
//	InitSem(&marg->S,1);
//	InitSem(&marg->C,0);
	marg->counter = 0;
	marg->turn = 0;

	if(fork() == 0) {
//	system("docker run -i -v /Users/rich/github/src/cspot/mio-test/linux:/data centos:7 /data/client");
	system("cd osx; ./client");
		exit(0);
	}


	flock(mio->fd,LOCK_EX);
	while(marg->counter <= 10) {
		MIOSync(mio);
		if(marg->turn == 1) {
			flock(mio->fd,LOCK_UN);
			flock(mio->fd,LOCK_EX);
			continue;
		}
		printf("server: incrementing counter at %d\n",
			marg->counter);
		fflush(stdout);
		marg->counter++;
		marg->turn = 1;
	}
	flock(mio->fd,LOCK_UN);

	printf("server exiting\n");
	fflush(stdout);

	return(0);
}
		



	


