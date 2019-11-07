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

#define ARGS "m:p:"
char *Usage = "slave_payload -m master_namespace -p port\n";

extern LOG *Name_log;

int main(int argc, char **argv)
{
	int c;
	int err;
	char master_namespace[4096];
	unsigned long history_size;
	unsigned long element_size;
	unsigned long seq_no;
	EVENT *ev_array;
	EVENT ev;
	char woof_name[4096];
	unsigned long i;
	struct stat sbuf;
	unsigned long master_seq, slave_seq;
	unsigned long num_events;
	int port = 8080;
	int sockfd = 0, n = 0;
	char buf[2048];
	char master_ip[1024];
	struct sockaddr_in serv_addr;
	void *payload;

	struct timeval socket_t1, socket_t2;
	struct timeval read_t1, read_t2;
	struct timeval begin, end;
	double socket_time = 0, read_time = 0;

	while ((c = getopt(argc, argv, ARGS)) != EOF)
	{
		switch (c)
		{
		case 'm':
			strncpy(master_namespace, optarg, sizeof(master_namespace));
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

	if (master_namespace[0] == 0)
	{
		fprintf(stderr, "must specify master_namespace\n");
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

	ev_array = (EVENT *)(((void *)Name_log) + sizeof(LOG));

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	inet_pton(AF_INET, master_ip, &serv_addr.sin_addr);
	
	master_seq = 0;
	
	while (1)
	{
		connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

		// tell master the latest event it has seen
		if (master_seq == 0)
		{
			gettimeofday(&begin, NULL);
			printf("received the first event at %lu ms\n", (unsigned long)(begin.tv_sec * 1000.0 + begin.tv_usec / 1000.0));
			fflush(stdout);
		}
		memset(buf, 0, sizeof(buf));
		sprintf(buf, "%lu", master_seq);
		printf("master_seq: %lu\n", master_seq);

		gettimeofday(&socket_t1, NULL);
		printf("sending the latest seqno %lu to master\n", master_seq);
		fflush(stdout);
		write(sockfd, buf, sizeof(buf));
		n = read(sockfd, buf, sizeof(buf));
		while (n < sizeof(buf))
		{
			n += read(sockfd, buf + n, sizeof(buf) - n);
		}
		gettimeofday(&socket_t2, NULL);
		socket_time += (socket_t2.tv_sec - socket_t1.tv_sec) * 1000.0 + (socket_t2.tv_usec - socket_t1.tv_usec) / 1000.0;

		num_events = strtoul(buf, (char **)NULL, 10);
		if (num_events > 0)
		{
			printf("receiving %lu events\n", num_events);
			fflush(stdout);
		}
		else
		{
			gettimeofday(&end, NULL);
			printf("received the last event at %lu ms\n", (unsigned long)(end.tv_sec * 1000.0 + end.tv_usec / 1000.0));
			printf("Number of events: %lu\n", master_seq);
			printf("Number of appends: %lu\n", ev_array[Name_log->head].seq_no);
			printf("Time on replay: %lu ms\n", (unsigned long)((end.tv_sec - begin.tv_sec) * 1000.0 + (end.tv_usec - begin.tv_usec) / 1000.0));
			printf("Time on read from master: %lu ms\n", (unsigned long)read_time);
			printf("Time on socket IO: %lu ms\n", (unsigned long)socket_time);
			printf("Done\n");
			fflush(stdout);
			// sending terminate signal to master
			memcpy(buf, "-1", sizeof(buf));
			write(sockfd, buf, sizeof(buf));
			return 1;
		}
		
		for (i = 0; i < num_events; i++)
		{
			gettimeofday(&socket_t1, NULL);
			n = read(sockfd, buf, sizeof(buf));
			while (n < sizeof(buf))
			{
				n += read(sockfd, buf + n, sizeof(buf) - n);
			}
			gettimeofday(&socket_t2, NULL);
			socket_time += (socket_t2.tv_sec - socket_t1.tv_sec) * 1000.0 + (socket_t2.tv_usec - socket_t1.tv_usec) / 1000.0;
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
			// fflush(stdout);

			if (ev.woofc_name[0] != 0 && stat(ev.woofc_name, &sbuf) < 0)
			{
				// get meta
				sprintf(woof_name, "%s/%s", master_namespace, ev.woofc_name);
				element_size = WooFMsgGetElSize(woof_name);
				// history_size = WooFMsgGetHistorySize(ev.woofc_name);
				history_size = 20000;
				if (element_size == 16) // BST_LL_AP
				{
					history_size = 10000;
				}
				else if (element_size == 32) // BST_LL_LINK
				{
					history_size = 20000;
				}
				else if (element_size == 520) // BST_DATA
				{
					history_size = 20000;
				}
				else if (element_size == 528) // LL_DATA
				{
					history_size = 10000;
				}

				err = WooFCreate(ev.woofc_name, element_size, history_size);
				if (err < 0)
				{
					fprintf(stderr, "can't create woof %s\n", ev.woofc_name);
					exit(1);
				}
			}

			if (ev.type == APPEND)
			{
				gettimeofday(&read_t1, NULL);

				// read element size from socket
				n = read(sockfd, buf, sizeof(buf));
				while (n < sizeof(buf))
				{
					n += read(sockfd, buf + n, sizeof(buf) - n);
				}
				element_size = strtoul(buf, (char **)NULL, 10);
				// printf("receiving element_size: %lu\n", element_size);
				// fflush(stdout);
				payload = malloc(element_size);
				if (payload == NULL)
				{
					fprintf(stderr, "failed to allocate memory for payload\n");
					fflush(stderr);
					exit(1);
				}
				// read payload from socket
				n = read(sockfd, payload, sizeof(payload));
				while (n < sizeof(payload))
				{
					n += read(sockfd, payload + n, sizeof(payload) - n);
				}

				gettimeofday(&read_t2, NULL);
				read_time += (read_t2.tv_sec - read_t1.tv_sec) * 1000.0 + (read_t2.tv_usec - read_t1.tv_usec) / 1000.0;
				seq_no = WooFPut(ev.woofc_name, NULL, &payload);
				if (WooFInvalid(seq_no))
				{
					fprintf(stderr, "failed to replay event to %s\n", ev.woofc_name);
					fflush(stderr);
					return 1;
				}
			}
			else
			{
				fprintf(stderr, "received event other than append\n");
				fflush(stderr);
				return 1;
			}
			master_seq = ev.seq_no;
		}
		printf("replayed %lu events in total\n", ev_array[Name_log->head].seq_no);
		fflush(stdout);
		sleep(1);
	}

	fflush(stdout);
	return (0);
}