#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "woofc-host.h"
#include "hw.h"

#define ARGS "W:"
char *Usage = "hw -W woof_name\n";

char Wname[4096];

int main(int argc, char **argv)
{
	int c;
	int err;
	HW_EL el;
	unsigned long ndx;

	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'W':
				strncpy(Wname,optarg,sizeof(Wname));
				break;
			default:
				fprintf(stderr,
				"unrecognized command %c\n",(char)c);
				fprintf(stderr,"%s",Usage);
				exit(1);
		}
	}

	if(Wname[0] == 0) {
		fprintf(stderr,"must specify woof name\n");
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		exit(1);
	}

	WooFInit();


	err = WooFCreate(Wname,sizeof(HW_EL),5);
	if(err < 0) {
		fprintf(stderr,"couldn't create woof from %s\n",Wname);
		fflush(stderr);
		exit(1);
	}

	memset(el.string,0,sizeof(el.string));
	strncpy(el.string,"my first bark",sizeof(el.string));

	ndx = WooFPut(Wname,"hw",(void *)&el);

	if(WooFInvalid(ndx)) {
		fprintf(stderr,"first WooFPut failed for %s\n",Wname);
		fflush(stderr);
		exit(1);
	}

	return(0);
}

