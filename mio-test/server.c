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

//	mio = MIOOpen("./osx/test.mio","w+",4096);
	mio = MIOOpen("./linux/test.mio","w+",4096);
	if(mio == NULL) {
		fprintf(stderr,"couldn't create mio\n");
		fflush(stderr);
		exit(1);
	}


	marg = (MARG *)MIOAddr(mio);
	InitSem(&marg->S,1);
	InitSem(&marg->C,0);
	marg->counter = 0;
	MIOSync(mio);

	if(fork() == 0) {
	system("docker run -it -v /root/src/cspot/mio-test/linux:/data centos:7 /data/client");
//	system("docker run -it -v /Users/rich/github/src/cspot/mio-test/linux:/data centos:7 /data/client");
//	system("cd osx; ./client");
		exit(0);
	}


	while(marg->counter <= 25) {
		MIOSync(mio);
		P(&marg->S);
		printf("server: counter %d -> ", marg->counter);
		fflush(stdout);
		marg->counter++;
		printf("server %d\n", marg->counter);
		fflush(stdout);
		V(&marg->C);
		MIOSync(mio);
		sleep(1);
	}
	V(&marg->C);
	MIOSync(mio);

	printf("server exiting\n");
	fflush(stdout);

	return(0);
}
		



	


