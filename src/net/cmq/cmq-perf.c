#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>

#include "woofc-access.h"
#include "cmq-pkt.h"
#include "cmq-mqtt-xport.h"

#ifdef USE_CMQ_SD_CACHE
#include <sys/socket.h>
#include "cmq-cache.h"
#endif

#define PORT 8079
#define TIMEOUT 3000 // 3 second timeout

#define IS_CLIENT (1)
#define IS_SERVER (2)

int Verbose;

#define ARGS "c:h:p:C:S:sVR"
char *Usage = "cmq-perf [-c host_ip]\n\
\t[-s] <server mode>\n\
\t-p host_port\n\
\t-C frame_count\n\
\t-R <run client remotely>\n\
\t-S frame_size\n\
\t-V verbose mode\n";

extern int CMQ_use_mqtt;

double Duration(struct timeval *end, struct timeval *start)
{
	double d;
	double d1;
	double d2;

	d1 = ((double)(end->tv_sec) + (double)(end->tv_usec)/1000000.0);
	d2 = ((double)(start->tv_sec) + (double)(start->tv_usec)/1000000.0);
	d = d1 - d2;
	return(d);
}

int SendFlags(int endpoint, int flags, int count, int size)
{
	unsigned char *fl;
	unsigned char *f;
	int err;
	unsigned long flags_array[3];

	flags_array[0] = htonl(flags);
	flags_array[1] = htonl(count);
	flags_array[2] = htonl(size);

	err = cmq_frame_list_create(&fl);
	if(err < 0) {
		fprintf(stderr,"ERROR: failed to create frame list for flags\n");
		return(-1);
	}
	err = cmq_frame_create(&f,(unsigned char *)&flags_array[0],sizeof(flags_array));
	if(err < 0) {
		fprintf(stderr,"ERROR: failed to create frame for flags\n");
		return(-1);
	}
	err = cmq_frame_append(fl,f);
	if(err < 0) {
		fprintf(stderr,"ERROR: failed to append frame for flags\n");
		return(-1);
	}
	err = cmq_pkt_send_msg(endpoint,fl);
	if(err < 0) {
		cmq_frame_list_destroy(fl);
		fprintf(stderr,"ERROR: failed to send flags msg\n");
		return(-1);
	}

	// destroy the frame list
	cmq_frame_list_destroy(fl);
	return(1);

}

int RecvFlags(int endpoint, int *flags, int *count, int *size)
{
	unsigned long flags_array[3];
	unsigned char *fl;
	unsigned char *f;
	unsigned char *p;
	int err;

	err = cmq_pkt_recv_msg(endpoint,&fl);
	if(err < 0) {
		fprintf(stderr,"ERROR: failed to recv flags\n");
		return(-1);
	}

	
	f = cmq_frame_list_head(fl);
	if(f == NULL) {
		fprintf(stderr,"ERROR: frame list head is NULL for flags\n");
		return(-1);
	}
	if(cmq_frame_size(f) < sizeof(flags_array)) {
		fprintf(stderr,
			"ERROR: flags too small: %d\n",cmq_frame_size(f));
		return(-1);
	}
	memcpy(flags_array,cmq_frame_payload(f),sizeof(flags_array));
	flags_array[0] = ntohl(flags_array[0]);
	flags_array[1] = ntohl(flags_array[1]);
	flags_array[2] = ntohl(flags_array[2]);
	cmq_frame_list_destroy(fl);

	p = (unsigned char *)&(flags_array[0]);
	*flags = *((int *)p);
	p = (unsigned char *)&(flags_array[1]);
	*count = *((int *)p);
	p = (unsigned char *)&(flags_array[2]);
	*size = *((int *)p);
	return(1);
}


