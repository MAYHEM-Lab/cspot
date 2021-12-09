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

#define ARGS "W:d:V:1"
char *Usage = "dfaddoperand -W woofname\n\
\t-d dest-node-id-for-result\n\
\t-V value\n\
\t-1 <must be first operand to dest in non-commute case>\n";

/*
 * add a node to the program woof
 */
int main(int argc, char **argv)
{
	DFOPERAND operand;
	unsigned long seqno;
	int err;
	int c;
	char op_woof[1024];
	int val_present;

	memset(&operand,0,sizeof(node));
	memset(op_woof,0,sizeof(op_woof));
	operand.dst_no = -1;
	val_present = 0;
	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'W':
				strncpy(operand.prog_woof,optarg,sizeof(prog_woof));
				strncat(operand.prog_woof,".dfprogram",sizeof(operand.prog_woof));
				strncpy(op_woof,optarg,sizeof(op_woof));
				strncat(op_woof,".dfoperand",sizeof(op_woof));
				break;
			case 'd':
				operand.dst_no = atoi(optarg);
				break;
			case '1'
				operand.order = 1;
				break;
			case 'V':
				operand.value = atof(optarg);
				val_present = 1;
				break;
			default:
				fprintf(stderr,"ERROR -- dfaddoperand: unrecognized command %c\n",
					(char)c);
				fprintf(stderr,"%s",Usage);
				exit(1);
			}
	}

	if(operand.dst_no == -1) {
		fprintf(stderr,"ERROR -- dfaddoperand: must specify dest node id\n");
		fprintf(stderr,"%s",Usage);
		exit(1);
	}
			

	if(operand.prog_woof[0] == 0) {
		fprintf(stderr,"ERROR -- dfaddoperand: must specify woof name\n");
		fprintf(stderr,"%s",Usage);
		exit(1);
	}

	if(val_present == 0) {
		fprintf(stderr,"ERROR -- dfaddoperand: must specify operand value\n");
		fprintf(stderr,"%s",Usage);
		exit(1);
	}

	seqno = WooFPut(op_woof,"dfhandler",&operand);

	if(WooFInvalid(seqno)) {
		fprintf(stderr,
		"ERROR -- put to %s of operand with dest %d failed\n",
		op_woof,
		operand.dst_no);
		exit(1)
	}

	exit(0);
		
}
