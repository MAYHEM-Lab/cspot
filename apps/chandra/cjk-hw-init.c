#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <string.h>

#include "woofc.h"
#include "hw.h"

#define ARGS "f:N:W:"
char *Usage = "cjk-hw -N namespace -W woof_name\n";

char Fname[4096];
char Wname[4096];
char NameSpace[4096];
char putbuf1[1024];

int starts_with(const char *str, const char *prefix) {
    // Compare the prefix part of the string with the given prefix
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

int main(int argc, char **argv)
{
	int c, err;

	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'f':
			case 'W':/* woof (file) name only (not namespace), uses local path
				  */
				strncpy(Fname,optarg,sizeof(Fname));
				break;
			case 'N': /* local namespace: -N /cspot-namespace1
				   */
				strncpy(NameSpace,optarg,sizeof(NameSpace));
				break;
			default:
				fprintf(stderr,
				"unrecognized command %c\n",(char)c);
				fprintf(stderr,"%s",Usage);
				exit(1);
		}
	}

	if(NameSpace[0] == 0) {
		fprintf(stderr,"must specify local namespace to create a woof\n");
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		exit(1);
	}
	if(Fname[0] == 0) {
		fprintf(stderr,"must specify filename for woof\n");
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		exit(1);
	}
        sprintf(Wname,"woof://%s/%s",NameSpace,Fname);
        sprintf(putbuf1,"WOOFC_DIR=%s",NameSpace);
        putenv(putbuf1);
	WooFInit(); // attach to namespace

	err = WooFCreate(Wname,sizeof(HW_EL),5); // create a WOOF
        if(err < 0) {
          	fprintf(stderr,"couldn't create woof from %s\n",Wname);
             	fflush(stderr);
              	exit(1);
        }
        fprintf(stderr,"WooFCreate success for %s\n",Wname);
        fflush(stderr);
	return(0);
}