int DoClient(int endpoint, int count, int size)
{
	unsigned char *fl;
	unsigned char *f;
	int i;
	unsigned char *payload;
	int err;
	double total;
	double duration;
	struct timeval start_tv;
	struct timeval end_tv;

	payload = malloc(size);
	if(payload == NULL) {
		fprintf(stderr,"ERROR: failed to create payload buffer: %d\n",size);
		exit(1);
	}

	err = cmq_frame_list_create(&fl);
	if(err < 0) {
		fprintf(stderr,"ERROR: failed to create frame list\n");
		free(payload);
		return(-1);
	}

	// create a frame list
	total = (double)count * (double)size;
	gettimeofday(&start_tv,NULL);
	for(i=0; i < count; i++) {
		//memset(payload,0,sizeof(payload));
		sprintf(payload,"frame-%d",i);
		if(Verbose == 1) {
			printf("adding %s to frame list\n",(char *)payload);
		}
		err = cmq_frame_create(&f,(unsigned char *)payload,strlen(payload));
		if(err < 0) {
			fprintf(stderr,"ERROR: failed to create frame %d\n",i);
			free(payload);
			return(-1);
		}
		err = cmq_frame_append(fl,f);
		if(err < 0) {
			fprintf(stderr,"ERROR: failed to append frame %d\n",i);
			free(payload);
			return(-1);
		}
	}

	free(payload);

	err = cmq_pkt_send_msg(endpoint,fl);
	if(err < 0) {
		cmq_frame_list_destroy(fl);
		fprintf(stderr,"ERROR: failed to send msg\n");
		return(-1);
	}

	// destroy the frame list
	cmq_frame_list_destroy(fl);

	// receive an ACK
	if(Verbose == 1) {
		printf("receiving ack from server\n");
	}
	err = cmq_pkt_recv_msg(endpoint,&fl);
	if(err < 0) {
		fprintf(stderr,"ERROR: failed to recv msg\n");
		return(-1);
	}
	err = cmq_frame_pop(fl,&f);
	if(err < 0) {
		fprintf(stderr,"ERROR: failed to recv ack frame\n");
		return(-1);
	}
	gettimeofday(&end_tv,NULL);
	duration = Duration(&end_tv,&start_tv);
	printf("cmq-pkt client sent %f megabytes / sec\n",(total/(1024*1024)) / duration);
	cmq_frame_destroy(f);
	cmq_frame_list_destroy(fl);
	cmq_pkt_close(endpoint);
	return(1);
}

int DoServer(int endpoint)
{
	unsigned char *fl;
	unsigned char *f;
	int i;
	unsigned char ack[1];
	int err;


	err = cmq_pkt_recv_msg(endpoint,&fl);
	if(err < 0) {
		fprintf(stderr,"ERROR: failed to recv msg\n");
		return(-1);
	}

	
	// print out frame list without destroying it
	if(Verbose == 1) {
		printf("receiving frame list from client\n");
	}
	f = cmq_frame_list_head(fl);
	if(f == NULL) {
		fprintf(stderr,"ERROR: frame list head is NULL\n");
		return(-1);
	}
	for(i=0; i < cmq_frame_list_count(fl); i++) {
		if(Verbose == 1) {
			printf("recv: %s\n",(char *)cmq_frame_payload(f));
		}
		f = cmq_frame_next(f);
		if(f == NULL) {
			break;
		}
	}

	if((f == NULL) && (i < (cmq_frame_list_count(fl)-1))) {
		fprintf(stderr,"ERROR: NULL frame at frame %d\n",i);
		return(-1);
	}

	cmq_frame_list_destroy(fl);

	// send an ack
	err = cmq_frame_list_create(&fl);
	if(err < 0) {
		fprintf(stderr,"ERROR: no frame list for ack\n");
		return(-1);
	}
	err = cmq_frame_create(&f,&ack[0],sizeof(ack));
	if(err < 0) {
		fprintf(stderr,"ERROR: no frame for ack\n");
		return(-1);
	}

	err = cmq_frame_append(fl,f);
	if(err < 0) {
		fprintf(stderr,"ERROR: could not append ack to fl\n");
		return(-1);
	}

	if(Verbose == 1) {
		printf("sending ack to client\n");
	}
	err = cmq_pkt_send_msg(endpoint,fl);
	if(err < 0) {
		fprintf(stderr,"ERROR: failed to send msg\n");
		return(-1);
	}

	// destroy the frame list
	cmq_frame_list_destroy(fl);
	cmq_pkt_close(endpoint);
	return(1);
}

