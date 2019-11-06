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
	REPLICA_EL el;
	unsigned long seq;

	while ((c = getopt(argc, argv, ARGS)) != EOF)
	{
		switch (c)
		{
		// case 'S':
		// 	strncpy(slave_namespace, optarg, sizeof(slave_namespace));
		// 	break;
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
	// printf("slave_namespace: %s\n", slave_namespace);
	// fflush(stdout);

	err = WooFCreate("events_woof_for_replica", sizeof(SLAVE_PROGRESS), 1000);
	if (err < 0)
	{
		fprintf(stderr, "couldn't create woof events_woof_for_replica\n");
		fflush(stderr);
		exit(1);
	}
	printf("events_woof_for_replica created\n");
	fflush(stdout);

	memset(el.events_woof, 0, sizeof(el.events_woof));
	sprintf(el.events_woof, "%s/events_woof_for_replica", slave_namespace);
	el.log_seqno = 0;
	// el.events_count = 0;

	sprintf(master_woof, "%s/slave_progress_for_replica", master_namespace);
	seq = WooFPut(master_woof, "replicate_event", (void *)&el);
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
