#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>

#include "woofc-priv.h"
#include "master-slave.h"

#define ARGS "W:"
char *Usage = "master-slave-pulse -W woof_name\n";

char Wname[4096];
char NameSpace[4096];
char Namelog_dir[4096];
char putbuf1[4096];
char putbuf2[4096];

#define MAX_RETRIES 20

int main(int argc, char **argv)
{
	int c;
	int i;
	int err;
	int uselocal;
	char wname[4096];
	char pulse_woof[4096];
	unsigned long pseq_no;
	unsigned long seq_no;
	PULSE pstc;

	memset(wname,0,sizeof(wname));
	uselocal = 0;

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

	if(Namelog_dir[0] != 0) {
		sprintf(putbuf2,"WOOF_NAMELOG_DIR=%s",Namelog_dir);
		putenv(putbuf2);
	}

	if(uselocal == 1) {
		WooFInit();
	}

	MAKE_EXTENDED_NAME(pulse_woof,wname,"pulse");

	pseq_no = WooFGetLatestSeqno(wname);
	if(WooFInvalid(pseq_no)) {
		fprintf(stderr,"master-slave-pulse: couldn't get seqno from %s\n",
			wname);
		fflush(stderr);
		exit(1);
	}

	memset(pstc.wname,0,sizeof(pstc.wname));
	strncpy(pstc.wname,wname,sizeof(pstc.wname));
	gettimeofday(&pstc.tm,NULL);
	pstc.last_seq_no = pseq_no;
	seq_no = WooFPut(pulse_woof,"MSPulseHandler",&pstc);

	if(WooFInvalid(seq_no)) {
		fprintf(stderr,"master-slave-pulse: couldn't put %lu from %s\n",
			pseq_no,
			pulse_woof);
		fflush(stderr);
		exit(1);
	}
	
	exit(0);
}

	

	
	
