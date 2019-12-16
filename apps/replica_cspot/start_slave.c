#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "woofc-host.h"
#include "replica.h"

#define ARGS "M:"
char *Usage = "start_slave -M master_woof\n";

extern char WooF_namespace[2048];

int main(int argc, char **argv)
{
	int c;
	int err;
	char slave_namespace[256];
	char master_namespace[256];
	char master_woof[4096];
	char local_ip[25];
	unsigned int port;
	SLAVE_PROGRESS slave_progress;
	unsigned long seq;

	while ((c = getopt(argc, argv, ARGS)) != EOF)
	{
		switch (c)
		{
		case 'M':
			strncpy(master_namespace, optarg, sizeof(master_namespace));
			break;
		default:
			fprintf(stderr, "unrecognized command %c\n", (char)c);
			fprintf(stderr, "%s", Usage);
			exit(1);
		}
	}

	if (master_namespace[0] == 0)
	{
		fprintf(stderr, "must specify master_namespace\n");
		fprintf(stderr, "%s", Usage);
		fflush(stderr);
		exit(1);
	}

	WooFInit();

	err = WooFLocalIP(local_ip, sizeof(local_ip));
	if (err < 0)
	{
		fprintf(stderr, "couldn't get local IP\n");
		fflush(stderr);
		exit(1);
	}
	port = WooFPortHash(WooF_namespace);
	sprintf(slave_namespace, "woof://%s:%u%s", local_ip, port, WooF_namespace);

	// create events woof
	err = WooFCreate(EVENTS_WOOF_NAME, sizeof(EVENT_BATCH), 1000);
	if (err < 0)
	{
		fprintf(stderr, "couldn't create woof %s\n", EVENTS_WOOF_NAME);
		fflush(stderr);
		exit(1);
	}
	printf("%s created\n", EVENTS_WOOF_NAME);
	fflush(stdout);

	// create progress woof
	err = WooFCreate(PROGRESS_WOOF_NAME, sizeof(SLAVE_PROGRESS), 1000);
	if (err < 0)
	{
		fprintf(stderr, "couldn't create woof %s\n", PROGRESS_WOOF_NAME);
		fflush(stderr);
		exit(1);
	}
	printf("%s created\n", PROGRESS_WOOF_NAME);
	fflush(stdout);

	memset(slave_progress.events_woof, 0, sizeof(slave_progress.events_woof));
	sprintf(slave_progress.events_woof, "%s/%s", slave_namespace, EVENTS_WOOF_NAME);
	slave_progress.log_seqno = 0;

	sprintf(master_woof, "%s/%s", master_namespace, PROGRESS_WOOF_NAME);
	seq = WooFPut(master_woof, "replicate_event", (void *)&slave_progress);
	if (WooFInvalid(seq))
	{
		fprintf(stderr, "failed to put to %s\n", master_woof);
		fflush(stderr);
		exit(1);
	}
	printf("sent slave registration to %s\n", master_woof);
	fflush(stdout);

	return (0);
}
