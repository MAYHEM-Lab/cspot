#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>

#include "woofc.h"
#include "senspot.h"

extern unsigned long WooFMsgGetElSize(char *wname);
unsigned long UseMover(char *wname);

// find the last valid file version in the woof wname
unsigned int LastFileVersion(char *wname)
{
	int err;
	SENSFILE *sf;
	SENSMV *sm;
	unsigned long seqno;
	unsigned long el_size;
	unsigned char *el_buf;
	int use_mover;
	unsigned long version;

	seqno = WooFGetLatestSeqno(wname);
	if(WooFInvalid(seqno)) {
		return((unsigned int)-1);
	}

	el_size = UseMover(wname);

	if(el_size == (unsigned long) -1) {
		return(el_size);
	}

	if(el_size == 0) {
		use_mover = 0;
		el_size = sizeof(SENSFILE);
	}

	el_buf = malloc(el_size);
	if(el_buf == NULL) {
		fprintf(stderr,"no space for %lu bytes\n",
		el_size);
		exit(1);
	}
	
	while(seqno > 0) {
		err = WooFGet(wname,el_buf,seqno);
		if(err < 0) {
			free(el_buf);
			return((unsigned long)-1);
		}
		if(el_size == sizeof(SENSFILE)) {
			sf = (SENSFILE *)el_buf;
			if(sf->flags & SENS_START) { // we found the latest start record
				version = sf->version;
				free(el_buf);
				return((unsigned long)version);
			}
		} else {
			sm = (SENSMV *)el_buf;
			if(sm->flags & SENS_START) { // we found the latest start record
				version = sm->version;
				free(el_buf);
				return((unsigned long)version);
			}
		}
		seqno--;
	}
	// no version found
	free(el_buf);
	return((unsigned long)-1);
}

unsigned long UseMover(char *wname)
{       
        unsigned long el_size;
	unsigned long ret;

        el_size = WooFMsgGetElSize(wname);

        if(el_size == (unsigned long)-1) {
                return(el_size);
        }

        if(el_size == sizeof(SENSFILE)) {
		ret = 0;
        } else {
		ret = el_size;
        }
	return(ret);
}

	
