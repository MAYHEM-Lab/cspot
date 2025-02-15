#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "woofc.h"
#include "hw.h"

#define ARGS "R:L:"
char *Usage = "cjk-ping -L woof1 -R woof2\n"; //can be the same woof
/* 
 * cjk-ping appends a string with delimiter to -L woof which triggers handler cjkHrw
 *	The string format is woof1name_cjkHrw_woof2name
 * cjkHrw when triggered reads the string appended, parses out woof2name
 * cjkHrw appends the current epoch in nanoseconds as a string to woof2name
 * the append triggers cjkHlw which extracts the time, diffs it with the current time
 * and prints the time difference in milliseconds
 * DELIMITER is defined in hw.h
 * USAGE: cjk-ping -L woof://172.17.0.2/cspot-namespace1/woof-cjk-lw -R woof://172.17.0.2/cspot-namespace1/woof-cjk-rw
 * This code has been tested locally (same host/namespace), across namespaces (same host), and across hosts.
 */
char Wname1[4096];
char Wname2[4096];
char putbuf1[1024];

int starts_with(const char *str, const char *prefix) {
    // Compare the prefix part of the string with the given prefix
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

int main(int argc, char **argv)
{
	int c;
	HW_EL el;
	unsigned long seq_no;
        fprintf(stderr,"[CJK] cjk-ping: starting...\n");
	fflush(stderr);

	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'L':/* fully qualified ping woof */
				memset(Wname1,0,sizeof(Wname1));
				strncpy(Wname1,optarg,sizeof(Wname1));
				break;
			case 'R':/* fully qualified pong woof */
				memset(Wname2,0,sizeof(Wname2));
				strncpy(Wname2,optarg,sizeof(Wname2));
				break;
			default:
				fprintf(stderr,
				"[CJK] cjk-ping: unrecognized command %c\n",(char)c);
				fprintf(stderr,"%s",Usage);
				exit(1);
		}
	}

	if(Wname1[0] == 0) {
		fprintf(stderr,"[CJK] cjk-ping: must specify filename for ping woof\n");
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		exit(1);
	}
	if(Wname2[0] == 0) {
		fprintf(stderr,"[CJK] cjk-ping: must specify filename for pong woof\n");
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		exit(1);
	}
        fprintf(stderr,"[CJK] cjk-ping: srcWoof=%s\n", Wname1);
        fprintf(stderr,"[CJK] cjk-ping: destWoof=%s\n", Wname2);
	fflush(stderr);

	/* check if the woof Wname1 exists */
	seq_no = WooFGetLatestSeqno(Wname1);
	if (((int)seq_no) == -1) {
            	fprintf(stderr,"[CJK] cjk-ping: Unable to access %s\nYou must run cjk-init on the host where the woof lives first.\n", Wname1);
	    	fflush(stderr);
		exit(1);
	}
	/* check if the woof Wname2 exists */
	seq_no = WooFGetLatestSeqno(Wname2);
	if (((int)seq_no) == -1) {
            	fprintf(stderr,"[CJK] cjk-ping: Unable to access %s\nYou must run cjk-init on the host where the woof lives first.\n", Wname2);
	    	fflush(stderr);
		exit(1);
	}

	/* perform a put with string value: srcWoof_cjkHrw_destWoof 
	 * Wname1 is the srcWoof (we are pinging) 
	 * Wname2 is the destWoof (that will be ponged by the other side) 
	 */
	memset(el.string,0,sizeof(el.string));
        strncpy(el.string,Wname1,sizeof(el.string)-1); //el.string (STRSIZE bytes)
        strcat(el.string,DELIMITER);
        strcat(el.string,Wname2);
        fprintf(stderr,"[CJK] cjk-ping: pinging %s via cjkHrw\n",Wname2);
        fflush(stderr);
	seq_no = WooFPut(Wname1,"cjkHrw",(void *)&el);
	if(WooFInvalid(seq_no)) {
                fprintf(stderr,"[CJK] cjk-ping: ping WooFPut failed for %s\n",Wname1);
                fflush(stderr);
                exit(1);
        }
	return(0);
}
