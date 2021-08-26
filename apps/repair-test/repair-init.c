#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "mio.h"
#include "log.h"
#include "woofc.h"
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
	unsigned long seq_no;
	unsigned long count;
	WOOF *wf;
	REPAIR_EL el;

	history_size = 10;
	count = 5;
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

	err = WooFCreate("test", element_size, history_size);
	if (err < 0)
	{
		fprintf(stderr, "couldn't create woof %s\n", "test");
		fflush(stderr);
		return (0);
	}

	for (i = 1; i <= count; i++)
	{
		memset(&el, 0, sizeof(REPAIR_EL));
		sprintf(el.string, "original_%d", i);
		seq_no = WooFPut("test", NULL, &el);
	}

	return (0);
}
