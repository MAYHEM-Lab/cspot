#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "woofc.h"
#include "put-test.h"


/*
 * put on the target and not on the WOOF with the args
 */
int null_handler(WOOF *wf, unsigned long seq_no, void *ptr)
{
	return(1);
}

