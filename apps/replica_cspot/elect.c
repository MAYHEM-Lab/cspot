#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "woofc-host.h"
#include "woofc-access.h"
#include "replica.h"

char *Usage = "elect slave1 slave2...\n";

int main(int argc, char **argv)
{
	int err, len, i;
	int max_seq, max_i;
	char **slaves;
	char woofname[256];
	unsigned long seq;
	SLAVE_PROGRESS slave_progress;

	if (argc == 1) {
		printf("%s", Usage);
		exit(1);
	}
	len = argc - 1;

	slaves = malloc(sizeof(char *) * len);
	for (i = 0; i < len; i++) {
		slaves[i] = malloc(strlen(argv[i + 1]) + 1);
		strcpy(slaves[i], argv[i + 1]);
	}

	WooFInit();

	max_seq = 0;
	max_i = 0;
	for (i = 0; i < len; i++) {
		printf("slaves[%d]: %s\n", i, slaves[i]);
		fflush(stdout);
		sprintf(woofname, "%s/%s", slaves[i], PROGRESS_WOOF_NAME);
		seq = WooFGetLatestSeqno(woofname);
		if (WooFInvalid(seq)) {
			fprintf(stderr, "couldn't get the latest seqno from woof %s\n", woofname);
			fflush(stderr);
			exit(1);
		}
		err = WooFGet(woofname, &slave_progress, seq);
		if (err < 0) {
			fprintf(stderr, "couldn't get the latest slave progress from woof %s\n", woofname);
			fflush(stderr);
			exit(1);
		}
		if (slave_progress.log_seqno > max_seq) {
			max_seq = slave_progress.log_seqno;
			max_i = i;
		}
	}

	printf("Slave %d has the latest update, seq: %lu\n", max_i, max_seq);
	printf("WooF: %s\n", slaves[max_i]);
	fflush(stdout);

	return (0);
}
