#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "mio.h"
#include "log.h"
#include "woofc.h"
#include "woofc-host.h"
#include "merge.h"
#include "log-merge.h"

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

	glog = GLogCreate(filename, 0, glog_size);
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
	GLogPrint(stdout, glog);
	fflush(stdout);

	return (0);
}
