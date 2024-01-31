#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "woofc.h"
#include "senspot.h"

int PingHandler(WOOF *wf, unsigned long seq_no, void *element)
{
        int rval;
        unsigned long seqno;

        /*
         * subtract 1 from random value and put it to the same woof
         */
        memcpy(&rval,element,sizeof(int));
        rval--;
        seqno = WooFPut(WoofGetFileName(wf),NULL,(void *)&rval);
        if((long)seqno < 0) {
                printf("PingHandler: ERROR, bad put to %s\n",WoofGetFileName(wf));
                return(-1);
        }
        return(1);
}


