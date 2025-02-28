#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include "cmq-pkt.h"
	
int cmq_pkt_connect(char *addr, unsigned short port, unsigned long timeout)
{
	int sd;
	struct sockaddr_in ep_in;
	int err;
	struct timeval tv; // timeout is in ms
#ifdef __APPLE__
	int opt = 1;
#endif

	sd = socket(AF_INET, SOCK_STREAM, 0);
	if(sd < 0) {
		return(-1);
	}
#ifdef __APPLE__
	if(setsockopt(sd, SOL_SOCKET, SO_NOSIGPIPE, &opt, sizeof(opt))) {
		return(-1);
	}
#else
	signal(SIGPIPE,SIG_IGN);
#endif
	tv.tv_sec = timeout / 1000;
	tv.tv_usec = 0;
	if(setsockopt(sd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv))) {
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
	struct sockaddr_in local_address;
	int opt = 1;
	int err;

	sd = socket(AF_INET, SOCK_STREAM, 0);
	if(sd == -1) {
		return(-1);
	}

	if(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
		return(-1);
	}
	if(setsockopt(sd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt))) {
		return(-1);
	}
#ifdef __APPLE__
	if(setsockopt(sd, SOL_SOCKET, SO_NOSIGPIPE, &opt, sizeof(opt))) {
		return(-1);
	}
#else
	signal(SIGPIPE,SIG_IGN);
#endif

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

	return(sd);
}


int cmq_pkt_accept(int sd, unsigned long timeout)
{
	int c_fd;
	struct sockaddr_in local_address;
	socklen_t len = sizeof(local_address);
	struct timeval tv;


	// accept blocks forever but sets recv timeout for recv socket when
	// accept completes
	c_fd = accept(sd, (struct sockaddr *)&local_address, &len);
	if(c_fd < 0) {
		return(-1);
	}
	if(timeout > 0) {
		tv.tv_sec = timeout/1000;
		tv.tv_usec = 0;
		if(setsockopt(c_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv))) {
			return(-1);
		}
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
	header.max_size = htonl(frame_list->max_size);

	// send the header using write
	err = write(endpoint,&header,sizeof(header));
	if((unsigned long int)err < sizeof(header)) {
		return(-1);
	}

	// send the frames (if they are there)
	frame = frame_list->head;
	while(frame != NULL) {
		// write the frame size
		size = htonl(frame->size);
		err = write(endpoint,&size,sizeof(size));
		if((unsigned long int)err < sizeof(size)) {
			return(-1);
		}
		// write the frame data if the size > 0
		if(frame->size > 0) {
			err = write(endpoint,frame->payload,frame->size);
			if((unsigned long int)err < frame->size) {
				return(-1);
			}
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
	unsigned long max_size;

	// read the header
	err = recv(endpoint,&header,sizeof(header),MSG_WAITALL);
	if((unsigned long int)err < sizeof(header)) {
		return(-1);
	}

	header.version = ntohl(header.version);
	header.frame_count = ntohl(header.frame_count);
	max_size = ntohl(header.max_size);

	if(header.version != CMQ_PKT_VERSION) {
		return(-1);
	}

	// create an empty frame list
	err = cmq_frame_list_create(&l_fl);
	if(err < 0) {
		return(-1);
	}

	// use hint to prallocate a bufffer
	// hint could be zero if only zero frame is received
	if(max_size > 0) {
		payload = (unsigned char *)malloc(max_size);
		if(payload == NULL) {
			cmq_frame_list_destroy(l_fl);
			return(-1);
		}
	} else {
		payload = NULL;
	}


	// read the frames
	for(i=0; i < (int)header.frame_count; i++) {
		err = recv(endpoint,(unsigned char *)&size,sizeof(size),MSG_WAITALL);
		if((unsigned long int)err < sizeof(size)) {
			cmq_frame_list_destroy(l_fl);
			return(-1);
		}
		// this could happen if a frame of max_size was popped
		// after frame_list was created
		if(ntohl(size) > max_size) {
			if(payload != NULL) {
				free(payload);
			}
			max_size = ntohl(size);
			payload = (unsigned char *)malloc(max_size);
			if(payload == NULL) {
				cmq_frame_list_destroy(l_fl);
				return(-1);
			}
		}
		// read the payload if the size > 0
		if(ntohl(size) > 0) {
			err = recv(endpoint,payload,ntohl(size), MSG_WAITALL);
			if((unsigned int)err < ntohl(size)) {
				free(payload);
				cmq_frame_list_destroy(l_fl);
				return(-1);
			}
		}
		// cerate the frame
		if(ntohl(size) > 0) {
			err = cmq_frame_create(&f,payload,ntohl(size));
		} else {
			err = cmq_frame_create(&f,NULL,0); // zero frame 
		}
		if(err < 0) {
			if(payload != NULL) {
				free(payload);
			}
			cmq_frame_list_destroy(l_fl);
			return(-1);
		}
		// add frame to frame_list
		err = cmq_frame_append(l_fl,f);
		if(err < 0) {
			if(payload != NULL) {
				free(payload);
			}
			cmq_frame_list_destroy(l_fl);
			cmq_frame_destroy(f);
			return(-1);
		}
	}
	if(payload != NULL) {
		free(payload);
	}

	*fl = l_fl;

	return(0);
}
		

#ifdef TESTCLIENT

#define PORT 8079
#define TIMEOUT 3000 // 3 second timeout

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

	close(endpoint);
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

	close(endpoint);
	return(0);
}

#endif
	


	
	
	

	
	

	

