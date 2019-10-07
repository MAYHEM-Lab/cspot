#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <openssl/sha.h>

#include "woofc.h"
#include "woofc-host.h"
#include "dht.h"

#define ARGS "t:n:"
char *Usage = "find -t topic -n node_addr\n";

int main(int argc, char **argv)
{
	char topic[256];
	char node_addr[256];
	char local_woof[256];
	char remote_woof[256];
	unsigned char topic_hash[SHA_DIGEST_LENGTH];
	char msg[256];
	int c;
	int i;
	int err;
	FIND_SUCESSOR_ARG arg;
	unsigned long seq_no;

	memset(topic, 0, sizeof(topic));
	memset(node_addr, 0, sizeof(node_addr));

	while ((c = getopt(argc, argv, ARGS)) != EOF)
	{
		switch (c)
		{
		case 't':
			strncpy(topic, optarg, sizeof(topic));
			break;
		case 'n':
			strncpy(node_addr, optarg, sizeof(node_addr));
			break;
		default:
			fprintf(stderr,
					"unrecognized command %c\n", (char)c);
			fprintf(stderr, "%s", Usage);
			exit(1);
		}
	}

	if (topic[0] == 0)
	{
		fprintf(stderr, "must specify topic\n");
		fprintf(stderr, "%s", Usage);
		fflush(stderr);
		exit(1);
	}

	err = node_woof_name(local_woof);
	if (err < 0)
	{
		fprintf(stderr, "find: couldn't get local node's woof name\n");
		fflush(stderr);
		exit(1);
	}

	if (node_addr[0] == 0)
	{
		fprintf(stderr, "must specify node woof\n");
		fprintf(stderr, "%s", Usage);
		fflush(stderr);
		exit(1);
	}

	find_init(local_woof, "find_result", topic, &arg);

	WooFInit();
	sprintf(remote_woof, "%s/%s", node_addr, DHT_FIND_SUCESSOR_ARG_WOOF);
	seq_no = WooFPut(remote_woof, "find_successor", &arg);
	if (WooFInvalid(seq_no))
	{
		fprintf(stderr, "couldn't put woof to %s\n", DHT_FIND_SUCESSOR_ARG_WOOF);
		fflush(stderr);
		exit(1);
	}

	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	printf("%lu, %lu\n", seq_no, ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
	fflush(stdout);

	return (0);
}
