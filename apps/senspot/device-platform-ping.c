#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#include "woofc.h"

#define ARGS "W:"
char *Usage = "device-platform-ping -W woof_name for ping test on device\n";

char Wname[4096];


int main(int argc, char **argv)
{
	int c;
	int err;
	char wname[4096];
	int randval;
	int gval;
	int pval;
	unsigned long seq_no;
	unsigned long last;

	memset(wname,0,sizeof(wname));

	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'W':
				strncpy(wname,optarg,sizeof(wname));
				break;
			default:
				fprintf(stderr,
				"unrecognized command %c\n",(char)c);
				fprintf(stderr,"%s",Usage);
				exit(1);
		}
	}

	if(wname[0] == 0) {
		fprintf(stderr,"must specify woof for target device\n");
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		exit(1);
	}

	randval = rand();

	/*
	 * test this with PingHandler
	 */
	seq_no = WooFPut(wname,"PingHandler",(void *)&randval);

	if(WooFInvalid(seq_no)) {
		fprintf(stderr,"device-platform-ping failed put for %s and value %d\n",
			wname,
			randval);
		fflush(stderr);
		exit(1);
	}

	err = WooFGet(wname,(void *)&gval,seq_no);

	if(err < 0) {
		fprintf(stderr,"device-platform-ping failed get for %s at seq_no %lu\n",
			wname,
			seq_no);
		fflush(stderr);
		exit(1);
	}

	if(randval != gval) {
		fprintf(stderr,"device-platform-ping sent %d but got %d for %s at seq_no %lu\n",
			randval,
			gval,
			wname,
			seq_no);
		fflush(stderr);
		exit(1);
	}

	last = WooFGetLatestSeqno(wname);

	if((long)last < 0) {
		fprintf(stderr,"device-platform-ping got last %d\n",
				last);
		exit(1);
	}

	/*
	 * if there is only one ping running against this device, then the last seqno
	 * should contain the sent value -1 because PingHandler ran and decremented it
	 */
	err = WooFGet(wname,(void *)&pval,last);
	if(err < 0) {
		fprintf(stderr,"device-platform-ping get of last %d failed\n",
				last);
		exit(1);
	}


	if(pval == (gval-1)) {
		printf("device-platform-ping: SUCCESS: sent %d and got %d, last: %d\n",
				randval,gval,last);
	} else {
		printf("device-platform-ping: WARNING: sent %d and got %d, pval: %d, last: %d\n",
				randval,gval,pval, last);
	}

	exit(0);
}

	

	
	
