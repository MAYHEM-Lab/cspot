#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "mio.h"
#include "log.h"
#include "woofc.h"
#include "woofc-host.h"
#include "repair.h"

#define ARGS "H:F:W:S:O:"
char *Usage = "woof-repair -H ip1:port1,ip2:port2... -F glog_filename -W woof -S seq_no_1,seq_no_2,... -O guide_filename\n";

extern unsigned long Name_id;

int Repair(GLOG *glog, char *namespace, unsigned long host, char *wf_name);
int RepairRO(GLOG *glog, char *namespace, unsigned long host, char *wf_name);
void Guide(GLOG *glog, FILE *fd);

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
	char guide_file[4096];
	unsigned long woof_name_id;
	RB *root;
	RB *casualty;
	RB *progress;
	RB *rb;
	int count_root;
	int count_casualty;
	int count_progress;
	char **woof_root;
	char **woof_casualty;
	char **woof_progress;
	unsigned long wf_host;
	char *wf_name;
	RB *asker;
	RB *mapping_count;
	FILE *guide_fd;

	memset(hosts, 0, sizeof(hosts));
	memset(filename, 0, sizeof(filename));
	memset(woof, 0, sizeof(woof));
	memset(namespace, 0, sizeof(namespace));
	memset(seq_nos, 0, sizeof(seq_nos));
	memset(guide_file, 0, sizeof(guide_file));

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
		case 'O':
			strncpy(guide_file, optarg, sizeof(guide_file));
			break;
		default:
			fprintf(stderr, "unrecognized command %c\n", (char)c);
			fprintf(stderr, "%s", Usage);
			exit(1);
		}
	}

	if (filename[0] == 0 || hosts[0] == 0 || seq_nos[0] == 0 || guide_file[0] == 0)
	{
		fprintf(stderr, "%s", Usage);
		fflush(stderr);
		exit(1);
	}

	guide_fd = fopen(guide_file, "w");
	if (guide_fd == NULL)
	{
		fprintf(stderr, "couldn't open guide_file %s\n", guide_file);
		fflush(stderr);
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
	printf("num_hosts: %d\n", num_hosts);
	fflush(stdout);

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
	printf("num_seq_no: %d\n", num_seq_no);
	fflush(stdout);

	WooFInit();

	glog_size = 0;
	mio = malloc(num_hosts * sizeof(MIO *));
	log = malloc(num_hosts * sizeof(LOG *));
	for (i = 0; i < num_hosts; i++)
	{
		printf("pulling log from host %d...\n", i + 1);
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
		printf("pulled log from host %d\n", i + 1);
		fflush(stdout);
	}

	printf("creating GLog with size %lu...\n", glog_size);
	fflush(stdout);
	glog = GLogCreate(filename, Name_id, glog_size);
	if (glog == NULL)
	{
		fprintf(stderr, "couldn't create global log\n");
		fflush(stderr);
		exit(1);
	}
	printf("GLog created\n", glog_size);
	fflush(stdout);

	for (i = 0; i < num_hosts; i++)
	{
		printf("importing log from host %d...\n", i + 1);
		fflush(stdout);
		err = ImportLogTail(glog, log[i]);
		if (err < 0)
		{
			fprintf(stderr, "couldn't import log tail from %s\n", endpoint[i]);
			fflush(stderr);
			exit(1);
		}
		printf("imported log from host %d\n", i + 1);
		fflush(stdout);
	}

#ifdef DEBUG
	printf("Global log:\n");
	GLogPrint(stdout, glog);
	fflush(stdout);
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

	printf("GLogMarkWooFDownstream\n");
	for (i = 0; i < num_seq_no; i++)
	{
		GLogMarkWooFDownstream(glog, woof_name_id, woof_name, seq_no[i]);
	}
	GLogPrint(stdout, glog);
	fflush(stdout);
	printf("mark finished\n");
	fflush(stdout);

	printf("GLogFindAffectedWooF\n");
	root = RBInitS();
	casualty = RBInitS();
	progress = RBInitS();
	GLogFindAffectedWooF(glog, root, &count_root, casualty, &count_casualty, progress, &count_progress);
	woof_root = malloc(count_root * sizeof(char *));
	i = 0;
	RB_FORWARD(root, rb)
	{
		woof_root[i] = malloc(4096 * sizeof(char));
		memset(woof_root[i], 0, sizeof(woof_root[i]));
		sprintf(woof_root[i], "%s", rb->key.key.s);
		free(rb->key.key.s);
		i++;
	}
	woof_casualty = malloc(count_casualty * sizeof(char *));
	i = 0;
	RB_FORWARD(casualty, rb)
	{
		woof_casualty[i] = malloc(4096 * sizeof(char));
		memset(woof_casualty[i], 0, sizeof(woof_casualty[i]));
		sprintf(woof_casualty[i], "%s", rb->key.key.s);
		free(rb->key.key.s);
		i++;
	}
	woof_progress = malloc(count_progress * sizeof(char *));
	i = 0;
	RB_FORWARD(progress, rb)
	{
		woof_progress[i] = malloc(4096 * sizeof(char));
		memset(woof_progress[i], 0, sizeof(woof_progress[i]));
		sprintf(woof_progress[i], "%s", rb->key.key.s);
		free(rb->key.key.s);
		i++;
	}
	RBDestroyS(root);
	RBDestroyS(casualty);
	RBDestroyS(progress);
	printf("finished\n");

	printf("woof root:\n");
	for (i = 0; i < count_root; i++)
	{
		wf_name = malloc(4096 * sizeof(char));
		err = WooFFromHval(woof_root[i], &wf_host, wf_name);
		if (err < 0)
		{
			fprintf(stderr, "cannot parse woof_root %s\n", woof_root[i]);
			fflush(stderr);
			return (-1);
		}
		printf("%lu %s\n", wf_host, wf_name);
		Repair(glog, namespace, wf_host, wf_name);
		free(wf_name);
	}
	printf("woof casualty:\n");
	for (i = 0; i < count_casualty; i++)
	{
		wf_name = malloc(4096 * sizeof(char));
		err = WooFFromHval(woof_casualty[i], &wf_host, wf_name);
		if (err < 0)
		{
			fprintf(stderr, "cannot parse woof_casualty %s\n", woof_casualty[i]);
			fflush(stderr);
			return (-1);
		}
		printf("%lu %s\n", wf_host, wf_name);
		Repair(glog, namespace, wf_host, wf_name);
		free(wf_name);
	}
	printf("woof progress:\n");
	for (i = 0; i < count_progress; i++)
	{
		wf_name = malloc(4096 * sizeof(char));
		err = WooFFromHval(woof_progress[i], &wf_host, wf_name);
		if (err < 0)
		{
			fprintf(stderr, "cannot parse woof_progress %s\n", woof_progress[i]);
			fflush(stderr);
			return (-1);
		}
		printf("%lu %s\n", wf_host, wf_name);
		RepairRO(glog, namespace, wf_host, wf_name);
		free(wf_name);
	}

	Guide(glog, guide_fd);
	fclose(guide_fd);

	// free memory
	for (i = 0; i < num_hosts; i++)
	{
		free(endpoint[i]);
	}
	free(endpoint);
	free(seq_no);
	for (i = 0; i < num_hosts; i++)
	{
		MIOClose(mio[i]);
	}
	free(mio);
	free(log);
	GLogFree(glog);

	return (0);
}

