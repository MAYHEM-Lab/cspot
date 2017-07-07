#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "woofc-obj1.h"

#define ARGS "c:f:s:"
char *Usage = "woofc-obj1-test -s size (in events)\n";

char Fname[4096];

int main(int argc, char **argv)
{
	int size;
	WOOF *wf_1;
	int err;
	OBJ_EL el;

	size = 5;

	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'f':
				strncpy(Fname,optarg,sizeof(Fname));
				break;
			case 'c':
				max_counter = atoi(optarg);
				break;
			case 's':
				size = atoi(optarg);
				break;
			default:
				fprintf(stderr,
				"unrecognized command %c\n",(char)c);
				fprintf(stderr,"%s",Usage);
				exit(1);
		}
	}

	WooFInit(1);


	wf_1 = WooFCreate("woofc-obj1",sizeof(OBJ_EL),size);

	if(wf_1 == NULL) {
		fprintf(stderr,"couldn't create wf_1\n");
		fflush(stderr);
		exit(1);
	}

	memset(el.string,0,sizeof(el.string));
	strcpy(el.string,"my first bark",sizeof(el.string));

	err = WooFPut(wf_1,(void *)&el);

	if(err < 0) {
		fprintf(stderr,"first WooFPut failed\n");
		fflush(stderr);
		exit(1);
	}

	WooFFree(wf_1);
	WooFExit();

	return(0);
}

