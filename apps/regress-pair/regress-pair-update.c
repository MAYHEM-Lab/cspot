#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#include "woofc.h"
#include "regress-pair.h"

#define ARGS "W:c:l:"
char *Usage = "regress-pair-update -W woof_name\n\
\t-c count-back (number of predicted elements to regress)\n\
\t -l max lags\n";

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
	char error_name[4096+64];
	char coeff_name[4096+64];
	char progress_name[4096+64];
	char finished_name[4096+64];
	int count_back;
	unsigned long seq_no;
	int max_lags;

	unsigned long history_size;

	memset(wname,0,sizeof(wname));
	history_size = 0;
	count_back = -1;
	max_lags = -1;

	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'W':
				strncpy(wname,optarg,sizeof(wname));
				break;
			case 'c':
				count_back = atoi(optarg);
				break;
			case 'l':
				max_lags = atoi(optarg);
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

	if(count_back < 0) {
		fprintf(stderr,"must specify non-zero count back for predicted series\n");
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		exit(1);
	}

	if(max_lags < 0) {
		fprintf(stderr,"must specify max_lags for model fitting\n");
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		exit(1);
	}

	MAKE_EXTENDED_NAME(index_name,wname,"index");
	

	if(Namelog_dir[0] != 0) {
		sprintf(putbuf2,"WOOF_NAMELOG_DIR=%s",Namelog_dir);
		putenv(putbuf2);
	}

	WooFInit();

	ri.count_back = count_back;
	ri.max_lags = max_lags;
	seq_no = WooFPut(index_name,NULL,(void *)&ri);
	if(WooFInvalid(seq_no)) {
		fprintf(stderr,"regress-pair-update: couldn't write index\n");
		fflush(stderr);
	}
	

	exit(0);
}

	

	
	
