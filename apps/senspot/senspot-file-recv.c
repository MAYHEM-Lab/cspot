#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <fcntl.h>

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
	int err;
	int uselocal;
	SENSFILE sf;
	int fd;
	int version;
	unsigned long seqno;
	unsigned long start_seqno;
	unsigned long end_seqno;
	unsigned int next_dedup;
	unsigned int bytes;

	memset(Wname,0,sizeof(Wname));
	uselocal = 0;
	version = 0;

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
		fprintf(stderr,"ERROR: must specify woof for file storage\n");
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		exit(1);
	}
	if(Fname[0] == 0) {
		fprintf(stderr,"ERROR: must specify file name to write\n");
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
				"ERROR: could not find a file version for %s in %s\n",
				Fname,Wname);
			exit(1);
		}
	}

	// find start record for correct version
	
	seqno = WooFGetLatestSeqno(Wname);
	if(WooFInvalid(seqno)) {
		fprintf(stderr,
			"ERROR: could not find last seqno in %s\n",
			Wname);
		exit(1);
	}

	err = WooFGet(Wname,&sf,seqno);

	if(err < 0) {
		fprintf(stderr,"ERROR: senspot-file-recv failed for %s at %lu\n",
			Wname,seqno);
		fflush(stderr);
		exit(1);
	}

	if(Verbose == 1) {
		printf("scanning for version %d\n",version);
	}

	while(((sf.flags & SENS_START) == 0) ||
		(sf.version != version)) {
		seqno--;
		err = WooFGet(Wname,&sf,seqno);
		if(err < 0) {
			fprintf(stderr,
		"ERROR: senspot-file-recv could not find start record for version %d in %s\n",
				version,Wname);
			exit(1);
		}
	}	
	// save off start and end to do a log-wrap sanity check
	//
	// note that there is a race condition here in that the log might wrap after
	// we check but
	//
	start_seqno = seqno;
	end_seqno = sf.woof_end; // is woof seqno for end record
	if(Verbose == 1) {
		printf("woof: %s\n",Wname);
		printf("file: %s\n",Fname);
		printf("version: %d\n",version);
		printf("\tstart: %lu\n",start_seqno);
		printf("\tend: %lu\n",end_seqno);
	}
	if((end_seqno < 1) || (end_seqno > start_seqno)) {
		fprintf(stderr,"ERROR: band end seqno %lu with start %lu in %s\n",
				end_seqno,start_seqno,Wname);
		exit(1);
	}
				
	err = WooFGet(Wname,&sf,end_seqno);
	if(err < 0) {
		fprintf(stderr,
		"ERROR: could not fetch end record from %s, version %d at %lu\n",
			Wname,version,end_seqno);
		exit(1);
	}
	if(!(sf.flags & SENS_EOF)) {
		fprintf(stderr,
		"ERROR: bad end record at %lu in %s, version %d -- could be log wrap\n",
			end_seqno,Wname,version);
		exit(1);
	}

	// open the file for overwrite
	fd = open(Fname,O_CREAT | O_WRONLY, 0600);
	if(fd < 0) {
		fprintf(stderr,
			"ERROR: could not open %s for writing\n",
			Fname);
		exit(1);
	}

	// main read loop -- read from the end of the log back
	err = WooFGet(Wname,&sf,start_seqno);
	if(err < 0) {
		fprintf(stderr,
		"ERROR: could not reread start record in %s, version %d at %lu\n",
		Wname,version,start_seqno);
		close(fd);
		exit(1);
	}
	if(!(sf.flags & SENS_START) || (sf.version != version)) {
		fprintf(stderr,
		"ERROR: start record changed in %s version %d to %d at %lu\n",
		Wname,version, sf.version);
		close(fd);
		exit(1);
	}

	// here is the main loop
	seqno = start_seqno;
	next_dedup = 1;
	while(end_seqno <= seqno) {
		// if we are on the right seqno, write out
		if(next_dedup == sf.dedup_seqno) {
			bytes = write(fd,sf.payload,sf.payload_size);
			if(bytes != sf.payload_size) {
				fprintf(stderr,
				"ERROR: bad write at %lu in %s %d %d\n",
				seqno,Wname,sf.payload_size,bytes);
			}
			if(Verbose == 1) {
				printf("\twrote %d from %lu dedup: %d\n",
					bytes,seqno,next_dedup);
			}
		}
		// here, we could have duplicate end records so end_seqno
		// could be a duplicate -- we could just read to the
		// end, but better to exit prematurely
		if(sf.flags & SENS_EOF) {
			if(Verbose == 1) {
				printf("\tEOF found at %lu dedup: %d\n",
						seqno,next_dedup);
			}
			break;
		}
		next_dedup = next_dedup+1;
		seqno = seqno - 1;
		err = WooFGet(Wname,&sf,seqno);
		if(err < 0) {
			fprintf(stderr,
			"ERROR: could not get block at %lu in %s\n",
			seqno,Wname);
			close(fd);
			exit(1);
		}
	}
	close(fd);


	exit(0);
}

	

	
	
