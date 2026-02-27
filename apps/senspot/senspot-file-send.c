#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "woofc.h"
#include "senspot.h"

#define ARGS "W:f:LT:V"
char *Usage = "senspot-file-send -W woof_name for put\n\
\t-L use same namespace for source and target\n\
\t-f filename to transfer\n\
\t-V verbose\n";

char Wname[4096];
char NameSpace[4096];
char Namelog_dir[4096];
char Fname[4096];
char putbuf1[PAYLOAD];
char putbuf2[PAYLOAD];
int Verbose;

#define MAX_RETRIES 20


int main(int argc, char **argv)
{
	int c;
	int i;
	int err;
	int uselocal;
	SENSFILE sf;
	struct stat sbuf;
	char wname[4096];
	char fname[4096];
	int fd;
	int bytes_read;
	int blocks_to_write;
	int blocks;
	unsigned long seqno;
	int last;
	off_t pos;
	

	memset(wname,0,sizeof(wname));
	memset(fname,0,sizeof(fname));
	uselocal = 0;

	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'W':
				strncpy(wname,optarg,sizeof(wname));
				break;
			case 'f':
				strncpy(fname,optarg,sizeof(fname));
				break;
			case 'L':
				uselocal = 1;
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

	if(wname[0] == 0) {
		fprintf(stderr,"must specify woofname for target object\n");
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		exit(1);
	}

	if(fname[0] == 0) {
		fprintf(stderr,"must specify filename for xfer\n");
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		exit(1);
	}

	if(Namelog_dir[0] != 0) {
		sprintf(putbuf2,"WOOF_NAMELOG_DIR=%s",Namelog_dir);
		putenv(putbuf2);
	}

	fd = open(fname,O_RDONLY,0);
	if(fd < 0) {
		fprintf(stderr,"could not open %s\n",fname);
		exit(1);
	}

	// for PROTO_1 put start record in at tail, end record
	// at the head
	sf.proto = PROTO_1;
	sf.flags = SENS_EOF;
	sf.version = LastFileVersion(wname);
	if(sf.version == (unsigned int) -1) {
		sf.version = 1;
	} else {
		sf.version++;
	}

	err = fstat(fd,&sbuf);
	if(err < 0) {
		fprintf(stderr,"could not stat %s\n",fname);
		close(fd);
		exit(1);
	}

	blocks = sbuf.st_size / FPAYLOAD; // number of blocks
	last = sbuf.st_size % FPAYLOAD; // partial block at the end

	// do not write an empty file
	if((blocks == 0) && (last == 0)) {
		fprintf(stderr,"file is empty\n");
		exit(1);
	}

	if(Verbose == 1) {
		printf("file: %s\n",fname);
		printf("woof: %s\n",wname);
		printf("\tversion: %d\n",sf.version);
		printf("\tsize: %d\n",sbuf.st_size);
		printf("\tblocks: %d\n",blocks);
		printf("\tlast: %d\n",last);
	}


	if(uselocal == 1) {
		WooFInit();
	}


	// for PROTO_1, read the file backwards
	blocks_to_write = blocks;
	sf.dedup_seqno = blocks+1; // seqno counts from 1

	while(blocks_to_write >= 0) {
		pos = lseek(fd,(blocks_to_write * FPAYLOAD),SEEK_SET);
		if(pos == -1) {
			fprintf(stderr,
				"could not seek to position %d in %s\n",
				blocks,fname);
			close(fd);
			exit(1);
		}
		memset(sf.payload,0,sizeof(sf.payload));
		// this assumes that either the end of the last block or a
		// full block will be read
		bytes_read = read(fd,sf.payload,sizeof(sf.payload));
		if(bytes_read < 0) {
			fprintf(stderr,"could not read byte %d at %d in %s\n",
				bytes_read,pos,wname);
			perror("read");
			close(fd);
			exit(1);
		}
		if(bytes_read == 0) {
			fprintf(stderr,"read EOF at %d in %s\n",
				pos,wname);
			close(fd);
			exit(1);
		}
		if((bytes_read != FPAYLOAD) && (bytes_read != last)) {
			fprintf(stderr,
				"short read at %d, of %d, file %s\n",
					pos,bytes_read,fname);
			close(fd);
			exit(1);
		}
		sf.payload_size = bytes_read;
		if(Verbose == 1) {
			printf("\tputting block %d, size %d, dedup_seqno %d flags: %d ",
				blocks_to_write, bytes_read, sf.dedup_seqno,
					sf.flags);
		}
		seqno = WooFPut(wname,NULL,&sf); // put it
		if(WooFInvalid(seqno)) {
			fprintf(stderr,"could not put block %d of %s\n",
					blocks_to_write,wname);
			close(fd);
			exit(1);
		}
		if(Verbose == 1) {
			printf("woof seqno: %d\n", seqno);
			fflush(stdout);
		}
		// if EOF, remember seqno and clear the flag
		if(sf.flags & SENS_EOF) {
			if(Verbose == 1) {
				printf("\tEOF put at %d\n",seqno);
			}
			sf.woof_end = seqno;
			sf.flags = 0;
		}
		sf.dedup_seqno--;
		blocks_to_write--;
		// sanity checks
		if((sf.dedup_seqno == 1) &&
		   (blocks_to_write == 0)) { // next write will be start
			sf.flags = SENS_START;
		} else if((sf.dedup_seqno == 1) &&
			  (blocks_to_write > 0)) {
			fprintf(stderr,
			  "dedup_seqno: %d, blocks_left: %d in %s\n",
					sf.dedup_seqno,blocks_to_write,wname);
			close(fd);
			exit(1);
		} else if((sf.dedup_seqno > 1) &&
			  (blocks_to_write == 0)) {
			fprintf(stderr,
			  "dedup_seqno: %d, blocks_left: %d in %s\n",
					sf.dedup_seqno,blocks_to_write,wname);
			close(fd);
			exit(1);
		}
	}
		
	close(fd);
	exit(0);
}
