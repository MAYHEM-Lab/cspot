#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#include "woofc.h"
#include "senspot.h"

#define ARGS "W:LS:"
char *Usage = "senspot-get -W woof_name for get\n\
\t-L use same namespace for source and target\n\
\t-S seq_no <sequence number to get, latest if missing)\n";

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
	unsigned char input_buf[4096];
	char *str;
	SENSPOT spt;
	char wname[4096];
	unsigned long seq_no;

	memset(wname,0,sizeof(wname));
	seq_no = 0;
	uselocal = 0;

	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'W':
				strncpy(wname,optarg,sizeof(wname));
				break;
			case 'L':
				uselocal = 1;
				break;
			case 'S':
				seq_no = atol(optarg);
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


	if(seq_no == 0) {
		seq_no = WooFGetLatestSeqno(wname);
	}

	err = WooFGet(wname,&spt,seq_no);

	if(err < 0) {
		fprintf(stderr,"senspot-get failed for %s\n",
			wname);
		fflush(stderr);
		exit(1);
	}

	SenspotPrint(&spt,seq_no);


	exit(0);
}

	

	
	
