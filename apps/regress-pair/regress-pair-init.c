#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#include "woofc.h"
#include "regress-pair.h"

#define ARGS "W:s:c:"
char *Usage = "regress-pair-init -W woof_name\n\
\t-c count-back (number of predicted elements to regress)\n\
\t-s (history size in number of elements)\n";

char NameSpace[4096];
char Namelog_dir[4096];
char putbuf1[4096];
char putbuf2[4096];

#define MAX_RETRIES 20

int main(int argc, char **argv)
{
	int c;
	int err;
	REGRESSINDEX ri;
	char wname[4096];
	char pred_name[4096+64];
	char measured_name[4096+64];
	char index_name[4096+64];
	char result_name[4096+64];
	int count_back;
	unsigned long seq_no;

	unsigned long history_size;

	memset(wname,0,sizeof(wname));
	history_size = 0;
	count_back = -1;

	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'W':
				strncpy(wname,optarg,sizeof(wname));
				break;
			case 's':
				history_size = atol(optarg);
				break;
			case 'c':
				count_back = atoi(optarg);
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

	if(history_size == 0) {
		fprintf(stderr,"must specify history size for target object\n");
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		exit(1);
	}

	if(count_back < 0) {
		fprintf(stderr,"must specify non-zero count back for predicted series\n");
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		exit(1);
	}

	MAKE_EXTENDED_NAME(pred_name,wname,"predicted");
	MAKE_EXTENDED_NAME(measured_name,wname,"measured");
	MAKE_EXTENDED_NAME(result_name,wname,"result");
	MAKE_EXTENDED_NAME(index_name,wname,"index");
	

	if(Namelog_dir[0] != 0) {
		sprintf(putbuf2,"WOOF_NAMELOG_DIR=%s",Namelog_dir);
		putenv(putbuf2);
	}

	WooFInit();

	/*
	 * first create the woofs to hold the data
	 */
	err = WooFCreate(wname,sizeof(REGRESSVAL),500); /* 500 to speed match */

	if(err < 0) {
		fprintf(stderr,"regress-pair-init failed for %s with history size %lu\n",
			wname,
			history_size);
		fflush(stderr);
		exit(1);
	}


	err = WooFCreate(pred_name,sizeof(REGRESSVAL),history_size);

	if(err < 0) {
		fprintf(stderr,"regress-pair-init failed for %s with history size %lu\n",
			pred_name,
			history_size);
		fflush(stderr);
		exit(1);
	}

	err = WooFCreate(measured_name,sizeof(REGRESSVAL),history_size);

	if(err < 0) {
		fprintf(stderr,"regress-pair-init failed for %s with history size %lu\n",
			measured_name,
			history_size);
		fflush(stderr);
		exit(1);
	}

	err = WooFCreate(result_name,sizeof(REGRESSVAL),history_size);

	if(err < 0) {
		fprintf(stderr,"regress-pair-init failed for %s with history size %lu\n",
			result_name,
			history_size);
		fflush(stderr);
		exit(1);
	}

	err = WooFCreate(index_name,sizeof(REGRESSINDEX),10); /* only 10 for index */

	if(err < 0) {
		fprintf(stderr,"regress-pair-init failed for %s with history size %lu\n",
			index_name,
			10);
		fflush(stderr);
		exit(1);
	}

	ri.count_back = count_back;
	seq_no = WooFPut(index_name,NULL,(void *)&ri);
	if(WooFInvalid(seq_no)) {
		fprintf(stderr,"regress-pair-init: couldn't write index\n");
		fflush(stderr);
	}
	

	exit(0);
}

	

	
	