int main(int argc, char **argv)
{
	int c;
	char host_ip[50];
	int flags;
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
	int reverse;
	struct timeval start_tv;
	struct timeval end_tv;
	unsigned char ack[1];
	double total;
	double duration;

	// sockets for now
	CMQ_use_mqtt = 0;
	host_port = 8079;
	memset(host_ip,0,sizeof(host_ip));
	count = 1;
	size = 8192;
	Verbose = 0;
	reverse = 0;


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
			case 'R':
				reverse = 1;
				break;
			case 'S':
				size = atoi(optarg);
				break;
			case 'V':
				Verbose = 1;
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
		printf("cmq-perf server listening on port %lu\n",host_port);
	} else {
		printf("cmq-perf client connecting to server on host %s at port %lu\n",
			       host_ip,host_port);
		printf("\tsending %d messages of size %d\n",count,size);
	}	

		
	if(is_server == 0) {
		endpoint = cmq_pkt_connect(host_ip,host_port,TIMEOUT);
		if(endpoint < 0) {
			fprintf(stderr,"ERROR: failed to create endpoint to %s:%lu\n",
					host_ip,host_port);
			exit(1);
		}
		if(reverse == 0) { // client determines direction
			flags = IS_CLIENT; // either client or server for now
		} else {
			flags = IS_SERVER;
		}

		err = SendFlags(endpoint,flags,count,size);
		if(err < 0) {
			fprintf(stderr,"ERROR: failed to send flags to %s:%lu\n",
					host_ip,host_port);
			exit(1);
		}

		if(Verbose == 1) {
			if(flags == IS_CLIENT) {
				printf("sending %d packets, size %d (total: %lu bytes) to %s:%lu\n",
					count,size,(unsigned long)(count*size),
					host_ip,host_port);
			} else {
				printf("receiving %d packets, size %d (total: %lu bytes) to %s:%lu\n",
					count,size,(unsigned long)(count*size),
					host_ip,host_port);
			}
		}
		if(flags == IS_CLIENT) {
			err = DoClient(endpoint,count,size);
		} else {
			err = DoServer(endpoint);
		}

		if(err < 0) {
			if(flags == IS_CLIENT) {
				fprintf(stderr,"ERROR: client side failed to %s:%lu\n",
					host_ip,host_port);
			} else {
				fprintf(stderr,"ERROR: reverse client side failed to %s:%lu\n",
					host_ip,host_port);
			}
			exit(1);
		}
		exit(0);
	} else { // i am the server
		server_sd = cmq_pkt_listen(host_port);
		if(server_sd < 0) {
			fprintf(stderr,"ERROR: failed to create server_sd\n");
			perror("listen");
			exit(1);
		}

		while(1) {
			if(Verbose == 1) {
				printf("listening for connection\n");
			}
			endpoint = cmq_pkt_accept(server_sd, 0); // zero timeout implies wait forever
			if(Verbose == 1) {
				printf("accpet has completed\n");
			}
			if(endpoint < 0) {
				fprintf(stderr,"ERROR: accept failed\n");
				fflush(stderr);
				continue;
			}
			err = RecvFlags(endpoint,&flags,&count,&size);
			if(err < 0) {
				fprintf(stderr,"ERROR: receiving flags from client\n");
				fflush(stderr);
				continue;
			}
			if(flags == IS_CLIENT) { // client determine direction
				err = DoServer(endpoint);
				if(Verbose == 1) {
					printf("server has completed\n");
				}
			} else {
				err = DoClient(endpoint,count,size);
				if(Verbose == 1) {
					printf("reverse server has completed, count: %d, size: %d\n",
							count,size);
				}
			}
			if(err < 0) {
				fprintf(stderr,"ERROR: server failed from %s:%lu\n",
						host_ip,host_port);
			}
		}
		exit(0);
	}
}
		


		
		
		

		
		

		

