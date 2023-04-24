#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "hw.h"

#define ARGS "f:N:W:"
char *Usage = "hw-start -W woof_name\n\
\t-N namespace <CWD is the default>\n";

char Fname[4096];
char Wname[4096];
char NameSpace[4096];
char Namelog_dir[4096];
int UseNameSpace;

char putbuf1[1024];
char putbuf2[1024];

int main(int argc, char **argv)
{
	int c;
	int err;
	HW_EL el;
	unsigned long long ndx;

	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'f':
			case 'W':
				strncpy(Fname,optarg,sizeof(Fname));
				break;
			case 'N':
				UseNameSpace = 1;
				strncpy(NameSpace,optarg,sizeof(NameSpace));
				break;
			default:
				fprintf(stderr,
				"unrecognized command %c\n",(char)c);
				fprintf(stderr,"%s",Usage);
				exit(1);
		}
	}

	if(Fname[0] == 0) {
		fprintf(stderr,"must specify filename for woof\n");
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		exit(1);
	}

	if(Namelog_dir[0] != 0) {
		sprintf(putbuf2,"WOOF_NAMELOG_DIR=%s",Namelog_dir);
		putenv(putbuf2);
	}

	if(UseNameSpace == 1) {
		sprintf(Wname,"woof://%s/%s",NameSpace,Fname);
		sprintf(putbuf1,"WOOFC_DIR=%s",NameSpace);
		putenv(putbuf1);
	} else {
		strncpy(Wname,Fname,sizeof(Wname));
	}

	WooFInit(); // attach to namespace

	err = WooFCreate(Wname,sizeof(HW_EL),5); // create a WOOF
	if(err < 0) {
		fprintf(stderr,"couldn't create woof from %s\n",Wname);
		fflush(stderr);
		exit(1);
	}

	/*
	 * copy string into a structure to be stored as an element
	 * in the WOOF
	 */
	memset(el.string,0,sizeof(el.string));
	strncpy(el.string,"my first bark",sizeof(el.string));

	/*
	 * put the string in the WOOF and trigger a handler
	 */
	ndx = WooFPut(Wname,"hw",(void *)&el);

	if(WooFInvalid(err)) {
		fprintf(stderr,"first WooFPut failed for %s\n",Wname);
		fflush(stderr);
		exit(1);
	}

	printf("successfully appended %s to %s at seq_no %llu\n",
		"my first bark",
		Wname,
		ndx);

	return(0);
}

