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

#define ARGS "W:f:VM"
char *Usage = "senspot-file-send -W woof_name for put\n\
\t-f filename to transfer\n\
\t-M <use file mover protocol>\n\
\t-V verbose\n";

char NameSpace[4096];
char Namelog_dir[4096];
char putbuf1[PAYLOAD];
char putbuf2[PAYLOAD];
int Verbose;

#define MAX_RETRIES 20

SENSFILE sf;

int SendFileNoMover(char *wname, int fd)
{
	int err;
	struct stat sbuf;
	int bytes_read;
	int blocks_to_write;
	int blocks;
	unsigned long seqno;
	int last;
	off_t pos;
	struct timeval tv;
	struct tm tm_buf;
	char buffer[64];
	double duration;
	double total;
	struct timeval start_tv;
	struct timeval end_tv;


	// for PROTO_1 (no mover) put start record in at tail, end record
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
		fprintf(stderr,"could not stat file\n");
		close(fd);
		exit(1);
	}

	gettimeofday(&tv,NULL);
	sf.creation_time = tv.tv_sec;
	if(Verbose == 1) {
		localtime_r(&sf.creation_time, &tm_buf);
		strftime(buffer, sizeof(buffer),
			"%Y-%m-%d %H:%M:%S",
			&tm_buf);
	}
	blocks = sbuf.st_size / FPAYLOAD; // number of blocks
	last = sbuf.st_size % FPAYLOAD; // partial block at the end

	// do not write an empty file
	if((blocks == 0) && (last == 0)) {
		fprintf(stderr,"file is empty\n");
		exit(1);
	}

	if(Verbose == 1) {
		printf("woof: %s\n",wname);
		printf("\tno_mover\n");
		printf("\tversion: %d\n",sf.version);
		printf("\tcreation_time: %s (%lu)\n",buffer,sf.creation_time);
		printf("\tsize: %d\n",sbuf.st_size);
		printf("\tblocks: %d\n",blocks);
		printf("\tlast: %d\n",last);
		gettimeofday(&start_tv,NULL);
		total = 0;
	}


	// for PROTO_1, read the file backwards
	blocks_to_write = blocks;
	sf.dedup_seqno = blocks+1; // seqno counts from 1

	while(blocks_to_write >= 0) {
		// if this just fits the last block, don't read EOF
		if(last == 0) {
			blocks_to_write--;
			continue;
		}
		pos = lseek(fd,(blocks_to_write * FPAYLOAD),SEEK_SET);
		if(pos == -1) {
			fprintf(stderr,
				"could not seek to position %d\n",
				blocks);
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
				"short read at %d, of %d\n",
					pos,bytes_read);
			close(fd);
			exit(1);
		}
		sf.payload_size = bytes_read;
		if(Verbose == 1) {
			printf("\tputting block %d, size %d, dedup_seqno %d flags: %d ",
				blocks_to_write, bytes_read, sf.dedup_seqno,
					sf.flags);
			total += bytes_read;
		}
		gettimeofday(&tv,NULL);
		sf.tv_sec = tv.tv_sec;
		sf.tv_usec = tv.tv_usec;
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
			gettimeofday(&end_tv,NULL);
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

	if(Verbose == 1) {
		duration = (((double)end_tv.tv_sec + 
			(double)end_tv.tv_usec/1000000) -
			   (((double)start_tv.tv_sec + 
                        (double)start_tv.tv_usec/1000000)));
		printf("\t%f megabytes / second wrote\n",
			(total/(1024*1024))/duration);
	}
		
	close(fd);
	return(1);
}

