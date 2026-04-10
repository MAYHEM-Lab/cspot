#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <fcntl.h>
#include <time.h>

#include "woofc.h"
#include "senspot.h"

extern int WooFMsgGet(const char* woof_name, void* element, unsigned long el_size, unsigned long seq_no);
extern unsigned long WooFMsgGetElSize(char *wname);

#define ARGS "W:LVv:f:m:ls:"
char *Usage = "senspot-file-recv -W woof_name for file storage\n\
\t-f file-to-write-out\n\
\t-l list latest version\n\
\t-L list all versions\n\
\t-v file version number to get\n\
\t-m minor-version number (optional)\n\
\t-s starting seqno for log scan (optional)\n\
\t-V verbose\n";

char Wname[4096];
char Fname[4096];
char NameSpace[4096];
char Namelog_dir[4096];
char putbuf1[4096];
char putbuf2[4096];
int Verbose;

#define MAX_RETRIES 20

SENSFILE *sf;
SENSMV *sm;
unsigned long El_size;

void PrintVersions(char *wname, int mode)
{
	unsigned long seqno;
	int err;
        struct tm tm_buf;
	time_t epoch;
        char buffer[64];
	unsigned char *el_buf;
	

	seqno = WooFGetLatestSeqno(wname);
	if(WooFInvalid(seqno)) {
		fprintf(stderr,"could not get latest seqno for %s\n",
			wname);
		exit(1);
	}

	el_buf = malloc(El_size);
	if(el_buf == NULL) {
		fprintf(stderr,"no space to pint versions: %lu\n",El_size);
		exit(1);
	}
	memset(el_buf,0,El_size);
	//err = WooFGet(wname,el_buf,seqno);
	err = WooFMsgGet(wname,el_buf,El_size,seqno);
	if(err < 0) {
		fprintf(stderr,"could not fetch tail from %s at %lu, created: %s\n",
			wname,seqno);
		exit(1);
	}

	sm = (SENSMV *)el_buf;
	while(1) {
		if(sm->flags & SENS_START) {
			epoch = (time_t)sm->creation_time;
			localtime_r((const time_t *)&epoch, &tm_buf);
			strftime(buffer, sizeof(buffer),
				"%Y-%m-%d %H:%M:%S",
				&tm_buf);
	printf("version %d:%d at %lu, created: %s (%lu) size: %lu start_seqno: %lu\n",
				sm->version,
				sm->woof_end,
				seqno,
				buffer,
				epoch,
				sm->file_size,
				seqno);
			fflush(stdout);
			if(mode == 1) {
				free(el_buf);
				return;
			}
		}
		seqno--;
		if(seqno == 0) {
			break;
		}
		memset(el_buf,0,El_size);
		//err = WooFGet(wname,el_buf,seqno);
		err = WooFMsgGet(wname,el_buf,El_size,seqno);
		if(err < 0) {
			break;
		}
	}

	free(el_buf);
	return;
}
		

