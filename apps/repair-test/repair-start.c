#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "woofc-host.h"
#include "repair.h"

#define ARGS "h:c:"
char *Usage = "repair-init -h history_size -c count\n";

int main(int argc, char **argv)
{
	int i;
	int c;
	int err;
	unsigned long history_size;
	unsigned long element_size;
	unsigned long count;
	unsigned long seq_no;
	unsigned long latest_seq_no;
	WOOF *wf;
	REPAIR_EL el;

	history_size = 10;
	element_size = sizeof(REPAIR_EL);

	while ((c = getopt(argc, argv, ARGS)) != EOF)
	{
		switch (c)
		{
		case 'h':
			history_size = strtoul(optarg, (char **)NULL, 10);
			break;
		case 'c':
			count = strtoul(optarg, (char **)NULL, 10);
			break;
		default:
			fprintf(stderr,
					"unrecognized command %c\n", (char)c);
			fprintf(stderr, "%s", Usage);
			exit(1);
		}
	}

	WooFInit();

	// memset(&el, 0, sizeof(REPAIR_EL));
	// sprintf(el.string, "repaired");
	err = WooFCopy("test", element_size, history_size, count);
	if (err < 0)
	{
		fprintf(stderr, "couldn't repair woof %s\n", "test");
		fflush(stderr);
		return (0);
	}

	printf("original:\n");
	WooFPrintMeta(stdout, "test");
	WooFDump(stdout, "test");

	printf("\nshadow:\n");
	WooFPrintMeta(stdout, "test_repair");
	WooFDump(stdout, "test_repair");

	fflush(stdout);
	return (0);
}
