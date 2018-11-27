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
char *Usage = "merge-init -E endpoint -F glog_filename\n";

char endpoint[4096];
char filename[4096];

int main(int argc, char **argv)
{
	int c;
	int err;
	unsigned long int log_space;
	MIO *mio;
	LOG *log;
	GLOG *glog;

	while ((c = getopt(argc, argv, ARGS)) != EOF)
	{
		switch (c)
		{
		case 'E':
			strncpy(endpoint, optarg, sizeof(endpoint));
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

	if (endpoint[0] == 0)
	{
		fprintf(stderr, "%s", Usage);
		exit(1);
	}

	if (filename[0] == 0)
	{
		fprintf(stderr, "%s", Usage);
		exit(1);
	}
	
	WooFInit();

	log_space = LogGetRemoteSize(endpoint);
	if (log_space < 0)
	{
		fprintf(stderr, "couldn't get remote log size from %s\n", endpoint);
		exit(1);
	}
	mio = MIOMalloc(log_space);
	log = (LOG *)MIOAddr(mio);
	err = LogGetRemote(log, mio, endpoint);
	if (err < 0)
	{
		fprintf(stderr, "couldn't get remote log from %s\n", endpoint);
		exit(1);
	}
	LogPrint(stdout, log);
	fflush(stdout);

	glog = GLogCreate(filename, 0, log->size * 2);
	if (glog == NULL)
	{
		fprintf(stderr, "couldn't create global log\n");
		exit(1);
	}
	err = ImportLogTail(glog, log);
	if (err < 0)
	{
		fprintf(stderr, "couldn't import log tail from %s\n", endpoint);
		exit(1);
	}
	GLogPrint(stdout, glog);
	fflush(stdout);

	return (0);
}
