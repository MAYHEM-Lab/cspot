#include "cspot_caplets.h"
#include "woofc.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    int err;
    char Wname[300];
    strncpy(Wname,"sensor_log",sizeof(Wname));

    WooFInit();
    err = WooFCreate(Wname, sizeof(EL), 1025); /*3*4 + 2 + some extra */
    if (err < 0)
    {
        fprintf(stderr, "WooFCreate for caplets_woof failed\n");
        fflush(stderr);
        exit(1);
    }

    char val_token[2048];
    strncpy(val_token,"4EB744B4B49783B51C308D1998765EC4B7FD267209F56859412BDEC3EF499D70%FRAME%/home/centos/*+0+111%FRAME%/home/centos/cspot-caplets/*+0+111",sizeof(val_token));
    
    EL el;
    memset(el.string,0,sizeof(el.string));
	strncpy(el.string,"my first bark",sizeof(el.string));
    
    unsigned long seq_no;
    for(int i=0; i<3; i++){
        seq_no =WooFPut( Wname,NULL,(void *)&el);
        if(WooFInvalid(seq_no)) {
            fprintf(stderr,"Failed to put\n");
            exit(1);
        
        }
        
        
        fprintf(stdout, "PUTTING NOW\n");
        fflush(stdout);


        seq_no = WooFPutWithToken(val_token, Wname,NULL,(void *)&el);

        if(WooFInvalid(seq_no)) {
            fprintf(stderr,"Failed to put\n");
            exit(1);
	    }

    }

}