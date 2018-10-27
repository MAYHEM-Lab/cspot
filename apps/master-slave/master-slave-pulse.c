#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#include "woofc.h"
#include "senspot.h"

#define ARGS "W:H:LT:"
char *Usage = "master-slave-pulse -W woof_name\n"l

char Wname[4096];
char NameSpace[4096];
char Namelog_dir[4096];
char putbuf1[4096];
char putbuf2[4096];

#define MAX_RETRIES 20

int main(int argc, char **argv)
{
	int c;
	int i;
	int err;
	int uselocal;
	unsigned char input_buf[4096];
	char *str;
	SENSPOT spt;
	char wname[4096];
	char hname[4096];
	char *handler;
	char type;
	unsigned long seq_no;
	struct timeval tm;

	handler = NULL;
	memset(hname,0,sizeof(hname));
	memset(wname,0,sizeof(wname));
	uselocal = 0;

	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'W':
				strncpy(wname,optarg,sizeof(wname));
				break;
			case 'H':
				strncpy(hname,optarg,sizeof(hname));
				handler = hname;
				break;
			case 'L':
				uselocal = 1;
				break;
			case 'T':
				type = optarg[0];
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

	if(Namelog_dir[0] != 0) {
		sprintf(putbuf2,"WOOF_NAMELOG_DIR=%s",Namelog_dir);
		putenv(putbuf2);
	}

	if(uselocal == 1) {
		WooFInit();
	}

	memset(input_buf,0,sizeof(input_buf));
	str = fgets(input_buf,sizeof(input_buf),stdin);

	if(str == NULL) {
		exit(0);
	}

	SenspotAssign(&spt,type,input_buf);
	WooFLocalIP(spt.ip_addr,sizeof(spt.ip_addr));
	gettimeofday(&tm,NULL);
	spt.tv_sec = htonl(tm.tv_sec);
	spt.tv_usec = htonl(tm.tv_usec);

	seq_no = WooFPut(wname,handler,(void *)&spt);

	if(WooFInvalid(seq_no)) {
		fprintf(stderr,"senspot-put failed for %s with handler %s type %c and cargo %s\n",
			wname,
			handler,
			type,
			input_buf);
		fflush(stderr);
		exit(1);
	}


	exit(0);
}

	

	
	
