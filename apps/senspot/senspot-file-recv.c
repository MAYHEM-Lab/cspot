#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#include "woofc.h"
#include "senspot.h"

#define ARGS "W:LVv:f:"
char *Usage = "senspot-file-recv -W woof_name for file storage\n\
\t-f file-to-write-out\n\
\t-L use same namespace for source and target\n\
\t-v file version number to get\n\
\t-V verbose\n";

char Wname[4096];
char Fname[4096];
char NameSpace[4096];
char Namelog_dir[4096];
char putbuf1[4096];
char putbuf2[4096];
int Verbose;

#define MAX_RETRIES 20

int main(int argc, char **argv)
{
	int c;
	int i;
	int err;
	int uselocal;
	SENSFILE sf;
	int version;
	unsigned long seqno;

	memset(Wname,0,sizeof(Wname));
	uselocal = 0;

	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'W':
				strncpy(Wname,optarg,sizeof(Wname));
				break;
			case 'f':
				strncpy(Fname,optarg,sizeof(Fname));
				break;
			case 'L':
				uselocal = 1;
				break;
			case 'v':
				version = atoi(optarg);
				break;
			case 'V':
				Verbose = 1;
				break;
			default:
				fprintf(stderr,
				"unrecognized command %c\n",(char)c);
				fprintf(stderr,"%s",Usage);
				exit(1);
		}
	}

	if(Wname[0] == 0) {
		fprintf(stderr,"must specify woof for file storage\n");
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		exit(1);
	}
	if(Fname[0] == 0) {
		fprintf(stderr,"must specify file name to write\n");
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


	if(version == 0) {
		version = LastFileVersion(Wname);
		if(version == -1) {
			fprintf(stderr,
				"could not find a file version for %s in %s\n",
				Fname,Wname);
			exit(1);
		}
	}

	// find start record for correct version
	
	seqno = WooFGetLatestSeqno(Wname);
	if(WooFInvalid(seqno)) {
		fprintf(stderr,
			"could not find last seqno in %s\n",
			Wname);
		exit(1);
	}

	err = WooFGet(Wname,&sf,seqno);

	if(err < 0) {
		fprintf(stderr,"senspot-file-recv failed for %s at %lu\n",
			wname,seqno);
		fflush(stderr);
		exit(1);
	}

	while(((sf.flags & SENS_START) == 0) ||
		(sf.version != version)) {
		seqno--;
		err = WooFGet(Wname,&sf,seqno);
		if(err < 0) {
			fprintf(stderr,
		"senspot-file-recv could not find start record for version %d in %s\n",
				version,Wname);
			exit(1);
		}
	}	
	end_seqno = sf.woof_end; // is woof seqno for end record
XXX




	exit(0);
}

	

	
	