int SendFileMover(char *wname, int fd, unsigned long el_size)
{
	int err;
	struct stat sbuf;
	int bytes_read;
	int blocks_to_write;
	int blocks;
	unsigned long seqno;
	int last;
	off_t pos;
	struct timeval tv;
	struct tm tm_buf;
	char buffer[64];
	double duration;
	double total;
	struct timeval start_tv;
	struct timeval end_tv;
	SENSMV *sm;
	unsigned char *el_buf;
	unsigned long fpayload;
	unsigned char *pbuf;

	el_buf = (unsigned char *)malloc(el_size);
	if(el_buf == NULL) {
		fprintf(stderr,
		"no space for mover buf of %u to send to %s\n",
		el_size,
		wname);
		exit(1);
	}
	sm = (SENSMV *)el_buf;
	fpayload = el_size - sizeof(SENSMV);
	if(fpayload > el_size) { // did we wrap?
		fprintf(stderr,"could not set payload size to %lu for %s\n",
		fpayload,
		wname);
		exit(1);
	}

	// for PROTO_2 (mover) put start record in at tail, end record
	// at the head
	sm->proto = PROTO_2;
	sm->flags = SENS_EOF;
	sm->version = LastFileVersion(wname);
	if(sm->version == (unsigned int) -1) {
		sm->version = 1;
	} else {
		sm->version++;
	}

	err = fstat(fd,&sbuf);
	if(err < 0) {
		fprintf(stderr,"could not stat file\n");
		close(fd);
		exit(1);
	}

	gettimeofday(&tv,NULL);
	sm->creation_time = tv.tv_sec;
	if(Verbose == 1) {
		localtime_r(&sm->creation_time, &tm_buf);
		strftime(buffer, sizeof(buffer),
			"%Y-%m-%d %H:%M:%S",
			&tm_buf);
	}
	blocks = sbuf.st_size / fpayload; // number of blocks
	last = sbuf.st_size % fpayload; // partial block at the end
	sm->file_size = sbuf.st_size;

	// do not write an empty file
	if((blocks == 0) && (last == 0)) {
		fprintf(stderr,"file is empty\n");
		exit(1);
	}

	if(Verbose == 1) {
		printf("woof: %s\n",wname);
		printf("\tmover\n");
		printf("\tversion: %d\n",sm->version);
		printf("\tcreation_time: %s (%lu)\n",buffer,sm->creation_time);
		printf("\tsize: %d\n",sbuf.st_size);
		printf("\tblocks: %d\n",blocks);
		printf("\tlast: %d\n",last);
		printf("\tel_size: %d\n",el_size);
		gettimeofday(&start_tv,NULL);
		total = 0;
	}


	// for PROTO_1, read the file backwards
	blocks_to_write = blocks;
	sm->dedup_seqno = blocks+1; // seqno counts from 1

	pbuf = ((unsigned char *)(sm)) + sizeof(SENSMV);
	while(blocks_to_write >= 0) {
		// if this just fits the last block, don't read EOF
		if(last == 0) {
			blocks_to_write--;
			continue;
		}
		pos = lseek(fd,(blocks_to_write * fpayload),SEEK_SET);
		if(pos == -1) {
			fprintf(stderr,
				"could not seek to position %d\n",
				blocks);
			close(fd);
			exit(1);
		}
		memset(pbuf,0,fpayload);
		// this assumes that either the end of the last block or a
		// full block will be read
		bytes_read = read(fd,pbuf,fpayload);
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
		if((bytes_read != fpayload) && (bytes_read != last)) {
			fprintf(stderr,
				"short read at %d, of %d\n",
					pos,bytes_read);
			close(fd);
			exit(1);
		}
		sm->payload_size = bytes_read;
		if(Verbose == 1) {
			printf("\tputting block %d, size %d, dedup_seqno %d flags: %d ",
				blocks_to_write, bytes_read, sm->dedup_seqno,
					sm->flags);
			total += bytes_read;
		}
		gettimeofday(&tv,NULL);
		sm->tv_sec = tv.tv_sec;
		sm->tv_usec = tv.tv_usec;
		seqno = WooFPut(wname,NULL,sm); // put it
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
		if(sm->flags & SENS_EOF) {
			if(Verbose == 1) {
				printf("\tEOF put at %d\n",seqno);
			}
			sm->woof_end = seqno;
			sm->flags = 0;
			gettimeofday(&end_tv,NULL);
		}
		sm->dedup_seqno--;
		blocks_to_write--;
		// sanity checks
		if((sm->dedup_seqno == 1) &&
		   (blocks_to_write == 0)) { // next write will be start
			sm->flags = SENS_START;
		} else if((sm->dedup_seqno == 1) &&
			  (blocks_to_write > 0)) {
			fprintf(stderr,
			  "dedup_seqno: %d, blocks_left: %d in %s\n",
					sm->dedup_seqno,blocks_to_write,wname);
			close(fd);
			exit(1);
		} else if((sm->dedup_seqno > 1) &&
			  (blocks_to_write == 0)) {
			fprintf(stderr,
			  "dedup_seqno: %d, blocks_left: %d in %s\n",
					sm->dedup_seqno,blocks_to_write,wname);
			close(fd);
			exit(1);
		}
	}

	if(Verbose == 1) {
		duration = (((double)end_tv.tv_sec + 
			(double)end_tv.tv_usec/1000000) -
			   (((double)start_tv.tv_sec + 
                        (double)start_tv.tv_usec/1000000)));
		printf("\t%f megabytes / second wrote\n",
			(total/(1024*1024))/duration);
	}
		
	close(fd);
	return(1);
}

int main(int argc, char **argv)
{
	int c;
	int i;
	int err;
	struct stat sbuf;
	char wname[4096];
	char fname[4096];
	int fd;
	unsigned long use_mover;

	

	memset(wname,0,sizeof(wname));
	memset(fname,0,sizeof(fname));
	use_mover = 0;

	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'W':
				strncpy(wname,optarg,sizeof(wname));
				break;
			case 'f':
				strncpy(fname,optarg,sizeof(fname));
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

	if(use_mover == 0) {
		use_mover = UseMover(wname);
		if(use_mover == (unsigned long)-1) {
			fprintf(stderr,
			"could not determine woof el size for %s\n",
			wname);
			exit(1);
		}
	}

	if(Verbose == 1) {
		printf("file: %s\n",fname);
	}
	if(use_mover == 0) {
		err = SendFileNoMover(wname,fd);
		if(err > 0) {
			exit(0);
		}
	} else {
		err = SendFileMover(wname,fd,use_mover);
		if(err > 0) {
			exit(0);
		}
	}

}
