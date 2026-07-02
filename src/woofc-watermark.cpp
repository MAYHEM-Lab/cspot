#include <cstdlib>
#include <unistd.h>
#include <string.h>
extern "C" {
#include "log.h"
#include "woofc.h"
}

#include "debug.h"
#include "global.h"
#include "net.h"
#include "woofc-access.h"
#include "woofc-priv.h"

#include <stdint.h>


#include <semaphore.h>
typedef sem_t sema_3_0;

typedef struct {
    int32_t value;
} sema_3_1;

struct woof_shared_stc_3_0{
    char filename[2032]; // added watermark and version -- was 2048
    uint64_t watermark; // this must be here in struct for bw compat
    double version; // this must be here in struct for bw compat
    sema_3_0 mutex;
    sema_3_0 tail_wait;
    unsigned long long seq_no;
    unsigned long history_size;
    unsigned long head;
    unsigned long tail;
    unsigned long element_size;
#ifdef REPAIR
    int repair_mode;
#endif
#ifdef TRACK
    int hid;
#endif
};

struct woof_shared_stc_3_1{
    char filename[2032]; // added watermark and version -- was 2048
    uint64_t watermark; // this must be here in struct for bw compat
    double version; // this must be here in struct for bw compat
    sema_3_1 mutex;
    sema_3_1 tail_wait;
    unsigned long long seq_no;
    unsigned long history_size;
    unsigned long head;
    unsigned long tail;
    unsigned long element_size;
#ifdef REPAIR
    int repair_mode;
#endif
#ifdef TRACK
    int hid;
#endif
};
typedef struct woof_shared_stc_3_0 WOOF_SHARED_3_0;
typedef struct woof_shared_stc_3_1 WOOF_SHARED_3_1;

#define CSPOT_WATERMARK (0xdeadbeef)

extern void WooFWatermark(char *name);

#define ARGS "W:U"
char *Usage = "woofc-watermark -W local-woof-name\n\
\t-U <perform upgrade>\n\
\t-F <force upgrade of current or higher version>\n";

char Fname[2032];
char NFname[2032];
int Force;

int main(int argc, char**argv)
{
	int c;
	int do_upgrade;
	double r;
	char s_buf[1024];
	WOOF *old_wf;
	WOOF *new_wf;
	WOOF_SHARED_3_0 *old_wfs;
	WOOF_SHARED_3_1 *new_wfs;
	unsigned char *old_ptr;
	unsigned char *new_ptr;
	int err;

	do_upgrade = 0;
	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'W':
				strncpy(Fname,optarg,sizeof(Fname));
				break;
			case 'U':
				do_upgrade = 1;
				break;
			case 'F':
				Force = 1;
				break;
			default:
				fprintf(stderr,"unrecognized command %c\n",(char)c);
				fprintf(stderr,"%s",Usage);
				exit(1);
		}
	}
	if(Fname[0] == 0) {
		fprintf(stderr,"must specify local woof name command\n");
		fprintf(stderr,"%s",Usage);
		exit(1);
	}

	WooFInit();
	if(do_upgrade == 0) {
		printf("watermarking %s with version %f\n",Fname,CSPOT_VERSION);
		fflush(stdout);
		WooFWatermark(Fname);
		exit(0);
	}


	r = drand48();
	// make a cp string
	sprintf(NFname,"./%s.%f",Fname,r);
	old_wf = WooFOpen(Fname);
	if(old_wf == NULL) {
		fprintf(stderr,"could not open %s as copy",Fname);
		exit(1);
	}

	old_wfs = (WOOF_SHARED_3_0 *)old_wf->shared;

	if((old_wfs->version >= CSPOT_VERSION)
	   && (Force == 0)) {
		fprintf(stderr,
		"%s is at version %f which <= current version %f\n",
		Fname,
		old_wfs->version,
		CSPOT_VERSION);
		fprintf(stderr,
		"-U is not idempotent and will destroy an already upgraded woof\n");
		fprintf(stderr,
			"if you are sure, rerun with the -F flag to force\n");
		exit(1);
	}

	printf("upgrading %s (version %f) to latest format at version %f\n",
		Fname,
		old_wfs->version,
		CSPOT_VERSION);
	fflush(stdout);

	err = WooFCreate(NFname,old_wfs->element_size,old_wfs->history_size);
	if(err < 0) {
		fprintf(stderr,"could not create new woof %s\n",NFname);
		WooFDrop(old_wf);
		exit(1);
	}

	new_wf = WooFOpen(NFname);
	if(new_wf == NULL) {
		fprintf(stderr,"could not open new woof %s\n",NFname);
		WooFDrop(old_wf);
		exit(1);
	}

	new_wfs = (WOOF_SHARED_3_1 *)new_wf->shared;

	// move the seqno
	new_wfs->seq_no = old_wfs->seq_no;

	// move the head and tail indices
	new_wfs->head = old_wfs->head;
	new_wfs->tail = old_wfs->tail;

	// move the file name
	strncpy(new_wfs->filename,old_wfs->filename,sizeof(new_wfs->filename));

	// copy the elements
	old_ptr = (unsigned char *)((((char*)old_wfs) + sizeof(WOOF_SHARED_3_0)));
	new_ptr = (unsigned char *)((((char*)new_wfs) + sizeof(WOOF_SHARED_3_1)));
	memcpy(new_ptr,old_ptr,old_wfs->history_size*(old_wfs->element_size + sizeof(ELID)));
	WooFDrop(old_wf);
	WooFDrop(new_wf);

	sprintf(s_buf,"cp ./%s ./%s",NFname,Fname);
	system(s_buf);
	sprintf(s_buf,"rm ./%s",NFname);
	system(s_buf);


	exit(0);
}
	