int Repair(GLOG *glog, char *namespace, unsigned long host, char *wf_name)
{
	Dlist *holes;
	DlistNode *dn;
	char wf[4096];
	int err;

	holes = DlistInit();
	if (holes == NULL)
	{
		fprintf(stderr, "Repair: cannot allocate Dlist\n");
		fflush(stderr);
		return (-1);
	}

	GLogFindWooFHoles(glog, host, wf_name, holes);
	printf("Repair: repairing %s/%s(%lu): %d holes", namespace, wf_name, host, holes->count);
	DLIST_FORWARD(holes, dn)
	{
		printf(" %lu", dn->value.i64);
	}
	printf("\n");
	fflush(stdout);
	if (holes->count > 0)
	{
		// TODO: remote repair not implemented yet
		sprintf(wf, "woof://%s/%s", namespace, wf_name);
		err = WooFRepair(wf, holes);
		if (err < 0)
		{
			fprintf(stderr, "Repair: cannot repair woof %s/%s(%lu)\n", namespace, wf_name, host);
			fflush(stderr);
			DlistRemove(holes);
			return (-1);
		}
	}
	DlistRemove(holes);
	return (0);
}

int RepairRO(GLOG *glog, char *namespace, unsigned long host, char *wf_name)
{
	RB *asker;
	RB *mapping_count;
	char wf[4096];
	int err;
	RB *rb;
	RB *rbc;
	unsigned long cause_host;
	char cause_woof[4096];

	asker = RBInitS();
	mapping_count = RBInitS();

	GLogFindLatestSeqnoAsker(glog, host, wf_name, asker, mapping_count);
	RB_FORWARD(asker, rb)
	{
		rbc = RBFindS(mapping_count, rb->key.key.s);
		if (rbc == NULL)
		{
			fprintf(stderr, "RepairRO: can't find entry %s in mapping_count\n", rb->key.key.s);
			fflush(stderr);
			RBDestroyS(asker);
			RBDestroyS(mapping_count);
			return (-1);
		}
		WooFFromHval(rb->key.key.s, &cause_host, cause_woof);
		sprintf(wf, "woof://%s/%s", namespace, wf_name);
		printf("RepairRO: sending mapping from cause_woof %s to %s\n", cause_woof, wf_name);
		fflush(stdout);
		err = WooFRepairReadOnly(wf, cause_host, cause_woof, rbc->value.i, (unsigned long *)rb->value.i64);
		if (err < 0)
		{
			fprintf(stderr, "RepairRO: cannot repair woof %s/%s(%lu)\n", namespace, wf_name, host);
			fflush(stderr);
			RBDestroyS(asker);
			RBDestroyS(mapping_count);
			return (-1);
		}
		// TODO: remote repair not implemented yet
	}

	RBDestroyS(asker);
	RBDestroyS(mapping_count);

	return (0);
}

void Guide(GLOG *glog, FILE *fd)
{
	EVENT *ev_array;
	LOG *log;
	int err;
	unsigned long curr;
	Hval value;

#ifdef DEBUG
	printf("Guide: called\n", host, woof_name);
	fflush(stdout);
#endif

	printf("To repair the app, please call WooFPut by the following order:\n");

	log = glog->log;
	ev_array = (EVENT *)(((void *)log) + sizeof(LOG));

	curr = (log->tail + 1) % log->size;
	while (curr != (log->head + 1) % log->size)
	{
		if (ev_array[curr].type == (TRIGGER | ROOT))
		{
			printf(" host %lu, woof_name %s, seq_no %lu\n", ev_array[curr].host, ev_array[curr].woofc_name, ev_array[curr].woofc_seq_no);
			fprintf(fd, "%lu,%s,%lu\n", ev_array[curr].host, ev_array[curr].woofc_name, ev_array[curr].woofc_seq_no);
			fflush(fd);
		}
		curr = (curr + 1) % log->size;
	}
	fflush(stdout);
}
