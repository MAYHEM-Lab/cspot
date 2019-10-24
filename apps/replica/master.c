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
#include "woofc.h"
#include "woofc-host.h"

#define ARGS "p:"
char *Usage = "master -p port\n";

extern LOG *Name_log;

int main(int argc, char **argv)
{
	int c;
	int n;
	int err;
	unsigned long master_seq, slave_seq;
	unsigned long num_events;
	unsigned long curr;
	int listenfd = 0, connfd = 0;
	struct sockaddr_in serv_addr;
	int port = 8080;
	EVENT *ev_array;

	char buf[2048];

	while ((c = getopt(argc, argv, ARGS)) != EOF)
	{
		switch (c)
		{
		case 'p':
			port = atoi(optarg);
			break;
		default:
			fprintf(stderr, "unrecognized command %c\n", (char)c);
			fprintf(stderr, "%s", Usage);
			exit(1);
		}
	}

	WooFInit();
	ev_array = (EVENT *)(((void *)Name_log) + sizeof(LOG));

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	memset(&serv_addr, 0, sizeof(serv_addr));
	memset(buf, 0, sizeof(buf));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);
	bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	listen(listenfd, 1);

	while (1)
	{
		connfd = accept(listenfd, (struct sockaddr *)NULL, NULL);

		memset(buf, 0, sizeof(buf));
		while (1)
		{
			n = read(connfd, buf, sizeof(buf));
			while (n < sizeof(buf))
			{
				n += read(connfd, buf + n, sizeof(buf) - n);
			}
			slave_seq = strtoul(buf, (char **)NULL, 10);
			master_seq = ev_array[Name_log->head].seq_no;

			// send (master_seq - slave_seq) events to slave
			memset(buf, 0, sizeof(buf));
			if (master_seq <= slave_seq)
			{
				num_events = 0;
			}
			else
			{
				num_events = master_seq - slave_seq;
				if (num_events > 10000)
				{
					num_events = 10000;
				}
			}
			sprintf(buf, "%lu", num_events);
			if (num_events > 0)
			{
				printf("latest seqno from slave: %lu, master: %lu\n", slave_seq, master_seq);
				printf("sending %lu events\n", num_events);
				fflush(stdout);
			}
			write(connfd, buf, sizeof(buf));
			while (num_events > 0)
			{
				num_events--;
				if (ev_array[Name_log->tail].seq_no > slave_seq + 1)
				{
					printf("already rolled over\n");
					fflush(stdout);
				}
				curr = Name_log->head;
				while (ev_array[curr].seq_no != slave_seq + 1)
				{
					curr = curr - 1;
					if (curr >= Name_log->size)
					{
						curr = (Name_log->size - 1);
					}
				}
				memset(buf, 0, sizeof(buf));
				memcpy(buf, &ev_array[curr], sizeof(EVENT));

				// printf("host: %lu seq_no: %llu r_host: %lu r_seq_no: %llu woofc_seq_no: %lu woof: %s type: %d timestamp: %lu\n",
				// 	ev_array[curr].host,
				// 	ev_array[curr].seq_no,
				// 	ev_array[curr].cause_host,
				// 	ev_array[curr].cause_seq_no,
				// 	ev_array[curr].woofc_seq_no,
				// 	ev_array[curr].woofc_name,
				// 	ev_array[curr].type,
				// 	ev_array[curr].timestamp);

				write(connfd, buf, sizeof(buf));
				fflush(stdout);
				slave_seq++;
			}
			// V(&Name_log->mutex);
		}

		close(connfd);
		usleep(100000);
	}
}
