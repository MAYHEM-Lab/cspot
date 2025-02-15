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
int UseNameSpace;
char putbuf1[1024];

int starts_with(const char *str, const char *prefix) {
    // Compare the prefix part of the string with the given prefix
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

int main(int argc, char **argv)
{
	int c, err;
	HW_EL el;
	unsigned long seq_no;
	short rand_short; //to hold the random short value that we'll convert to a string
	char rand_str[32]; //to hold the random string
	srand(time(NULL)); //seed the random number generator

	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'f':
			case 'W':/* woof (file) name
				  * can be just the woof name (woof-cjk-hw) 
				  * (requires -N option as well)
				  * or if -N is not used * it is the fully qualified path/woof 
				  * (uses the messaging layer and works across machines)
				  */
				strncpy(Fname,optarg,sizeof(Fname));
				break;
			case 'N': /* local namespace: -N /cspot-namespace1
				   */
				UseNameSpace = 1; //says use the fast path (not the messaging layer)
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
	if(UseNameSpace == 1) {
                sprintf(Wname,"woof://%s/%s",NameSpace,Fname);
                sprintf(putbuf1,"WOOFC_DIR=%s",NameSpace);
                putenv(putbuf1);
        } else {
                strncpy(Wname,Fname,sizeof(Wname));
        }
	if (UseNameSpace == 1) {
	    WooFInit(); // attach to namespace
	}

	/* check if the woof exists */
	seq_no = WooFGetLatestSeqno(Wname);
        fprintf(stderr,"%s latest seq_no %lu (%d)\n",Wname,seq_no,seq_no);
	fflush(stderr);

	/* perform a put given a random string */
	rand_short = (short)(rand());
	snprintf(rand_str, sizeof(rand_str), "%hd", rand_short);
	memset(el.string,0,sizeof(el.string));
        strncpy(el.string,rand_str,sizeof(el.string)-1); //rand_str (32 bytes), el.string (STRSIZE bytes)
        fprintf(stderr,"before put: %s\n",Wname);
        fflush(stderr);
	seq_no = WooFPut(Wname,"cjkHhw",(void *)&el);
	//seq_no = WooFPut(Wname,NULL,(void *)&el);
        fprintf(stderr,"after put: %s\n",Wname);
        fflush(stderr);
	if(WooFInvalid(seq_no)) {
                fprintf(stderr,"first WooFPut failed for %s\n",Wname);
                fflush(stderr);
                exit(1);
        }
        fprintf(stderr,"WooFPut success: %s at seq_no %lu (%d)\n",rand_str,seq_no,((int)seq_no)); 
        fflush(stderr);
	err = WooFGet(Wname,&el,seq_no);
	if(err < 0) {
               	fprintf(stderr,"couldn't perform WooFGet on %s\n",Wname);
               	fflush(stderr);
               	exit(1);
	}
       	fprintf(stderr,"Get %s at seq_no %lu is %s\n",Wname,seq_no,el.string); 
	return(0);
}
