#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "mio.h"
#include "log.h"
#include "woofc.h"
#include "woofc-host.h"
#include "repair.h"

#define ARGS "H:F:W:S:"
char *Usage = "woof-repair -H ip1:port1,ip2:port2... -F glog_filename -W woof -S seq_no_1,seq_no_2,...\n";

extern unsigned long Name_id;

unsigned long hash(char *namespace)
{
	unsigned long h = 5381;
	unsigned long a = 33;
	unsigned long i;

	for (i = 0; i < strlen(namespace); i++)
	{
		h = ((h * a) + namespace[i]); /* no mod p due to wrap */
	}
	return (h);
}

int main(int argc, char **argv)
{
	int i;
	int c;
	int err;
	char *ptr;
	int num_hosts;
	int num_seq_no;
	unsigned long log_size;
	unsigned long glog_size;
	MIO **mio;
	LOG **log;
	GLOG *glog;
	char **endpoint;
	unsigned long *seq_no;
	char hosts[4096];
	char filename[4096];
	char woof[4096];
	char namespace[4096];
	char woof_name[4096];
	char seq_nos[4096];
	unsigned long woof_name_id;
	RB *woof_put;
	RB *woof_get;
	RB *rb;
	int count_woof_put;
	int count_woof_get;
	char **woof_rw;
	char **woof_ro;
	unsigned long wf_host;
	char *wf_name;

	memset(hosts, 0, sizeof(hosts));
	memset(filename, 0, sizeof(filename));
	memset(woof, 0, sizeof(woof));
	memset(namespace, 0, sizeof(namespace));
	memset(seq_nos, 0, sizeof(seq_nos));

	while ((c = getopt(argc, argv, ARGS)) != EOF)
	{
		switch (c)
		{
		case 'H':
			strncpy(hosts, optarg, sizeof(hosts));
			break;
		case 'F':
			strncpy(filename, optarg, sizeof(filename));
			break;
		case 'W':
			strncpy(woof, optarg, sizeof(woof));
			break;
		case 'S':
			strncpy(seq_nos, optarg, sizeof(seq_nos));
			break;
		default:
			fprintf(stderr, "unrecognized command %c\n", (char)c);
			fprintf(stderr, "%s", Usage);
			exit(1);
		}
	}

	if (filename[0] == 0 || hosts[0] == 0 || seq_nos[0] == 0)
	{
		fprintf(stderr, "%s", Usage);
		exit(1);
	}

	// parse hosts
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
		endpoint[i] = malloc(128 * sizeof(char));
		memset(endpoint[i], 0, sizeof(endpoint[i]));
	}

	i = 0;
	ptr = strtok(hosts, ",");
	while (ptr != NULL)
	{
		sprintf(endpoint[i], ">tcp://%s", ptr);
		ptr = strtok(NULL, ",");
		i++;
	}
#ifdef DEBUG
	printf("num_hosts: %d\n", num_hosts);
	fflush(stdout);
#endif

	// parse seq_no
	num_seq_no = 1;
	for (i = 0; seq_nos[i] != '\0'; i++)
	{
		if (seq_nos[i] == ',')
		{
			num_seq_no++;
		}
	}
	seq_no = malloc(num_seq_no * sizeof(unsigned long));

	i = 0;
	ptr = strtok(seq_nos, ",");
	while (ptr != NULL)
	{
		seq_no[i] = strtoul(ptr, (char **)NULL, 10);
		ptr = strtok(NULL, ",");
		i++;
	}
#ifdef DEBUG
	printf("num_seq_no: %d\n", num_seq_no);
	fflush(stdout);
#endif

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
		// LogPrint(stdout, log[i]);
		// fflush(stdout);
	}

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

#ifdef DEBUG
	printf("Global log:\n");
	GLogPrint(stdout, glog);
#endif

	err = WooFNameSpaceFromURI(woof, namespace, sizeof(namespace));
	if (err < 0)
	{
		fprintf(stderr, "woof: %s no name space\n", woof);
		fflush(stderr);
		return (-1);
	}
	err = WooFNameFromURI(woof, woof_name, sizeof(woof_name));
	if (err < 0)
	{
		fprintf(stderr, "woof: %s no woof name\n", woof);
		fflush(stderr);
		return (-1);
	}
	woof_name_id = hash(namespace);
#ifdef DEBUG
	printf("namespace: %s, id: %lu\n", namespace, woof_name_id);
	fflush(stdout);
