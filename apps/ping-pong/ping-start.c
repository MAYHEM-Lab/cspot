#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "ping-pong.h"

#define ARGS "W:w:s:"
char *Usage = "ping-start -w ping_woof_name\n\
\t-W pong_woof_name\n\
\t-s size\n";

char ping_Wname[4096];
char pong_Wname[4096];
char ping_NameSpace[4096];
char putbuf1[4098];
char putbuf2[4098];

int main(int argc, char **argv)
{
	int c;
	int err;
	PP_EL el;
	unsigned long seq_no;
	unsigned int pid;
	char local_ns[4096];
	char l_wname[4096];
	int size;

	size = 5;
	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'w':
				strncpy(ping_Wname,optarg,sizeof(ping_Wname));
				break;
			case 'W':
				strncpy(pong_Wname,optarg,sizeof(pong_Wname));
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

	if((ping_Wname[0] == 0) ||
	   (pong_Wname[0] == 0)) {
		fprintf(stderr,"must specify fully qualified woof name for ping and pong objects\n");
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		exit(1);
	}

	/*
	 * I am ping -- pong woof must have already been created
	 */
	memset(local_ns,0,sizeof(local_ns));
	err = WooFNameSpaceFromURI(ping_Wname,local_ns,sizeof(local_ns));
	if(err < 0) { // not a valid woof spec
		fprintf(stderr,"%s does not contain a valid namespace\n",ping_Wname);
		fprintf(stderr,"please specify full woof name for ping\n");
		fprintf(stderr,"%s",Usage);
		exit(1);
	} else { // name space okay
		sprintf(putbuf1,"WOOFC_DIR=%s",local_ns);
		sprintf(putbuf2,"WOOF_NAMELOG_DIR=%s",local_ns);
	}
	putenv(putbuf1);

	WooFInit();

	memset(l_wname,0,sizeof(l_wname));
	err = WooFNameFromURI(ping_Wname,l_wname,sizeof(l_wname));
	if(err < 0) {
		fprintf(stderr,"%s does not contain a valid object name for ping\n",
			ping_Wname);
		fprintf(stderr,"%s",Usage);
		exit(1);
	}

	/*
	 * create ping woof
	 */
	err = WooFCreate(l_wname,sizeof(PP_EL),size);

	if(err < 0) {
		fprintf(stderr,"couldn't create %s in %s from %s\n",
			l_wname,
			local_ns,
			ping_Wname);
		fflush(stderr);
		exit(1);
	}
	memset(&el,0,sizeof(el));
	strncpy(el.next_woof2,pong_Wname,sizeof(el.next_woof));
	strncpy(el.next_woof,ping_Wname,sizeof(el.next_woof2));

	el.counter = 0;
	el.max = size;
	seq_no = WooFPut(pong_Wname,"pong",(void *)&el);
	if(WooFInvalid(seq_no)) {
		fprintf(stderr,"couldn't put first put to %s\n",
				pong_Wname);
		exit(1);
	}

	exit(0);
	return(0);
}

