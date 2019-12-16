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
	err = WooFCreate(PROGRESS_WOOF_NAME, sizeof(SLAVE_PROGRESS), 10);
	if (err < 0)
	{
		fprintf(stderr, "couldn't create woof %s\n", PROGRESS_WOOF_NAME);
		fflush(stderr);
		exit(1);
	}
	printf("master started\n");
	printf("%s created\n", PROGRESS_WOOF_NAME);
	fflush(stdout);

	return (0);
}
