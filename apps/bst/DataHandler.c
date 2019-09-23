#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "woofc.h"
#include "Data.h"
#include "DataItem.h"

int DataHandler(WOOF *wf, unsigned long seq_no, void *ptr)
{

	DATA *el = (DATA *)ptr;
	fprintf(stdout,"DATA HANDLER -- WooF Name {%s} Seq No {%lu} Data {%s}\n",
			wf->shared->filename, seq_no, DI_get_str(el->di));
	fflush(stdout);
	return(1);

}

