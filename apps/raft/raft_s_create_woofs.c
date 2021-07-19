#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "woofc.h"
// #include "woofc-host.h"
#include "raft.h"

int main(int argc, char **argv) {
	WooFInit();
	if (raft_create_woofs() < 0) {
		fprintf(stderr, "Can't create WooFs\n");
		exit(1);
	}
	return 0;
}

