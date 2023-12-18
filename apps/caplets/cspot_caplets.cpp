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
    strncpy(Wname,"log",sizeof(Wname));

    WooFInit();
    err = WooFCreate(Wname, sizeof(EL), 1025); /*3*4 + 2 + some extra */
    if (err < 0)
    {
        fprintf(stderr, "WooFCreate for caplets_woof failed\n");
        fflush(stderr);
        exit(1);
    }

    char cap_token[1000];
    strncpy(cap_token,"6EC5AA523E19B916A4576718AF8AAEB78EF6856E775EE19CB92D1D3DC17A0569%FRAME%/home/centos/*+0+111%FRAME%/home/centos/cspot-caplets/*+0+111$FUNCTION:identity_constraint:855354fee30548e7c266e5a4a625fb3552a4d1f97050282fb5db8768902793bb%FRAME%request:/home/centos/cspot-caplets/log:my_handler:0:110",sizeof(cap_token));
    
    char id_token[1000];
    strncpy(id_token,"5671E3CDD07D330DF944E49C662D528AE242B9156D94620A3CA7A42F1CC9C688%FRAME%DummyBody%FRAME%855354fee30548e7c266e5a4a625fb3552a4d1f97050282fb5db8768902793bb%FRAME%6EC5AA523E19B916A4576718AF8AAEB78EF6856E775EE19CB92D1D3DC17A0569",sizeof(id_token));

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


        seq_no = WooFPutWithToken(cap_token, id_token, Wname, "temp",(void *)&el);

        if(WooFInvalid(seq_no)) {
            fprintf(stderr,"Failed to put\n");
            exit(1);
	    }

    }

}