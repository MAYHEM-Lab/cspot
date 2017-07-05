#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/file.h>

#include "mio.h"
#include "miotest.h"
#include "fsema.h"

FILE *Fd;

int main(int argc, char **argv)
{
	MIO *mio;
	MARG *marg;
	FILE *fd;

	chdir("/data");

	fd = fopen("/data/client.log","w");
	if(fd == NULL) {
		exit(1);
	}
	Fd = fd;

	fprintf(fd,"client running\n");
	fflush(fd);

	mio = MIOOpen("/data/test.mio","a+",4096);
	if(mio == NULL) {
		fprintf(fd,"client couldn't create mio\n");
		fflush(fd);
		exit(1);
	}

	fprintf(fd,"client mio open\n");
	fflush(fd);

	marg = (MARG *)MIOAddr(mio);

	fprintf(fd,"client: initial counter value: %d\n",
			marg->counter);
	fflush(fd);

	while(marg->counter <= 25) {
		MIOSync(mio);
		P(&marg->C);
		fprintf(fd,"client: counter %d -> ", marg->counter);
		fflush(fd);
		marg->counter++;
		fprintf(fd,"client: %d\n", marg->counter);
		fflush(fd);
		V(&marg->S);
		MIOSync(mio);
		sleep(1);
	}

	V(&marg->S);
	MIOSync(mio);
	fprintf(fd,"client exiting\n");
	fflush(fd);
	fclose(fd);


	return(0);
}
		



	


