#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/file.h>

#include "mio.h"
#include "miotest.h"
#include "fsema.h"

int main(int argc, char **argv)
{
	MIO *mio;
	MARG *marg;
	int ld;

//	mio = MIOOpen("./osx/test.mio","w+",4096);
	mio = MIOOpen("./linux/test.mio","w+",4096);
	if(mio == NULL) {
		fprintf(stderr,"couldn't create mio\n");
		fflush(stderr);
		exit(1);
	}

	ld = open("./linux/test.lock",O_RDWR|O_CREAT,0600);
	if(ld < 0) {
		perror("open:");
		exit(1);
	}

	marg = (MARG *)MIOAddr(mio);
	marg->counter = 0;
	MIOSync(mio);

	if(fork() == 0) {
	system("docker run -it -v /Users/rich/github/src/cspot/mio-test/linux:/data centos:7 /data/client-1");
//	system("cd osx; ./client");
		exit(0);
	}


	while(marg->counter <= 1000) {
		flock(ld,LOCK_EX);
		MIOSync(mio);
		printf("server: counter %d -> ", marg->counter);
		fflush(stdout);
		marg->counter++;
		printf("server %d\n", marg->counter);
		fflush(stdout);
		MIOSync(mio);
		flock(ld,LOCK_UN);
		sleep(1);
	}

	printf("server exiting\n");
	fflush(stdout);

	unlink("./linux/test.lock");

	return(0);
}
		



	


