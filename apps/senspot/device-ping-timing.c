#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>

#include "woofc.h"

#define ARGS "W:s:"
char *Usage = "device-ping-timing -W woof_name\n";

char Wname[4096];
char NameSpace[4096];
char Namelog_dir[4096];
char putbuf1[4096];
char putbuf2[4096];

#define MAX_RETRIES 20

int main(int argc, char **argv)
{
	int c;
	int err;
	double rtt;
	unsigned int payload;
	char wname[4096];
	unsigned long history_size;
	unsigned long seqno;

	memset(wname,0,sizeof(wname));
	history_size = 0;

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
		fprintf(stderr,"must specify filename for target object\n");
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		exit(1);
	}

	WooFInit();

	seqno = WooFGetLatestSeqno(wname);
	if(seqno == (unsigned long)-1){
		fprintf(stderr,"could not get last seqno from %s\n",
				wname);
		exit(1);
	}

	while(1) {
		err = WooFGet(wname,&rtt,seqno);
		if(err < 0) {
			break;
		}
		printf("rtt: %f\n",rtt);
		seqno--;
	}



	exit(0);
}

	

	
	
