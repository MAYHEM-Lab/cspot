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

#define ARGS "W:i:o:d:p:n"
char *Usage = "dfaddnode -W woofname\n\
\t-i node-id\n\
\t-o node-opcode\n\
\t-d dest-node-id-for-result\n\
\t-p dest-port-id-for-result\n\
\t-n node-arity\n";

/*
 * add a node to the program woof
 */
int main(int argc, char **argv)
{
	DFNODE node;
	unsigned long seqno;
	int c;
	char prog_woof[4096];

	//node state is set to WAITING
	memset(prog_woof,0,sizeof(prog_woof));
	memset(&node,0,sizeof(node));
	node.dst_no = -1;
	node.node_no = -1;
	node.opcode = -1;
	node.total_val_ct = -1;
	node.dst_port = -1;
	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'W':
				strncpy(prog_woof,optarg,sizeof(prog_woof));
				strncat(prog_woof,".dfprogram",sizeof(prog_woof));
				break;
			case 'i':
				node.node_no = atoi(optarg);
				break;
			case 'o':
				node.opcode = atoi(optarg);
				break;
			case 'd':
				node.dst_no = atoi(optarg);
				break;
			case 'n':
				node.total_val_ct = atoi(optarg);
				break;
			case 'p':
				node.dst_port = atoi(optarg);
				break;
			default:
				fprintf(stderr,"ERROR -- dfaddnode: unrecognized command %c\n",
					(char)c);
				fprintf(stderr,"%s",Usage);
				exit(1);
			}
	}

	if(node.dst_no == -1) {
		fprintf(stderr,"ERROR -- dfaddnode: must specify dest node id\n");
		fprintf(stderr,"%s",Usage);
		exit(1);
	}
			
	if(node.node_no == -1) {
		fprintf(stderr,"ERROR -- dfaddnode: must specify node id\n");
		fprintf(stderr,"%s",Usage);
		exit(1);
	}

	if(node.opcode == -1) {
		fprintf(stderr,"ERROR -- dfaddnode: must specify an opcode\n");
		fprintf(stderr,"%s",Usage);
		exit(1);
	}
	
	if(node.dst_port == -1) {
		fprintf(stderr,"ERROR -- dfaddoperand: must specify dest node port\n");
		fprintf(stderr,"%s",Usage);
		exit(1);
	}

	if(prog_woof[0] == 0) {
		fprintf(stderr,"ERROR -- dfaddnode: must specify program woof name\n");
		fprintf(stderr,"%s",Usage);
		exit(1);
	}

	if(node.total_val_ct == -1) {
		fprintf(stderr,"ERROR -- dfaddnode: must specify input count\n");
		fprintf(stderr,"%s",Usage);
		exit(1);		
	}
	node.values = (double*)malloc(sizeof(double)*node.total_val_ct);

	WooFInit();	/* local only for now */

	seqno = WooFPut(prog_woof,NULL,&node);

	if(WooFInvalid(seqno)) {
		fprintf(stderr,
		"ERROR -- put to %s of node %d, opcode %d failed\n",
		prog_woof,
		node.node_no,
		node.opcode);
		exit(1);
	}

	exit(0);
		
}
