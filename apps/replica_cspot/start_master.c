#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "woofc-host.h"
#include "woofc-access.h"
#include "replica.h"

int main(int argc, char **argv)
{
	int err;
	
	WooFInit();
	err = WooFCreate("slave_progress_for_replica", sizeof(REPLICA_EL), 10);
	if (err < 0)
	{
		fprintf(stderr, "couldn't create woof slave_progress_for_replica\n");
		fflush(stderr);
		exit(1);
	}
	printf("master started\n");
	printf("slave_progress_for_replica created\n");
	fflush(stdout);

	return (0);
}
