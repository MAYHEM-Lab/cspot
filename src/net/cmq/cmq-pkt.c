#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "cmq-pkt.h"


	
int cmq_pkt_endpoint(char *addr, unsigned short port)
{
	int sd;
	struct sockaddr_in ep_in;
	int err;

	sd = socket(AF_INET, SOCK_STREAM, 0);
	if(sd < 0) {
		return(-1);
	}

	ep_in.sin_family = AF_INET;
	ep_in.sin_port = htons(port);	

	err = inet_pton(AF_INET,addr,&ep_in.sin_addr);
	if(err <= 0) {
		close(sd);
		return(-1);
	}

	err = connect(sd,(struct sockaddr *)&ep_in,sizeof(ep_in));
	if(err < 0) {
		perror("ERROR: connect failed");
		close(sd);
		return(-1);
	}

	return(sd);
}

int cmq_pkt_listen(unsigned long port)
{
	int sd;
	int c_fd;
	struct sockaddr_in local_address;
	socklen_t len = sizeof(local_address);
	int opt = 1;
	int err;

	sd = socket(AF_INET, SOCK_STREAM, 0);
	if(sd == -1) {
		return(-1);
	}

	if(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
		return(-1);
	}

	local_address.sin_family = AF_INET;
	local_address.sin_addr.s_addr = INADDR_ANY;
	local_address.sin_port = htons(port);

	err = bind(sd,(struct sockaddr *)&local_address, sizeof(local_address));
	if(err < 0) {
		close(sd);
		return(-1);
	}

	err = listen(sd,3);
	if(err < 0) {
		close(sd);
		return(-1);
	}

	c_fd = accept(sd, (struct sockaddr *)&local_address, &len);
	if(c_fd < 0) {
		close(sd);
		return(-1);
	}

	return(c_fd);
}

int cmq_pkt_send_msg(int endpoint, unsigned char *fl)
{
	CMQFRAMELIST *frame_list = (CMQFRAMELIST *)fl;
	CMQFRAME *frame;
	CMQPKTHEADER header;
	int err;
	unsigned long size;

	header.version = htonl(CMQ_PKT_VERSION);
	header.frame_count = htonl(frame_list->count);

	// send the header using write
	err = write(endpoint,&header,sizeof(header));
	if(err < sizeof(header)) {
		return(-1);
	}

	// send the frames (if they are there)
	frame = frame_list->head;
	while(frame != NULL) {
		// write the frame size
		size = htonl(frame->size);
		err = write(endpoint,&size,sizeof(size));
		if(err < sizeof(size)) {
			return(-1);
		}
		// write the frame
		err = write(endpoint,frame->payload,frame->size);
		if(err < frame->size) {
			return(-1);
		}
		frame = frame->next;
	}

	return(0);
}

int cmq_pkt_recv_msg(int endpoint, unsigned char **fl)
{
	unsigned char *l_fl;
	CMQPKTHEADER header;
	unsigned char *f;
	int err;
	int i;
	unsigned long size;
	unsigned char *payload;

	// read the header
	err = read(endpoint,&header,sizeof(header));
	if(err < sizeof(header)) {
		return(-1);
	}

	header.version = ntohl(header.version);
	header.frame_count = ntohl(header.frame_count);

	if(header.version != CMQ_PKT_VERSION) {
		return(-1);
	}

	// create an empty frame list
	err = cmq_frame_list_create(&l_fl);
	if(err < 0) {
		return(-1);
	}

	// read the frames
	for(i=0; i < header.frame_count; i++) {
		// read the size
		err = read(endpoint,&size,sizeof(size));
		if(err < sizeof(size)) {
			return(-1);
		}
		payload = (unsigned char *)malloc(ntohl(size));
		if(payload == NULL) {
			return(-1);
		}
		// read the payload
		err = read(endpoint,payload,ntohl(size));
		if(err < ntohl(size)) {
			free(payload);
			return(-1);
		}
		// cerate the frame
		err = cmq_frame_create(&f,payload,ntohl(size));
		if(err < 0) {
			free(payload);
			return(-1);
		}
		// add frame to frame_list
		err = cmq_frame_append(l_fl,f);
		if(err < 0) {
			free(payload);
			return(-1);
		}
		free(payload);
	}

	*fl = l_fl;

	return(0);
}
		

#ifdef TESTCLIENT

#define PORT 8079

#define ARGS "h:p:c:"
char *Usage = "cmq-test-client -h host_ip\n\
\t-p host_port\n\
\t-c frame_count\n";

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

	host_port = 8079;
	memset(host_ip,0,sizeof(host_ip));
	count = 1;

	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'c':
				count = atoi(optarg);
				break;
			case 'h':
				strncpy(host_ip,optarg,sizeof(host_ip));
				break;
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
		
	endpoint = cmq_pkt_endpoint(host_ip,host_port);
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
	close(endpoint);
	return(0);
}

#endif

#ifdef TESTSERVER

#define PORT 8079

#define ARGS "p:"
char *Usage = "cmq-test-server -p host_port\n";

int main(int argc, char **argv)
{
	int c;
	unsigned long host_port;
	int endpoint;
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
		
	endpoint = cmq_pkt_listen(host_port);
	if(endpoint < 0) {
		fprintf(stderr,"ERROR: failed to create endpoint\n");
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

	close(endpoint);
	return(0);
}
#endif
	


	
	
	

	
	

	

