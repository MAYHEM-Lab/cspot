#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#include "mio.h"
#include "log.h"
#include "event.h"
#include "woofc.h"
#include "woofc-host.h"
#include "woofc-access.h"

#define ARGS "m:p:f:"
char *Usage = "slave -m master_namespace -p port -f log_name\n";

extern LOG *Name_log;
char log_name[4096];

int main(int argc, char **argv)
{
	int c;
	int err;
	char master_namespace[4096];
	unsigned long history_size;
	unsigned long element_size;
	unsigned long seq_no;
	LOG *log;
    MIO *lmio;
	EVENT *ev_array;
	EVENT ev;
	char woof_name[4096];
	unsigned long i;
	struct stat sbuf;
	WOOF *woof;
	void *buffer;
	unsigned long master_seq, slave_seq;
	unsigned long num_events;

	int port = 8080;
	int sockfd = 0, n = 0;
	char buf[2048];
	char master_ip[1024];
	struct sockaddr_in serv_addr;

	while ((c = getopt(argc, argv, ARGS)) != EOF)
	{
		switch (c)
		{
		case 'm':
			strncpy(master_namespace, optarg, sizeof(master_namespace));
			break;
		case 'f':
			strncpy(log_name, optarg, sizeof(log_name));
			break;
		case 'p':
			port = atoi(optarg);
			break;
		default:
			fprintf(stderr, "unrecognized command %c\n", (char)c);
			fprintf(stderr, "%s", Usage);
			exit(1);
		}
	}

	if (log_name[0] == 0 || master_namespace[0] == 0)
	{
		fprintf(stderr, "must specify log name\n");
		fprintf(stderr, "%s", Usage);
		exit(1);
	}
	err = WooFIPAddrFromURI(master_namespace, master_ip, sizeof(master_ip));
	if (err < 0)
	{
		fprintf(stderr, "couldn't get master IP\n");
		fflush(stderr);
		exit(1);
	}

	WooFInit();

	lmio = MIOReOpen(log_name);
	if (lmio == NULL)
	{
		fprintf(stderr, "couldn't open mio for log %s\n", log_name);
		fflush(stderr);
		exit(1);
	}
	log = (LOG *)MIOAddr(lmio);
	if (log == NULL)
	{
		fprintf(stderr, "couldn't open log as %s\n", log_name);
		fflush(stderr);
		exit(1);
	}

	ev_array = (EVENT *)(((void *)Name_log) + sizeof(LOG));
	printf("log head: %lu\n", Name_log->head);
	printf("log tail: %lu\n", Name_log->tail);
	printf("log head seqno: %lu\n", ev_array[Name_log->head].seq_no);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	inet_pton(AF_INET, master_ip, &serv_addr.sin_addr);

	while (1)
	{
		connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

		// send the latest event seqno to master
		slave_seq = ev_array[Name_log->head].seq_no;
		sprintf(buf, "%lu", slave_seq);
		write(sockfd, buf, sizeof(buf));

		n = read(sockfd, buf, sizeof(buf));
		num_events = strtoul(buf, (char **)NULL, 10);
		if (num_events > 0)
		{
			printf("receiving %lu events\n", num_events);
		}
		for (i = 0; i < num_events; i++)
		{
			n = read(sockfd, buf, sizeof(EVENT));
			memcpy(&ev, buf, sizeof(EVENT));
			// printf("host: %lu seq_no: %llu r_host: %lu r_seq_no: %llu woofc_seq_no: %lu woof: %s type: %d timestamp: %lu\n",
			// 	ev.host,
			// 	ev.seq_no,
			// 	ev.cause_host,
			// 	ev.cause_seq_no,
			// 	ev.woofc_seq_no,
			// 	ev.woofc_name,
			// 	ev.type,
			// 	ev.timestamp);

			if (ev.woofc_name[0] != 0 && stat(ev.woofc_name, &sbuf) < 0)
			{
				// get meta
				sprintf(woof_name, "%s/%s", master_namespace, ev.woofc_name);
				element_size = WooFMsgGetElSize(woof_name);
				// history_size = WooFMsgGetHistorySize(ev.woofc_name);
				history_size = 10000;

				// woof = WooFOpen(woof_name);
				err = WooFCreate(ev.woofc_name, element_size, history_size);
				if (err < 0)
				{
					fprintf(stderr, "can't create woof %s\n", ev.woofc_name);
					exit(1);
				}
			}
			if (ev.type == LATEST_SEQNO)
			{
				WooFGetLatestSeqno(ev.woofc_name);
				// printf("WooFGetLatestSeqno(ev.woofc_name);\n");
			}
			else if (ev.type == READ)
			{
				woof = WooFOpen(ev.woofc_name);
				buffer = (void *)malloc(woof->shared->element_size);
				WooFDrop(woof);
				WooFGet(ev.woofc_name, buffer, ev.woofc_seq_no);
				// printf("WooFGet(ev.woofc_name, buffer, ev.woofc_seq_no);\n");
			}
			else if (ev.type == APPEND)
			{
				woof = WooFOpen(ev.woofc_name);
				buffer = (void *)malloc(woof->shared->element_size);
				WooFDrop(woof);
				sprintf(woof_name, "%s/%s", master_namespace, ev.woofc_name);
				// WooFGetWithoutEvent(woof_name, buffer, ev.woofc_seq_no);
				WooFGet(woof_name, buffer, ev.woofc_seq_no);
				seq_no = WooFPut(ev.woofc_name, NULL, buffer);
			}
		}
		fflush(stdout);
		usleep(100000);
	}
	
    fflush(stdout);
	return (0);
}
