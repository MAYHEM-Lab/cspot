#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "mio.h"
#include "log.h"
#include "woofc.h"
#include "woofc-host.h"
#include "merge.h"
#include "repair.h"

#define ARGS "E:F:"
char *Usage = "merge-init -E ip1:port1,ip2:port2... -F glog_filename\n";

char hosts[4096];
char filename[4096];
extern unsigned long Name_id;

int main(int argc, char **argv)
{
	int i;
	int c;
	int err;
	char *ptr;
	int num_hosts;
	unsigned long log_size;
	unsigned long glog_size;
	MIO **mio;
	LOG **log;
	GLOG *glog;
	char **endpoint;

	while ((c = getopt(argc, argv, ARGS)) != EOF)
	{
		switch (c)
		{
		case 'E':
			strncpy(hosts, optarg, sizeof(hosts));
			break;
		case 'F':
			strncpy(filename, optarg, sizeof(filename));
			break;
		default:
			fprintf(stderr, "unrecognized command %c\n", (char)c);
			fprintf(stderr, "%s", Usage);
			exit(1);
		}
	}

	if (filename[0] == 0)
	{
		fprintf(stderr, "%s", Usage);
		exit(1);
	}

	if (hosts[0] == 0)
	{
		fprintf(stderr, "%s", Usage);
		exit(1);
	}
	num_hosts = 1;
	for (i = 0; hosts[i] != '\0'; i++)
	{
		if (hosts[i] == ',')
		{
			num_hosts++;
		}
	}
	endpoint = malloc(num_hosts * sizeof(char *));
	for (i = 0; i < num_hosts; i++)
	{
		endpoint[i] = malloc(128);
		memset(endpoint[i], 0, sizeof(endpoint[i]));
	}
	i = 0;
	ptr = strtok(hosts, ",");
	while (ptr != NULL)
	{
		sprintf(endpoint[i], ">tcp://%s", ptr);
		ptr = strtok(NULL, ",");
		i++;
		;
	}

	WooFInit();

	glog_size = 0;
	mio = malloc(num_hosts * sizeof(MIO *));
	log = malloc(num_hosts * sizeof(LOG *));
	for (i = 0; i < num_hosts; i++)
	{
		log_size = LogGetRemoteSize(endpoint[i]);
		if (log_size < 0)
		{
			fprintf(stderr, "couldn't get remote log size from %s\n", endpoint[i]);
			exit(1);
		}
		mio[i] = MIOMalloc(log_size);
		log[i] = (LOG *)MIOAddr(mio[i]);
		err = LogGetRemote(log[i], mio[i], endpoint[i]);
		if (err < 0)
		{
			fprintf(stderr, "couldn't get remote log from %s\n", endpoint[i]);
			exit(1);
		}
		glog_size += log[i]->size;
		LogPrint(stdout, log[i]);
		fflush(stdout);
	}

	printf("Name_id: %lu\n", Name_id);
	fflush(stdout);

	glog = GLogCreate(filename, Name_id, glog_size);
	if (glog == NULL)
	{
		fprintf(stderr, "couldn't create global log\n");
		exit(1);
	}
	for (i = 0; i < num_hosts; i++)
	{
		err = ImportLogTail(glog, log[i]);
		if (err < 0)
		{
			fprintf(stderr, "couldn't import log tail from %s\n", endpoint[i]);
			exit(1);
		}
	}
	printf("\nBEFORE\n");
	GLogPrint(stdout, glog);
	GLogMarkWooFDownstream(glog, 4204335350254715064ul, 3);
	printf("\nAFTER\n");
	GLogPrint(stdout, glog);

	unsigned long begin, end, cnt;
	RB *seq;
	RB *rb;
	seq = RBInitI64();

	Dlist *holes;
	DlistNode *dn;

	holes = DlistInit();
	GLogFindMarkedWooF(glog, 4204335350254715064ul, holes);
	printf("%lu:", 4204335350254715064ul);
	DLIST_FORWARD(holes, dn)
	{
		printf(" %lu", dn->value.i64);
	}
	printf("\n");
	WooFRepair("woof://10.1.5.1:55064/home/centos/cspot/apps/cause-test/cspot/test", holes);
	DlistRemove(holes);

	printf("Dumping original\n");
	WooFDump(stdout, "woof://10.1.5.1:55064/home/centos/cspot/apps/cause-test/cspot/test");
	printf("Dumping shadow\n");
	WooFDump(stdout, "woof://10.1.5.1:55064/home/centos/cspot/apps/cause-test/cspot/test_shadow");
	printf("\n");

	MERGE_EL el;
	memset(&el, 0, sizeof(el));
	sprintf(el.string, "repaired");

	unsigned long seq_no;
	seq_no = WooFPut("woof://10.1.5.1:55064/home/centos/cspot/apps/cause-test/cspot/test", "cause_recv", &el);
	printf("\nseq_no: %lu\n", seq_no);
	printf("Dumping original\n");
	WooFDump(stdout, "woof://10.1.5.1:55064/home/centos/cspot/apps/cause-test/cspot/test");
	printf("Dumping shadow\n");
	WooFDump(stdout, "woof://10.1.5.1:55064/home/centos/cspot/apps/cause-test/cspot/test_shadow");
	printf("\n");

	holes = DlistInit();
	GLogFindMarkedWooF(glog, 1877160991625576554ul, holes);
	printf("\n%lu:", 1877160991625576554ul);
	DLIST_FORWARD(holes, dn)
	{
		printf(" %lu", dn->value.i64);
	}
	printf("\n");
	WooFRepair("woof://10.1.5.1:55064/home/centos/cspot/apps/cause-test/cspot/test", holes);
	DlistRemove(holes);
	fflush(stdout);

	return (0);
}
