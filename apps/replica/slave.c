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
char *Usage = "slave -m master_namespace -p port\n";

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
	char woof_element[4096];
	unsigned long master_seq, slave_seq;
	unsigned long num_events;
	int port = 8080;
	int sockfd = 0, n = 0;
	char buf[2048];
	char master_ip[1024];
	struct sockaddr_in serv_addr;

	unsigned long last_seqno = 0;
	struct timeval begin, end;
	struct timeval t1, t2;
	double elapsedTime = 0;

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
	
	while (1)
	{
		printf("connecting to master\n");
		fflush(stdout);
		connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

		// send the latest event seqno to master
		slave_seq = ev_array[Name_log->head].seq_no;
		if (slave_seq == 0)
		{
			gettimeofday(&begin, NULL);
		}
		memset(buf, 0, sizeof(buf));
		sprintf(buf, "%lu", slave_seq);
		// printf("slave_seq: %lu\n", slave_seq);

		gettimeofday(&t1, NULL);
		printf("sending the latest seqno %lu to master\n", slave_seq);
		fflush(stdout);
		printf("sending buf: %s\n", buf);
		fflush(stdout);
		write(sockfd, buf, sizeof(buf));
		printf("sent buf: %s\n", buf);
		fflush(stdout);
		n = read(sockfd, buf, sizeof(buf));
		while (n < sizeof(buf))
		{
			n += read(sockfd, buf + n, sizeof(buf) - n);
		}
		gettimeofday(&t2, NULL);
		elapsedTime += (t2.tv_sec - t1.tv_sec) * 1000.0 + (t2.tv_usec - t1.tv_usec) / 1000.0;

		num_events = strtoul(buf, (char **)NULL, 10);
		if (num_events > 0)
		{
			printf("receiving %lu events\n", num_events);
			fflush(stdout);
		}
		for (i = 0; i < num_events; i++)
		{
			gettimeofday(&t1, NULL);
			n = read(sockfd, buf, sizeof(buf));
			while (n < sizeof(buf))
			{
				n += read(sockfd, buf + n, sizeof(buf) - n);
			}
			gettimeofday(&t2, NULL);
			elapsedTime += (t2.tv_sec - t1.tv_sec) * 1000.0 + (t2.tv_usec - t1.tv_usec) / 1000.0;
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
			if (ev.type == LATEST_SEQNO)
			{
				WooFGetLatestSeqno(ev.woofc_name);
				// printf("WooFGetLatestSeqno(ev.woofc_name);\n");
			}
			else if (ev.type == READ)
			{
				WooFGet(ev.woofc_name, &woof_element, ev.woofc_seq_no);
			}
			else if (ev.type == APPEND)
			{
				sprintf(woof_name, "%s/%s", master_namespace, ev.woofc_name);
				WooFGet(woof_name, &woof_element, ev.woofc_seq_no);
				seq_no = WooFPut(ev.woofc_name, NULL, &woof_element);
			}
			ev_array[Name_log->head].seq_no++;
		}
		if (ev_array[Name_log->head].seq_no == last_seqno)
		{
			gettimeofday(&end, NULL);
			printf("total: %f ms\n", (end.tv_sec - begin.tv_sec) * 1000.0 + (end.tv_usec - begin.tv_usec) / 1000.0);
			printf("communication: %f ms\n", elapsedTime);
			fflush(stdout);
			memset(buf, 0, sizeof(buf));
			sprintf(buf, "-1");
			write(sockfd, buf, sizeof(buf));
			break;
		}
		printf("seqno: %lu\n", ev_array[Name_log->head].seq_no);
		fflush(stdout);
		last_seqno = ev_array[Name_log->head].seq_no;
		usleep(100000);
	}

	fflush(stdout);
	return (0);
}