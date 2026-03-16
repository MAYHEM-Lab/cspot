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
	int count;
	char payload[1024];
	int i;
	unsigned char *fl;
	unsigned char *f;
	int err;
	int size;
	int is_server = 0;

	host_port = 8079;
	memset(host_ip,0,sizeof(host_ip));
	count = 1;


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
	if(host_ip[0] == 0) {
		fprintf(stderr,"must specify server ip address\n");
		fprintf(stderr,"%s",Usage);
		exit(1);
	}

	if(count <= 0) {
		fprintf(stderr,"count must be >= 1\n");
		fprintf(stderr,"%s",Usage);
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

	// receive an echo of the frame list
	printf("receiving frame list from server %s:%lu\n",host_ip,host_port);
	err = cmq_pkt_recv_msg(endpoint,&fl);
	if(err < 0) {
		fprintf(stderr,"ERROR: failed to recv msg\n");
		exit(1);
	}

	// print out the frame list echoed from the server
	printf("printing echoed frame list\n");
	while(!cmq_frame_list_empty(fl)) {
		err = cmq_frame_pop(fl,&f);
		if(err < 0) {
			fprintf(stderr,"ERROR: could not pop frame\n");
			exit(1);
		}
		memset(payload,0,sizeof(payload));
		memcpy(payload,cmq_frame_payload(f),cmq_frame_size(f));
		printf("echo: %s\n",(char *)payload);
		cmq_frame_destroy(f);
	}

	cmq_frame_list_destroy(fl);

	err = cmq_frame_list_create(&fl);
	if(err < 0) {
		fprintf(stderr,"ERROR: failed create frame list for zero send\n");
		exit(1);
	}

	printf("client send zero frame\n");
	// now test zero frame send
	err = cmq_frame_create(&f,NULL,0);
	if(err < 0) {
		fprintf(stderr,"ERROR: failed to create zero frame\n");
		exit(1);
	}

	err = cmq_frame_append(fl,f);
	if(err < 0) {
		fprintf(stderr,"ERROR: failed to append zero frame for send\n");
		exit(1);
	}

	err = cmq_pkt_send_msg(endpoint,fl);
	if(err < 0) {
		fprintf(stderr,"ERROR: failed to send zero frame\n");
		exit(1);
	}

	cmq_frame_list_destroy(fl);
	// recv a zero frame back
	err = cmq_pkt_recv_msg(endpoint,&fl);
	if(err < 0) {
		fprintf(stderr,"ERROR: failed to recv zero frame\n");
		exit(1);
	}

	err = cmq_frame_pop(fl,&f);
	if(err < 0) {
		fprintf(stderr,"ERROR: failed to pop zero frame\n");
		exit(1);
	}

	cmq_frame_list_destroy(fl);

	if(cmq_frame_size(f) != 0) {
		fprintf(stderr,"ERROR: zero frame has size %d\n",
				cmq_frame_size(f));
		exit(1);
	}
	if(cmq_frame_payload(f) != NULL) {
		fprintf(stderr,"ERROR: zero frame has non NULL payload\n");
		exit(1);
	}

	cmq_frame_destroy(f);
	printf("client recv zero frame echo\n");

	cmq_pkt_close(endpoint);
	return(0);
}

#endif

#ifdef TESTSERVER

#define PORT 8079
#define TIMEOUT 3000 // 3 second accept timeout

#define ARGS "p:"
char *Usage = "cmq-test-server -p host_port\n";

int main(int argc, char **argv)
{
	int c;
	unsigned long host_port;
	int endpoint;
	int server_sd;
	int count;
	char payload[1024];
	int i;
	unsigned char *fl;
	unsigned char *f;
	int err;

	host_port = 8079;

	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'p':
				host_port = atoi(optarg);
				break;
			default:
				fprintf(stderr,"unrecognized argument %c\n",
					(char)c);
				fprintf(stderr,"%s",Usage);
				exit(1);
		}
	}
		
	server_sd = cmq_pkt_listen(host_port);
	if(server_sd < 0) {
		fprintf(stderr,"ERROR: failed to create server_sd\n");
		perror("listen");
		exit(1);
	}

	endpoint = cmq_pkt_accept(server_sd, 0); // zero timeout implies wait forever
	if(endpoint < 0) {
		fprintf(stderr,"ERROR: failed to accept endpoint\n");
		perror("listen");
		exit(1);
	}


	err = cmq_pkt_recv_msg(endpoint,&fl);
	if(err < 0) {
		fprintf(stderr,"ERROR: failed to recv msg\n");
		exit(1);
	}

	
	// print out frame list without destroying it
	printf("receiving frame list from client\n");
	f = cmq_frame_list_head(fl);
	if(f == NULL) {
		fprintf(stderr,"ERROR: frame list head is NULL\n");
		exit(1);
	}
	for(i=0; i < cmq_frame_list_count(fl); i++) {
		memset(payload,0,sizeof(payload));
		memcpy(payload,cmq_frame_payload(f),cmq_frame_size(f));
		printf("recv: %s\n",(char *)payload);
		f = cmq_frame_next(f);
		if(f == NULL) {
			break;
		}
	}

	if((f == NULL) && (i < (cmq_frame_list_count(fl)-1))) {
		fprintf(stderr,"ERROR: NULL frame at frame %d\n",i);
		exit(1);
	}
			

	// send frame list to server
	printf("sending frame list to client\n");
	err = cmq_pkt_send_msg(endpoint,fl);
	if(err < 0) {
		fprintf(stderr,"ERROR: failed to send msg\n");
		exit(1);
	}

	// destroy the frame list
	cmq_frame_list_destroy(fl);

	// test zero frame recv and echo
	err = cmq_pkt_recv_msg(endpoint,&fl);
	if(err < 0) {
		fprintf(stderr,"ERROR: failed to recv zero frame\n");
		exit(1);
	}

	err = cmq_frame_pop(fl,&f);
	if(err < 0) {
		fprintf(stderr,"ERROR: failed to pop zero frame\n");
		exit(1);
	}
	cmq_frame_list_destroy(fl);

	if(cmq_frame_size(f) != 0) {
		fprintf(stderr,"ERROR: zero frame has size %d\n",
				cmq_frame_size(f));
		exit(1);
	}

	if(cmq_frame_payload(f) != NULL) {
		fprintf(stderr,"ERROR: zero frame has non NULL payload\n");
		exit(1);
	}
	cmq_frame_list_destroy(f);

	printf("server recv zero frame\n");
	// create zero frame response
	
	err = cmq_frame_list_create(&fl);
	if(err < 0) {
		fprintf(stderr,"ERROR: failed to create zero frame response list\n");
		exit(1);
	}

	err = cmq_frame_create(&f,NULL,0);
	if(err < 0) {
		fprintf(stderr,"ERROR: failed to create zero frame response\n");
		exit(1);
	}

	err = cmq_frame_append(fl,f);
	if(err < 0) {
		fprintf(stderr,"ERROR: failed to append zero frame response\n");
		exit(1);
	}
	err = cmq_pkt_send_msg(endpoint,fl);
	if(err < 0) {
		fprintf(stderr,"ERROR: failed to send zero frame response\n");
		exit(1);
	}

	cmq_frame_list_destroy(fl);
	printf("server sent zero frame echo\n");

	cmq_pkt_close(endpoint);
	return(0);
}

#endif
	


	
	
	

	
	

	

