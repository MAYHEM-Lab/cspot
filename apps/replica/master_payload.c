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
char *Usage = "master_payload -p port\n";

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
	unsigned long num_latest = 0, num_read = 0, num_append = 0;
	WOOF *woof;
	void *payload;

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

	printf("start listening\n");
	fflush(stdout);
	while (1)
	{
		printf("waiting connection\n");
		fflush(stdout);
		connfd = accept(listenfd, (struct sockaddr *)NULL, NULL);

		memset(buf, 0, sizeof(buf));
		while (1)
		{
			n = read(connfd, buf, sizeof(buf));
			while (n < sizeof(buf))
			{
				n += read(connfd, buf + n, sizeof(buf) - n);
			}
			if (buf[0] == '-' && buf[1] == '1')
			{
				printf("Number of latest_seqno: %lu\n", num_latest);
				printf("Number of reads: %lu\n", num_read);
				printf("Number of appends: %lu\n", num_append);
				fflush(stdout);
				close(connfd);
				return 0;
			}
			slave_seq = strtoul(buf, (char **)NULL, 10);
			
			if (ev_array[Name_log->tail].seq_no > slave_seq + 1)
			{
				fprintf(stderr, "already rolled over\n");
				fflush(stderr);
				return 1;
			}

			master_seq = ev_array[Name_log->head].seq_no;
			// count how many events to be sent
			if (ev_array[0].seq_no < slave_seq + 1)
			{
				curr = (slave_seq + 1 - ev_array[0].seq_no);
			}
			else if (ev_array[0].seq_no > slave_seq + 1)
			{
				curr = (Name_log->size - (ev_array[0].seq_no - (slave_seq + 1)));
			}
			num_events = 0;
			while (curr != (Name_log->head + 1) % Name_log->size)
			{
				if (ev_array[curr].type == APPEND)
				{
					num_events++;
				}
				curr = (curr + 1) % Name_log->size;
			}
			// tell slave the number of events to be sent
			memset(buf, 0, sizeof(buf));
			sprintf(buf, "%lu", num_events);
			printf("latest seqno from slave: %lu, master: %lu\n", slave_seq, master_seq);
			printf("sending %lu append events\n", num_events);
			fflush(stdout);
			write(connfd, buf, sizeof(buf));

			// send the events
			if (ev_array[0].seq_no < slave_seq + 1)
			{
				curr = (slave_seq + 1 - ev_array[0].seq_no);
			}
			else if (ev_array[0].seq_no > slave_seq + 1)
			{
				curr = (Name_log->size - (ev_array[0].seq_no - (slave_seq + 1)));
			}
			while (curr != (Name_log->head + 1) % Name_log->size)
			{
				if (ev_array[curr].type == LATEST_SEQNO)
				{
					num_latest++;
				}
				else if (ev_array[curr].type == READ)
				{
					num_read++;
				}
				else if (ev_array[curr].type == APPEND)
				{
					num_append++;
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
					// fflush(stdout);
					write(connfd, buf, sizeof(buf));
					woof = WooFOpen(ev_array[curr].woofc_name);
					if (woof == NULL){
						fprintf(stderr, "failed to open WooF %s\n", ev_array[curr].woofc_name);
						fflush(stderr);
						exit(1);
					}
					// telling the slave the element size
					memset(buf, 0, sizeof(buf));
					sprintf(buf, "%lu", woof->shared->element_size);
					write(connfd, buf, sizeof(buf));
					// printf("told slave the element size is %lu\n", woof->shared->element_size);
					// fflush(stdout);
					payload = malloc(woof->shared->element_size);
					if (payload == NULL)
					{
						fprintf(stderr, "failed to allocate memory for payload\n");
						fflush(stderr);
						exit(1);
					}
					err = WooFRead(woof, payload, ev_array[curr].woofc_seq_no);
					if (err < 0)
					{
						fprintf(stderr, "failed to read woof %s[%lu]\n", ev_array[curr].woofc_name, ev_array[curr].woofc_seq_no);
						fflush(stderr);
						exit(1);
					}
					WooFDrop(woof);
					write(connfd, payload, sizeof(payload));
					// printf("send the element\n");
					// fflush(stdout);
					
					free(payload);
				}
				curr = (curr + 1) % Name_log->size;
			}
			sleep(1);
		}

		close(connfd);
		sleep(1);
	}
	return 0;
}