#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "mio.h"
#include "log.h"
#include "woofc.h"
#include "woofc-host.h"
#include "repair.h"

int main(int argc, char **argv)
{
	int i;
	int err;
	unsigned long history_size;
	unsigned long element_size;
	unsigned long seq_no;
	WOOF *wf;
	REPAIR_EL el;

	history_size = 10;
	element_size = sizeof(REPAIR_EL);

	WooFInit();

	err = WooFCreate("test", element_size, history_size);
	if (err < 0)
	{
		fprintf(stderr, "couldn't create woof %s\n", "test");
		fflush(stderr);
		return (0);
	}
	memset(&el, 0, sizeof(REPAIR_EL));
	sprintf(el.string, "original");

	for (i = 0; i < 5; i++)
	{
		seq_no = WooFPut("test", NULL, &el);
	}

	return (0);
}
