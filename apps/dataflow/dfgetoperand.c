#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>


#include "woofc.h"
#include "df.h"

#define ARGS "W:S:"
char *Usage = "dfgetoperand -W woofname\n\
\t-S seqno\n";

/*
 * get and print an operand from the operand woof
 */
int main(int argc, char **argv)
{
	DFOPERAND operand;
	unsigned long seqno;
	int c;
	char op_woof[1024];
	int err;

	memset(&operand,0,sizeof(operand));
	memset(op_woof,0,sizeof(op_woof));
	seqno = 0;
	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'W':
				strncpy(op_woof,optarg,sizeof(op_woof));
				strncat(op_woof,".dfoperand",sizeof(op_woof));
				break;
			case 'S':
				seqno = atoi(optarg);
				break;
			default:
				fprintf(stderr,"ERROR -- dfgetoperand: unrecognized command %c\n",
					(char)c);
				fprintf(stderr,"%s",Usage);
				exit(1);
			}
	}


	if(op_woof[0] == 0) {
		fprintf(stderr,"ERROR -- dfgetoperand: must specify woof name\n");
		fprintf(stderr,"%s",Usage);
		exit(1);
	}

	WooFInit();	/* local only for now */

	if(seqno == 0) {
		seqno = WooFGetLatestSeqno(op_woof);
	}
	err = WooFGet(op_woof,&operand,seqno);

	if(err < 0) {
		fprintf(stderr,
		"ERROR -- get from %s of operand with seqno %lu failed\n",
		op_woof,
		seqno);
		exit(1);
	}

	printf("op_woof: %s ",op_woof);
	printf("dst_no: %d ",operand.dst_no);
	printf("dst_port: %d",operand.dst_port);
	printf("value: %f ",operand.value);

	exit(0);
		
}
