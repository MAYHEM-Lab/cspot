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

	memset(&el, 0, sizeof(REPAIR_EL));
	sprintf(el.string, "repaired");
	// err = WooFRepairPut("test", 2, &el);
	err = WooFCopy("test", element_size, history_size, count);
	if (err < 0)
	{
		fprintf(stderr, "couldn't repair woof %s\n", "test");
		fflush(stderr);
		return (0);
	}

	printf("original:\n");
	WooFPrintMeta(stdout, "test");
	latest_seq_no = WooFGetLatestSeqno("test");
	printf("latest_seq_no: %lu\n", latest_seq_no);
	fflush(stdout);
	if (latest_seq_no > history_size)
	{
		i = latest_seq_no - history_size + 1;
	}
	else
	{
		i = 1;
	}
	for (; i <= latest_seq_no; i++)
	{
		err = WooFGet("test", &el, i);
		if (err < 0)
		{
			fprintf(stderr, "couldn't get woof %s[%d]\n", "test", i);
			fflush(stderr);
			return (0);
		}
		printf("woof[%d]: %s\n", i, el.string);
	}

	printf("\nshadow:\n");
	WooFPrintMeta(stdout, "test_repair");
	latest_seq_no = WooFGetLatestSeqno("test_repair");
	for (i = latest_seq_no; i > 0; i--)
	{
		err = WooFGet("test_repair", &el, i);
		if (err < 0)
		{
			fprintf(stderr, "couldn't get woof %s[%d]\n", "test_repair", i);
			fflush(stderr);
			return (0);
		}
		printf("woof[%d]: %s\n", i, el.string);
	}

	fflush(stdout);
	return (0);
}
