#include <stdio.h>
#include <stdlib.h>

#include "Helper.h"

#include "woofc.h"
#include "woofc-host.h"

int createWooF(char *woof_name, unsigned long element_size, unsigned long history_size){
    int err;
    WooFInit();
    err = WooFCreate(woof_name, element_size, history_size);
    if(err < 0){
        fprintf(stderr, "could not create WooF named %s\n", woof_name);
        fflush(stderr);
        exit(1);
    }
    return err;
}

unsigned long insertIntoWooF(char *woof_name, char *handler_name, void *element){

        unsigned long ndx;

	ndx = WooFPut(woof_name, handler_name, element);
	if(WooFInvalid(ndx)) {
		fprintf(stderr,"WooFPut failed for %s\n",woof_name);
		fflush(stderr);
		exit(1);
	}

        return ndx;

}
