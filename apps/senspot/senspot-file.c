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

// find the last valid file version in the woof wname
unsigned int LastFileVersion(char *wname)
{
	int err;
	SENSFILE sf;
	unsigned long seqno;
	seqno = WooFGetLatestSeqno(wname);
	if(WooFInvalid(seqno)) {
		return((unsigned int)-1);
	}
	while(seqno > 0) {
		err = WooFGet(wname,&sf,seqno);
		if(err < 0) {
			return((unsigned int)-1);
		}
		if(sf.flags & SENS_START) { // we found the latest start record
			return(sf.version);
		}
		seqno--;
	}
	// no version found
	return((unsigned int)-1);
}

unsigned long UseMover(char *wname)
{       
        unsigned long el_size;
        el_size = WooFMsgGetElSize(wname);
        if(el_size == (unsigned long)-1) {
                return((unsigned long)-1);
        }
        if(el_size == sizeof(SENSFILE)) {
                return(0);
        } else {
                return(el_size);
        }
}

	
