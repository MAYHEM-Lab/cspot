#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "ping-pong.h"

#define ARGS "c:f:s:N:n:H:"
char *Usage = "ping-start -f woof_name\n\
\t-H namelog-path\n\
\t-s size (in events)\n\
\t-N ping namespace-path\n\
\t-n pong namespace-path\n";

char Fname[4096];
char Wname[4096];
char Wname2[4096];
char NameSpace[4096];
char NameSpace2[4096];
char Namelog_dir[4096];
char putbuf1[4096];
char putbuf2[4096];

int main(int argc, char **argv)
{
	int c;
	int size;
	int err;
	PP_EL el;
	unsigned long seq_no;
	unsigned int pid;
	char local_ns[4096];
	char ip_addr_str[50];

	size = 5;

	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'f':
				strncpy(Fname,optarg,sizeof(Fname));
				break;
			case 's':
				size = atoi(optarg);
				break;
			case 'N':
				strncpy(NameSpace,optarg,sizeof(NameSpace));
				break;
			case 'n':
				strncpy(NameSpace2,optarg,sizeof(NameSpace));
				break;
			case 'H':
				strncpy(Namelog_dir,optarg,sizeof(Namelog_dir));
				break;
			default:
				fprintf(stderr,
				"unrecognized command %c\n",(char)c);
				fprintf(stderr,"%s",Usage);
				exit(1);
		}
	}

	if(Fname[0] == 0) {
		fprintf(stderr,"must specify filename for object\n");
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		exit(1);
	}

	if((NameSpace[0] == 0) && 
	   (NameSpace2[0] != 0)) {
		fprintf(stderr,"must specify two name spaces or none\n");
		fprintf(stderr,"%s",Usage);
		exit(1);
	}

	if((NameSpace[0] != 0) && 
	   (NameSpace2[0] == 0)) {
		fprintf(stderr,"must specify two name spaces or none\n");
		fprintf(stderr,"%s",Usage);
		exit(1);
	}

	if(Namelog_dir[0] != 0) {
		sprintf(putbuf2,"WOOF_NAMELOG_DIR=%s",Namelog_dir);
		putenv(putbuf2);
	}

	/*
	 * I am ping
	 */
	err = WooFIPAddrFromURI(NameSpace,ip_addr_str,sizeof(ip_addr_str));
	if(err < 0) { // no host spec
		err = WooFURINameSpace(NameSpace,local_ns,sizeof(local_ns));
		if(err > 0) {
			sprintf(putbuf1,"WOOFC_DIR=%s",local_ns);
		} else { // assume it is a local path
			sprintf(putbuf1,"WOOFC_DIR=%s",NameSpace);
		}
	} else { // there is a host spec
		err = WooFURINameSpace(NameSpace,local_ns,sizeof(local_ns));
		if(err < 0) {
			fprintf(stderr,"badly formed namespace %s\n",NameSpace);
			exit(1);
		}
		sprintf(putbuf1,"WOOFC_DIR=%s",local_ns);
	}
	putenv(putbuf1);
	/*
	 * ping's woof
	 */
	sprintf(Wname,"%s/%s",NameSpace,Fname);
	/*
	 * pong's woof
	 */
	sprintf(Wname2,"%s/%s",NameSpace2,Fname);


	WooFInit();

	/*
	 * create ping's woof (assume pong is up)
	 */
	err = WooFCreate(Wname,sizeof(PP_EL),size);

	if(err < 0) {
		fprintf(stderr,"couldn't create wf_1 from %s\n",Wname);
		fflush(stderr);
		exit(1);
	}

	memset(&el,0,sizeof(el));
	/*
	 * ping moves first and triggers pong
	 */
	strncpy(el.next_woof,Wname,sizeof(el.next_woof));
	strncpy(el.next_woof2,Wname2,sizeof(el.next_woof2));

	el.counter = 0;
	el.max = size;
	seq_no = WooFPut(Wname,"pong",(void *)&el);

	if(WooFInvalid(seq_no)) {
		fprintf(stderr,"first WooFPut failed\n");
		fflush(stderr);
		exit(1);
	}

	pthread_exit(NULL);
	return(0);
}