int main(int argc, char **argv)
{
	int c;
	int err;
	int uselocal;
	int fd;
	unsigned int version;
	unsigned int minor;
	unsigned long seqno;
	unsigned long start_seqno;
	unsigned long end_seqno;
	unsigned int next_dedup;
	unsigned int bytes;
        struct tm tm_buf;
	time_t epoch;
        char buffer[64];
	struct timeval start_tv;
	struct timeval end_tv;
	double total;
	double duration;
	int found;
	int latest;
	unsigned char *el_buf;
	unsigned char *pbuf;
	unsigned int psize;
	unsigned long pdedup;
	unsigned long start_block;

	memset(Wname,0,sizeof(Wname));
	uselocal = 0;
	version = 0;
	minor = 0;
	latest = 0;

	start_block = 0;

	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'W':
				strncpy(Wname,optarg,sizeof(Wname));
				break;
			case 'f':
				strncpy(Fname,optarg,sizeof(Fname));
				break;
			case 'L':
				latest = 2;
				break;
			case 'l':
				latest = 1;
				break;
			case 'm':
				minor = atoi(optarg);
				break;
			case 's':
				start_block = atol(optarg);
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
	El_size = WooFMsgGetElSize(Wname);;
	if(El_size  == (unsigned long)-1) {
		fprintf(stderr,"ERROR: could not get El_size for %s\n",Wname);
		exit(1);
	}

	if((latest > 0) && (latest <= 2)) {
		PrintVersions(Wname,latest);
		exit(0);
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

	if(version == 0) {
		version = LastFileVersion(Wname);
		if(version == -1) {
			fprintf(stderr,
				"ERROR: could not find a file version for %s in %s\n",
				Fname,Wname);
			exit(1);
		}
	}


	el_buf = malloc(El_size);
	if(el_buf == NULL) {
		fprintf(stderr,"could not get size for %lu bytes\n",
		El_size);
		exit(1);
	}

	sm = (SENSMV *)el_buf;

	// find start record for correct version
	if(start_block == 0) {
		seqno = WooFGetLatestSeqno(Wname);
		if(WooFInvalid(seqno)) {
			fprintf(stderr,
				"ERROR: could not find last seqno in %s\n",
				Wname);
			exit(1);
		}
	} else {
		// start scan at this seqno
		// does not wrap around the woof so can fail even
		// even if version is present but at a higher
		// seqno
		seqno = start_block;
	}

	memset(el_buf,0,El_size);
	//err = WooFGet(Wname,el_buf,seqno);
	err = WooFMsgGet(Wname,el_buf,El_size,seqno);

	if(err < 0) {
		fprintf(stderr,"ERROR: senspot-file-recv failed for %s at %lu\n",
			Wname,seqno);
		fflush(stderr);
		exit(1);
	}


	if(Verbose == 1) {
		printf("scanning for version %d:%d\n",version,minor);
	}

	found = 0;
	while(found == 0) {
		if((sm->flags & SENS_START) != 0) {
			if(minor == 0) {
				if(sm->version == version) {
					found = 1;
					break;
				}
			} else if((sm->version == version) &&
				  (minor == sm->woof_end)) {
					found = 1;
					break;
			}
		}
		seqno--;
		//err = WooFGet(Wname,el_buf,seqno);
		err = WooFMsgGet(Wname,el_buf,El_size,seqno);
		if(err < 0) {
			fprintf(stderr,
		"ERROR: senspot-file-recv could not find start record for version %d:%d in %s\n",
				version,minor,Wname);
			exit(1);
		}
	}	
	// send can't fill in the end seqno if it is in the same element as
	// the start seqno so need to test for this case
	if((sm->flags & SENS_START) && (sm->flags & SENS_EOF)) {
		sm->woof_end = seqno;
		minor = seqno;
	}
	// save off start and end to do a log-wrap sanity check
	//
	// note that there is a race condition here in that the log might wrap after
	// we check but
	//
	start_seqno = seqno;
	minor = end_seqno = sm->woof_end; // is woof seqno for end record
	if(Verbose == 1) {
		epoch = (time_t)sm->creation_time;
		localtime_r((const time_t *)&epoch, &tm_buf);
		strftime(buffer, sizeof(buffer),
             		"%Y-%m-%d %H:%M:%S",
             		&tm_buf);
		printf("woof: %s\n",Wname);
		printf("file: %s\n",Fname);
		printf("mover\n");
		printf("\tversion: %d:%d\n",version,minor);
		printf("\tcreation_time: %s (%lu)\n",buffer,sm->creation_time);
		printf("\tstart: %lu\n",start_seqno);
		printf("\tend: %lu\n",end_seqno);
	}
	if((end_seqno < 1) || (end_seqno > start_seqno)) {
		fprintf(stderr,"ERROR: band end seqno %lu with start %lu in %s\n",
				end_seqno,start_seqno,Wname);
		exit(1);
	}
				
	memset(el_buf,0,El_size);
	//err = WooFGet(Wname,el_buf,end_seqno);
	err = WooFMsgGet(Wname,el_buf,El_size,end_seqno);
	if(err < 0) {
		fprintf(stderr,
		"ERROR: could not fetch end record from %s, version %d at %lu\n",
			Wname,version,end_seqno);
		exit(1);
	}
	if(!(sm->flags & SENS_EOF)) {
		fprintf(stderr,
		"ERROR: bad end record at %lu in %s, version %d -- could be log wrap\n",
			end_seqno,Wname,version);
		exit(1);
	}

	// open the file for overwrite
	fd = open(Fname,O_CREAT | O_WRONLY | O_TRUNC, 0600);
	if(fd < 0) {
		fprintf(stderr,
			"ERROR: could not open %s for writing\n",
			Fname);
		exit(1);
	}

	// main read loop -- read from the end of the log back
	if(Verbose == 1) {
		gettimeofday(&start_tv,NULL);
		total = 0;
	}
	memset(el_buf,0,El_size);
	//err = WooFGet(Wname,el_buf,start_seqno);
	err = WooFMsgGet(Wname,el_buf,El_size,start_seqno);
	if(err < 0) {
		fprintf(stderr,
		"ERROR: could not reread start record in %s, version %d at %lu\n",
		Wname,version,start_seqno);
		close(fd);
		exit(1);
	}
	if((sm->flags & SENS_START) && (sm->flags & SENS_EOF)) {
		sm->woof_end = minor;
	}
	if(!(sm->flags & SENS_START) || 
	   (sm->version != version) || 
	   (sm->woof_end != minor)) {
		fprintf(stderr,
		"ERROR: start record changed in %s version %d:%d to %d at %lu\n",
		Wname,version, sm->version,minor);
		close(fd);
		exit(1);
	}

	// here is the main loop
	seqno = start_seqno;
	next_dedup = 1;
	pbuf = ((unsigned char *)sm) + sizeof(SENSMV);
	while(end_seqno <= seqno) {
		pdedup = sm->dedup_seqno;

		// skip if the wong version

		if((sm->version != version) ||
		   ((sm->woof_end != end_seqno) && (seqno != end_seqno))) {
			seqno--;
			err = WooFMsgGet(Wname,el_buf,El_size,seqno);
			if(err < 0) {
				fprintf(stderr,
				"ERROR: could not get other mover block at %lu in %s\n",
				seqno,Wname);
				close(fd);
				exit(1);
			}
			continue;
		}
				
		// if we are on the right seqno, write out
		if(next_dedup == pdedup) {
			psize = sm->payload_size;
			bytes = write(fd,pbuf,sm->payload_size);
			if(bytes != psize) {
				fprintf(stderr,
				"ERROR: bad write at %lu in %s %d %d\n",
				seqno,Wname,psize,bytes);
				exit(1);
			}
			if(Verbose == 1) {
				total += bytes;
				printf("\twrote %d from %lu dedup: %d\n",
					bytes,seqno,next_dedup);
			}
		}
		// here, we could have duplicate end records so end_seqno
		// could be a duplicate -- we could just read to the
		// end, but better to exit prematurely
		if(sm->flags & SENS_EOF) {
			if(Verbose == 1) {
				printf("\tEOF found at %lu dedup: %d\n",
						seqno,next_dedup);
			}
			break;
		}
		next_dedup = next_dedup+1;
		seqno = seqno - 1;
		//err = WooFGet(Wname,el_buf,seqno);
		err = WooFMsgGet(Wname,el_buf,El_size,seqno);
		if(err < 0) {
			fprintf(stderr,
			"ERROR: could not get block at %lu in %s\n",
			seqno,Wname);
			close(fd);
			exit(1);
		}
	}
	if(Verbose == 1) {
		gettimeofday(&end_tv,NULL);
		duration = (((double)end_tv.tv_sec + (double)end_tv.tv_usec / 1000000) -
			    ((double)start_tv.tv_sec + (double)start_tv.tv_usec / 1000000));
		printf("\t%f megabytes/sec read (%f bytes in %f sec)\n",
		(double)(total / (1024*1024))/duration,
		total,
		duration);
	}
	close(fd);

	free(el_buf);

	exit(0);
}

	

	
	