#endif

	// GLogMarkWooFDownstreamRange(glog, woof_name_id, start_seq_no, end_seq_no);
	for (i = 0; i < num_seq_no; i++)
	{
		GLogMarkWooFDownstream(glog, woof_name_id, woof_name, seq_no[i]);
	}
	// GLogPrint(stdout, glog);
	// fflush(stdout);
#ifdef DEBUG
	printf("mark finished\n");
	fflush(stdout);
#endif

	woof_put = RBInitS();
	woof_get = RBInitS();
	GLogFindAffectedWooF(glog, woof_put, &count_woof_put, woof_get, &count_woof_get);
	woof_rw = malloc(count_woof_put * sizeof(char *));
	i = 0;
	RB_FORWARD(woof_put, rb)
	{
		woof_rw[i] = malloc(4096 * sizeof(char));
		memset(woof_rw[i], 0, sizeof(woof_rw[i]));
		sprintf(woof_rw[i], "%s", rb->key.key.s);
		free(rb->key.key.s);
		i++;
	}
	woof_ro = malloc(count_woof_get * sizeof(char *));
	i = 0;
	RB_FORWARD(woof_get, rb)
	{
		woof_ro[i] = malloc(4096 * sizeof(char));
		memset(woof_ro[i], 0, sizeof(woof_ro[i]));
		sprintf(woof_ro[i], "%s", rb->key.key.s);
		free(rb->key.key.s);
		i++;
	}
	RBDestroyI64(woof_put);
	RBDestroyI64(woof_get);

	printf("woof rw:\n");
	for (i = 0; i < count_woof_put; i++)
	{
		wf_name = malloc(4096 * sizeof(char));
		err = WooFFromHval(woof_rw[i], &wf_host, wf_name);
		if (err < 0)
		{
			fprintf(stderr, "cannot parse woof_rw %s\n", woof_rw[i]);
			fflush(stderr);
			return (-1);
		}
		printf("%lu %s\n", wf_host, wf_name);

		repair(glog, namespace, wf_host, wf_name);
		free(wf_name);
	}
	printf("woof ro:\n");
	for (i = 0; i < count_woof_get; i++)
	{
		wf_name = malloc(4096 * sizeof(char));
		err = WooFFromHval(woof_ro[i], &wf_host, wf_name);
		if (err < 0)
		{
			fprintf(stderr, "cannot parse woof_ro %s\n", woof_ro[i]);
			fflush(stderr);
			return (-1);
		}
		printf("%lu %s\n", wf_host, wf_name);
		free(wf_name);
	}

	// holes = DlistInit();
	// GLogFindMarkedWooF(glog, woof_name_id, wf[i], holes);
	// // #ifdef DEBUG
	// printf("repairing namespace %s/%s(%lu):", namespace, wf[i], woof_name_id);
	// DLIST_FORWARD(holes, dn)
	// {
	// 	printf(" %lu", dn->value.i64);
	// }
	// printf("\n");
	// fflush(stdout);
	// // #endif
	// if (holes->count > 0)
	// {
	// 	sprintf(woof, "woof://%s/%s", namespace, wf[i]);
	// 	WooFRepair(woof, holes);
	// 	DlistRemove(holes);
	// }

	return (0);
}

int repair(GLOG *glog, char *namespace, unsigned long host, char *wf_name)
{
	Dlist *holes;
	DlistNode *dn;
	char wf[4096];
	int err;

	holes = DlistInit();
	if (holes == NULL)
	{
		fprintf(stderr, "cannot allocate Dlist\n");
		fflush(stderr);
		return (-1);
	}

	GLogFindReplacedWooF(glog, host, wf_name, holes);
	// #ifdef DEBUG
	printf("repairing %s/%s(%lu): %d holes", namespace, wf_name, host, holes->count);
	DLIST_FORWARD(holes, dn)
	{
		printf(" %lu", dn->value.i64);
	}
	printf("\n");
	fflush(stdout);
	// #endif
	if (holes->count > 0)
	{
		sprintf(wf, "woof://%s/%s", namespace, wf_name);
		err = WooFRepair(wf, holes);
		if (err < 0)
		{
			fprintf(stderr, "cannot repair woof %s/%s(%lu)\n", namespace, wf_name, host);
			fflush(stderr);
			return (-1);
		}
		DlistRemove(holes);
	}
	return (0);
}