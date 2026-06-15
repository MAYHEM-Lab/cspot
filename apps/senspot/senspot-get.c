#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#include "woofc.h"
#include "senspot.h"

#define ARGS "W:C:LS:"
char *Usage = "senspot-get -W woof_name for get\n\
\t-L use same namespace for source and target\n\
\t-C count <count of values to get at specifc seq_no\n\
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
	int recvd;
	int uselocal;
	unsigned char input_buf[4096];
	char *str;
	SENSPOT *spt;
	unsigned char *e_p;
	char wname[4096];
	unsigned long seq_no;
	unsigned long r_seq_no;
	unsigned long el_size;
	unsigned int count;

	memset(wname,0,sizeof(wname));
	seq_no = 0;
	uselocal = 0;
	count = 1; // default is 1

	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'W':
				strncpy(wname,optarg,sizeof(wname));
				break;
			case 'C':
				count = atoi(optarg);
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

	if(count < 0) {
		fprintf(stderr,"cannot specify negative count\n");
		fprintf(stderr,"usage: %s",Usage);
		exit(1);
	}


	if(seq_no == 0) {
		seq_no = WooFGetLatestSeqno(wname);
	}
	if(uselocal == 1) {
		// NULL says it is local
		el_size = WooFGetElSize(NULL,wname);
		if(el_size == (unsigned long)-1) {
			fprintf(stderr,
			"senspot-get: could not get element size for %s\n",
			wname);
			exit(1);
		}
		spt = (SENSPOT *)malloc(count * el_size); 
		if(spt == NULL) {
			fprintf(stderr,
			"senspot-get: no space for element\n");
			exit(1);
		}
		// for older platforms that do not have range
		if(count == 1) {
			recvd = WooFGet(wname,spt,seq_no);
		} else {
			recvd = WooFGetRange(wname,spt,seq_no,count);
		}
		if(recvd < 0) {
			fprintf(stderr,"senspot-get failed for %s\n",
			wname);
			fflush(stderr);
			exit(1);
		}
	} else {
		// let remote woof determine size
		// use MsgGet to avpid an extra GetElSize
		el_size = WooFMsgGetElSize(wname);
		if(el_size == (unsigned long)-1) {
			fprintf(stderr,
			"senspot-get: could not get element size for %s\n",
			wname);
			exit(1);
		}
		spt = (SENSPOT *)malloc(count*el_size);
		if(spt == NULL) {
			fprintf(stderr,
			"senspot-get: no space for element\n");
			exit(1);
		}
		// older platforms do not have range msg type
		if(count == 1) {
			recvd = WooFMsgGet(wname,spt,el_size,seq_no);
		} else {
			recvd = WooFGetRange(wname,spt,seq_no,count);
		}
		if(recvd < 0) {
			fprintf(stderr,"senspot-get failed for %s\n",
			wname);
			fflush(stderr);
			exit(1);
		}
	}


	r_seq_no = seq_no;
	e_p = (unsigned char *)spt;
//printf("recvd: %d\n",recvd);
	for(i=0; i < recvd; i++) {
		SenspotPrint((SENSPOT *)e_p,r_seq_no);
		r_seq_no++;
		e_p += el_size;
	}
	free(spt);

	exit(0);
}

	

	
	
