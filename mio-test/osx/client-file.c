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
	FILE *fd;

	fd = fopen("./client.log","w");
	if(fd == NULL) {
		exit(1);
	}

	fprintf(fd,"client running\n");
	fflush(fd);

	mio = MIOOpen("./test.mio","a+",4096);
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

	flock(mio->fd,LOCK_EX);
	while(marg->counter <= 10) {
		fprintf(fd,"client mio sync 1\n");
		fflush(fd);
		MIOSync(mio);
		fprintf(fd,"client testing turn: %d\n", marg->turn);
		fflush(fd);
		if(marg->turn == 0) {
			flock(mio->fd,LOCK_UN);
			flock(mio->fd,LOCK_EX);
			continue;
		}
		fprintf(fd,"client: incrementing counter at %d\n",
			marg->counter);
		fflush(fd);
		marg->counter++;
		marg->turn = 0;
	}
	flock(mio->fd,LOCK_UN);

	fprintf(fd,"client exiting\n");
	fflush(fd);
	fclose(fd);


	return(0);
}
		



	


