#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include "woofc-access.h"
#include "cmq-pkt.h"
#include "cmq-mqtt-xport.h"

#ifdef USE_CMQ_SD_CACHE
#include <sys/socket.h>
#include "cmq-cache.h"
#endif

#define PORT 8079
#define TIMEOUT 3000 // 3 second timeout

#define ARGS "h:p:C:S:s"
char *Usage = "cmq-perf [-c host_ip]\n\
\t[-s] <server mode>\n\
\t-p host_port\n\
\t-C frame_count\n\
\t-S frame_size\n";

int main(int argc, char **argv)
{
	int c;
	char host_ip[50];
	unsigned long host_port;
	int endpoint;
	int server_sd;
	int count;
	unsigned char *payload;
	int i;
	unsigned char *fl;
	unsigned char *f;
	int err;
	int size;
	int is_server = 0;
	struct timeval start_tv;
	struct timeval end_tv;
	unsigned char ack[1];

	host_port = 8079;
	memset(host_ip,0,sizeof(host_ip));
	count = 1;
	size = 8192;


	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'C':
				count = atoi(optarg);
				break;
			case 'c':
				strncpy(host_ip,optarg,sizeof(host_ip));
				break;
			case 'p':
				host_port = atoi(optarg);
				break;
			case 's':
				is_server = 1;
				break;
			case 'S':
				size = atoi(optarg);
				break;
			default:
				fprintf(stderr,"unrecognized argument %c\n",
					(char)c);
				fprintf(stderr,"%s",Usage);
				exit(1);
		}
	}
	if((is_server == 0) && (host_ip[0] == 0)) {
		fprintf(stderr,"must specify server ip address in client mode\n");
		fprintf(stderr,"%s",Usage);
		exit(1);
	}

	if(count <= 0) {
		fprintf(stderr,"count must be >= 1\n");
		fprintf(stderr,"%s",Usage);
		exit(1);
	}

	if(is_server == 1) {
		printf("cmq-perf server listening on port %d\n",host_port);
	} else {
		printf("cmq-perf client connecting to server on host %s at port %d\n",
			       host_ip,host_port);
	}	
	printf("\tsending %d messages of size %d\n",count,size);

		
	if(is_server == 0) {
		payload = malloc(size);
		if(payload == NULL) {
			exit(1);
		}
		endpoint = cmq_pkt_connect(host_ip,host_port,TIMEOUT);
		if(endpoint < 0) {
			fprintf(stderr,"ERROR: failed to create endpoint\n");
			exit(1);
		}

		err = cmq_frame_list_create(&fl);
		if(err < 0) {
			fprintf(stderr,"ERROR: failed to create frame list\n");
			exit(1);
		}

		// create a frame list
		for(i=0; i < count; i++) {
			memset(payload,0,sizeof(payload));
			sprintf(payload,"frame-%d",i);
			printf("adding %s to frame list\n",(char *)payload);
			err = cmq_frame_create(&f,(unsigned char *)payload,strlen(payload));
			if(err < 0) {
				fprintf(stderr,"ERROR: failed to create frame %d\n",i);
				exit(1);
			}
			err = cmq_frame_append(fl,f);
			if(err < 0) {
				fprintf(stderr,"ERROR: failed to append frame %d\n",i);
				exit(1);
			}
		}

		// send frame list to server
		printf("sending frame list to server %s:%lu\n",host_ip,host_port);
		err = cmq_pkt_send_msg(endpoint,fl);
		if(err < 0) {
			fprintf(stderr,"ERROR: failed to send msg\n");
			exit(1);
		}

		// destroy the frame list
		cmq_frame_list_destroy(fl);

		// receive an ACK
		printf("receiving frame list from server %s:%lu\n",host_ip,host_port);
		err = cmq_pkt_recv_msg(endpoint,&fl);
		if(err < 0) {
			fprintf(stderr,"ERROR: failed to recv msg\n");
			exit(1);
		}
		err = frame_list_pop(fl,&f);
		if(err < 0) {
			fprintf(stderr,"ERROR: failed to recv ack frame\n");
			exit(1);
		}
		cmq_frame_destroy(f);
		cmq_frame_list_destroy(fl);
		exit(0);
	} else { // i am the server
		server_sd = cmq_pkt_listen(host_port);
		if(server_sd < 0) {
			fprintf(stderr,"ERROR: failed to create server_sd\n");
			perror("listen");
			exit(1);
		}

		while(1) {
			endpoint = cmq_pkt_accept(server_sd, 0); // zero timeout implies wait forever
			if(endpoint < 0) {
				fprintf(stderr,"ERROR: failed to accept endpoint\n");
				perror("listen");
				exit(1);
			}
			err = cmq_pkt_recv_msg(endpoint,&fl);
			if(err < 0) {
				fprintf(stderr,"ERROR: failed to recv msg\n");
				break;
			}

			
			// print out frame list without destroying it
			printf("receiving frame list from client\n");
			f = cmq_frame_list_head(fl);
			if(f == NULL) {
				fprintf(stderr,"ERROR: frame list head is NULL\n");
				break;
			}
			for(i=0; i < cmq_frame_list_count(fl); i++) {
				printf("recv: %s\n",(char *)cmq_frame_payload(f));
				f = cmq_frame_next(f);
				if(f == NULL) {
					break;
				}
			}

			if((f == NULL) && (i < (cmq_frame_list_count(fl)-1))) {
				fprintf(stderr,"ERROR: NULL frame at frame %d\n",i);
				break;
			}

			cmq_frame_list_destroy(fl);

			// send an ack
			err = cmq_frame_list_create(&fl);
			if(err < 0) {
				fprintf(stderr,"ERROR: no frame list for ack\n");
				break;
			}
			err = cmq_frame_create(&f,&ack[0],sizeof(ack));
			if(err < 0) {
				fprintf(stderr,"ERROR: no frame for ack\n");
				break;
			}

			printf("sending ack to client\n");
			err = cmq_pkt_send_msg(endpoint,fl);
			if(err < 0) {
				fprintf(stderr,"ERROR: failed to send msg\n");
				break;
			}

			// destroy the frame list
			cmq_frame_list_destroy(fl);
			cmq_pkt_close(endpoint);
		}
		exit(0);
	}
}
		


		
		
		

		
		

		

