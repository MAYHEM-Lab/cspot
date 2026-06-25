#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#include "woofc.h"
#include "senspot.h"

#define ARGS "W:s:R:M:"
char *Usage = "senspot-init -W woof_name\n\
\t-s (history size in number of elements)\n\
	\t-M payload_size (default is 1K)\n\
\t-R reset-sequence-number (and do not init)\n";

char Wname[4096];
char NameSpace[4096];
char Namelog_dir[4096];
char putbuf1[4096];
char putbuf2[4096];
unsigned long New_seqno;

#define MAX_RETRIES 20

int main(int argc, char **argv)
{
	int c;
	int err;
	SENSPOT spt;
	char wname[4096];
	unsigned long history_size;
	int payload_size;

	memset(wname,0,sizeof(wname));
	history_size = 0;
	payload_size = 1024;

	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'M':
				payload_size = atoi(optarg);
				break;
			case 'W':
				strncpy(wname,optarg,sizeof(wname));
				break;
			case 's':
				history_size = atol(optarg);
				break;
			case 'R':
				New_seqno = strtoul(optarg,&New_seqno,10);
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

	if((history_size == 0) && (New_seqno == 0)) {
		fprintf(stderr,"must specify history size or new seqno for target object\n");
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		exit(1);
	}

	if(Namelog_dir[0] != 0) {
		sprintf(putbuf2,"WOOF_NAMELOG_DIR=%s",Namelog_dir);
		putenv(putbuf2);
	}

	WooFInit();

	if(history_size > 0) {
		err = WooFCreate(wname,sizeof(SENSPOT_HEADER) + payload_size,history_size);
		if(err < 0) {
			fprintf(stderr,"senspot-init failed for %s with history size %lu\n",
				wname,
				history_size);
			fflush(stderr);
			exit(1);
		}
	}

	if(New_seqno > 0) {
		err = WooFSetSeqno(wname,New_seqno);
		if(err < 0) {
		fprintf(stderr,"senspot-init failed to reset seqno to %lu for %s\n",
			New_seqno,
			wname);
			fflush(stderr);
			exit(1);
		}
	}



	exit(0);
}

	

	
	
