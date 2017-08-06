#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include <czmq.h>
#include <sys/types.h>
#include <ifaddrs.h>

#include "uriparser2.h"

#include "woofc.h"	/* for WooFPut */

extern char Host_ip[25];

#define DEBUG

/*
 * from https://en.wikipedia.org/wiki/Universal_hashing
 */
unsigned int WooFPortHash(char *namespace)
{
        unsigned long h = 5381;
        unsigned long a = 33;
        unsigned long i;

        for(i=0; i < strlen(namespace); i++) {
                h = ((h*a) + namespace[i]); /* no mod p due to wrap */
        }

	/*
	 * hash namespace to port number between 50000 and 60000
	 */
        return(50000 + (h%10000));
}


int WooFValidURI(char *str) 
{
	char *prefix;
	/*
	 * must begin with woof://
	 */
	prefix = strstr(str,"woof://");
	if(prefix == str) {
		return(1);
	} else {
		return(0);
	}
	
}

/*
 * extract namespace from full woof_name
 */
int WooFNameSpaceFromURI(char *woof_uri_str, char *woof_namespace, int len)
{
	struct URI uri;
	int i;

	if(!WooFValidURI(woof_uri_str)) { /* still might be local name, but return error */
		return(-1);
	}

	uri_parse2(woof_uri_str,&uri);
	if(uri.path == NULL) {
		return(-1);
	}
	/*
	 * walk back to the last '/' character
	 */
	i = strlen(uri.path);
	while(i >= 0) {
		if(uri.path[i] == '/') {
			if(i <= 0) {
				return(-1);
			}
			if(i > len) { /* not enough space to hold path */
				return(-1);
			}
			strncpy(woof_namespace,uri.path,i);
			return(1);
		}
		i--;
	}
	/*
	 * we didn't find a '/' in the URI path for the woofname -- error out
	 */
	return(-1);
}

int WooFNameFromURI(char *woof_uri_str, char *woof_name, int len)
{
	struct URI uri;
	int i;
	int j;

	if(!WooFValidURI(woof_uri_str)) { 
		return(-1);
	}

	uri_parse2(woof_uri_str,&uri);
	if(uri.path == NULL) {
		return(-1);
	}
	/*
	 * walk back to the last '/' character
	 */
	i = strlen(uri.path);
	j = 0;
	/*
	 * if last character in the path is a '/' this is an error
	 */
	if(uri.path[i] == '/') {
		return(-1);
	}
	while(i >= 0) {
		if(uri.path[i] == '/') {
			i++;
			if(i <= 0) {
				return(-1);
			}
			if(j > len) { /* not enough space to hold path */
				return(-1);
			}
			/*
			 * pull off the end as the name of the woof
			 */
			strncpy(woof_name,&(uri.path[i]),len);
			return(1);
		}
		i--;
		j++;
	}
	/*
	 * we didn't find a '/' in the URI path for the woofname -- error out
	 */
	return(-1);
} 

int WooFLocalIP(char *ip_str, int len)
{
	struct ifaddrs *addrs;
	struct ifaddrs *tmp;
	/*
	 * need the non loop back IP address for local machine to allow inter-container
	 * communication (e.g. localhost won't work)
	 */
	getifaddrs(&addrs);
	tmp = addrs;

	while (tmp) 
	{
	    if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET)
	    {
		if(strcmp(tmp->ifa_name,"lo") != 0) {
			struct sockaddr_in *pAddr = (struct sockaddr_in *)tmp->ifa_addr;
			strncpy(ip_str,inet_ntoa(pAddr->sin_addr),len);
			freeifaddrs(addrs);
			return(1);
		}
	    }
	    tmp = tmp->ifa_next;
	}
	fprintf(stderr,"WooFLocalIP: no local IP address found\n");
	fflush(stderr);
	freeifaddrs(addrs);
	exit(1);
}


int WooFIPAddrFromURI(char *woof_uri_str, char *ip_str, int len)
{
	struct URI uri;
	int i;
	int j;

	if(!WooFValidURI(woof_uri_str)) { 
		return(-1);
	}

	/*
	 * for now
	 */
	strncpy(ip_str,Host_ip,len);
	return(1);


#if 0
	uri_parse2(woof_uri_str,&uri);

	/*
	 * no host => localhost
	 */
	if(uri.host == NULL) {
		strncpy(ip_str,"127.0.0.1",len);
		return(1);
	}

	/*
	 * in the future, parse URI for host and return IP address. 
	 *
	 * for now, all woofs are local
	 */
	strncpy(ip_str,"127.0.0.1",len);
	return(1);
#endif
}


void *WooFMsgThread(void *arg)
{
	zsock_t *receiver;
	zmsg_t *msg;
	zmsg_t *r_msg;
	zframe_t *frame;
	zframe_t *r_frame;
	char *str;
	char namespace[2048];
	char hand_name[2048];
	unsigned int copy_size;
	void *element;
	int count;
	unsigned long seq_no;
	char buffer[255];
	int err;

	/*
	 * right now, we use REQ-REP pattern from ZeroMQ.  need a way to timeout, however, as this
	 * pattern blocks indefinitely on network partition
	 */

	/*
	 * create a reply zsock and connect it to the back end of the proxy in the msg server
	 */
	receiver = zsock_new_rep(">inproc://workers");
	if(receiver == NULL) {
		perror("WooFMsgThread: couldn't open receiver");
		pthread_exit(NULL);
	} 


	seq_no = 0;

#ifdef DEBUG
	printf("WooFMsgThread: about to call receive\n");
	fflush(stdout);
#endif
	msg = zmsg_recv(receiver);
	while(msg != NULL)
	{
#ifdef DEBUG
	printf("WooFMsgThread: received\n");
	fflush(stdout);
#endif
		/*
		 * WooFPut requires a namespace, handler_name, and the data
		 */
		frame = zmsg_first(msg);
		if(frame == NULL) {
			perror("WooFMsgThread: couldn't set cursor in msg");
			exit(1);
		}
		/*
		 * namespace in the first frame
		 */
		memset(namespace,0,sizeof(namespace));
		str = (char *)zframe_data(frame);
		copy_size = zframe_size(frame);
		if(copy_size > (sizeof(namespace)-1)) {
			copy_size = sizeof(namespace)-1;
		}
		strncpy(namespace,str,copy_size);

#ifdef DEBUG
	printf("WooFMsgThread: received namespace %s\n",namespace);
	fflush(stdout);
#endif
		/*
		 * handler name in the second frame
		 */
		memset(hand_name,0,sizeof(hand_name));
		frame = zmsg_next(msg);
		str = (char *)zframe_data(frame);
		copy_size = zframe_size(frame);
		if(copy_size > (sizeof(hand_name)-1)) {
			copy_size = sizeof(hand_name)-1;
		}
		strncpy(hand_name,str,copy_size);

#ifdef DEBUG
	printf("WooFMsgThread: received handler name %s\n",hand_name);
	fflush(stdout);
#endif
		/*
		 * raw data in the third frame
		 *
		 * need a maximum size but for now see if we can malloc() the space
		 */
		frame = zmsg_next(msg);
		copy_size = zframe_size(frame);
		element = malloc(copy_size);
		if(element == NULL) { /* element too big */
			fprintf(stderr,"WooFMsgThread: namespace: %s, handler: %s, size %lu too big\n",
				namespace,hand_name,copy_size);
			fflush(stderr);
			seq_no = -1;
		} else {
			str = (char *)zframe_data(frame);
			memcpy(element,(void *)str,copy_size);
#ifdef DEBUG
	printf("WooFMsgThread: received %lu bytes for the element\n", copy_size);
	fflush(stdout);
#endif
			/*
			 * czmq docs say that this will free the internal frames within the
			 * message
			 */
//			zmsg_destroy(&msg);

			/*
			 * attempt to put the element into the local namespace
			 *
			 * note that we could sanity check the namespace against the local namespace
			 * but WooFPut does this also
			 */
			seq_no = WooFPut(namespace,hand_name,element);

			free(element);
		}

		/*
		 * send seq_no back
		 */
		r_msg = zmsg_new();
		if(r_msg == NULL) {
			perror("WooFMsgThread: no reply message");
			exit(1);
		}
		memset(buffer,0,sizeof(buffer));
		sprintf(buffer,"%lu",seq_no);
		r_frame = zframe_new(buffer,strlen(buffer));
		if(r_frame == NULL) {
			perror("WooFMsgThread: no reply frame");
			exit(1);
		}
		err = zmsg_append(r_msg,&r_frame);
		if(err != 0) {
			perror("WooFMsgThread: couldn't append to r_msg");
			exit(1);
		}
		err = zmsg_send(&r_msg,receiver);
		if(err != 0) {
			perror("WooFMsgThread: couldn't send r_msg");
			exit(1);
		}
		/*
		 * wait for next PUT request
		 */
		msg = zmsg_recv(receiver);
	}

	pthread_exit(NULL);
}
		

int WooFMsgServer (char *namespace)
{

	int port;
	zactor_t *proxy;
	int err;
	pthread_t tid;
	char endpoint[255];

	zsock_t *frontend;
	zsock_t *workers;
	zmsg_t *msg;

	if(namespace[0] == 0) {
		fprintf(stderr,"WooFMsgServer: couldn't find namespace\n");
		fflush(stderr);
		exit(1);
	}

#ifdef DEBUG
	printf("WooFMsgServer: started for namespace %s\n",namespace);
	fflush(stdout);
#endif

	/*
	 * set up the front end router socket
	 */
	memset(endpoint,0,sizeof(endpoint));
	port = WooFPortHash(namespace);

	/*
	 * listen to any tcp address on port hash of namespace
	 */
	sprintf(endpoint,"tcp://*:%d",port);

#ifdef DEBUG
	printf("WooFMsgServer: fontend at %s\n",endpoint);
	fflush(stdout);
#endif

	/*
	 * create zproxy actor
	 */
	proxy = zactor_new(zproxy,NULL);
	if(proxy == NULL) {
		perror("WooFMsgServer: couldn't create proxy");
		exit(1);
	}

	/*
	 * create and bind endpoint with port has to frontend zsock
	 */
	zstr_sendx(proxy,"FRONTEND","ROUTER",endpoint,NULL);
	zsock_wait(proxy);

	/*
	 * inproc:// is a process internal enpoint for ZeroMQ
	 *
	 * if backend isn't in this process, this endpoint will need to be
	 * some kind of IPC endpoit.  For now, assume it is within the process
	 * and handled by threads
	 */
	zstr_sendx(proxy,"BACKEND","DEALER","inproc://workers",NULL);
	zsock_wait(proxy);

	/*
	 * create a single thread for now.  The DEALER pattern can handle multiple threads, however
	 * so this can be increased if need be
	 */
	err = pthread_create(&tid,NULL,WooFMsgThread,NULL);
	if(err < 0) {
		fprintf(stderr,"WooFMsgServer: couldn't create thread\n");
		exit(1);
	}

	/*
	 * right now, there is no way for this thread to exit so the msg server will block
	 * indefinitely in this join
	 */
	pthread_join(tid,NULL);

	/*
	 * we'll get here if the Msg thread is ever pthread_canceled()
	 */
	zactor_destroy(&proxy);
//	zsock_destroy(&frontend);
//	zsock_destroy(&workers);

	exit(0);
}

unsigned long WooFMsgPut(char *woof_name, char *hand_name, void *element, unsigned long el_size)
{
	zsock_t *server;
	char endpoint[255];
	char namespace[2048];
	char ip_str[25];
	int port;
	zmsg_t *msg;
	zmsg_t *r_msg;
	zframe_t *frame;
	zframe_t *r_frame;
	char buffer[255];
	char *str;
	struct timeval tm;
	unsigned long seq_no;
	int err;

	memset(namespace,0,sizeof(namespace));
	err = WooFNameSpaceFromURI(woof_name,namespace,sizeof(namespace));
	if(err < 0) {
		fprintf(stderr,"WooFMsgPut: woof: %s no name space\n",
			woof_name);
		fflush(stderr);
		return(-1);
	}

	memset(ip_str,0,sizeof(ip_str));
	err = WooFIPAddrFromURI(woof_name,ip_str,sizeof(ip_str));
	if(err < 0) {
		fprintf(stderr,"WooFMsgPut: woof: %s invalid IP address\n",
			woof_name);
		fflush(stderr);
		return(-1);
	}

	port = WooFPortHash(namespace);

	memset(endpoint,0,sizeof(endpoint));
	sprintf(endpoint,">tcp://%s:%d",ip_str,port);

#ifdef DEBUG
	printf("WooFMsgPut: woof: %s trying enpoint %s\n",woof_name,endpoint);
	fflush(stdout);
#endif
	server = zsock_new_req(endpoint);
	if(server == NULL) {
		fprintf(stderr,"WooFMsgPut: woof: %s no socket to server at %s\n",
			woof_name,endpoint);
		perror("WooFMsgPut: new req");
		fflush(stderr);
		return(-1);
	}

	msg = zmsg_new();
	if(msg == NULL) {
		fprintf(stderr,"WooFMsgPut: woof: %s no outbound msg to server at %s\n",
			woof_name,endpoint);
		perror("WooFMsgPut: allocating msg");
		fflush(stderr);
		return(-1);
	}

	/*
	 * make a frame for the namespace
	 */
	frame = zframe_new(namespace,strlen(namespace));
	if(frame == NULL) {
		fprintf(stderr,"WooFMsgPut: woof: %s no frame for namespace %s to server at %s\n",
			woof_name,namespace,endpoint);
		perror("WooFMsgPut: couldn't get new frame");
		fflush(stderr);
//		zmsg_destroy(&msg);
		return(-1);
	}
	/*
	 * add the namespace frame to the msg
	 */
	err = zmsg_append(msg,&frame);
	if(err < 0) {
		fprintf(stderr,"WooFMsgPut: woof: %s can't append namespace %s to frame to server at %s\n",
			woof_name,namespace,endpoint);
		perror("WooFMsgPut: couldn't append namespace frame");
//		zmsg_destroy(&msg);
		return(-1);
	}

	/*
	 * make a frame for the handler name
	 */
	memset(buffer,0,sizeof(buffer));
	strncpy(buffer,hand_name,sizeof(buffer));
	frame = zframe_new(buffer,strlen(buffer));
	if(frame == NULL) {
		fprintf(stderr,"WooFMsgPut: woof: %s no frame for handler name %s to server at %s\n",
			woof_name,hand_name,endpoint);
		perror("WooFMsgPut: couldn't get new handler frame");
		fflush(stderr);
//		zmsg_destroy(&msg);
		return(-1);
	}

	err = zmsg_append(msg,&frame);
	if(err < 0) {
		fprintf(stderr,"WooFMsgPut: woof: %s couldn't append frame for handler name %s to server at %s\n",
			woof_name,hand_name,endpoint);
		perror("WooFMsgPut: couldn't append handler frame");
		fflush(stderr);
//		zmsg_destroy(&msg);
		exit(1);
	}

	frame = zframe_new(element,el_size);
	if(frame == NULL) {
		fprintf(stderr,"WooFMsgPut: woof: %s no frame size %lu for handler name %s to server at %s\n",
			woof_name,el_size, hand_name,endpoint);
		perror("WooFMsgPut: couldn't get element frame");
		fflush(stderr);
//		zmg_destroy(&msg);
		return(-1);
	}
	err = zmsg_append(msg,&frame);
	if(err < 0) {
		fprintf(stderr,"WooFMsgPut: woof: %s couldn't append frame for element size %lu name %s to server at %s\n",
			woof_name,el_size,hand_name,endpoint);
		perror("WooFMsgPut: couldn't append element frame");
//		zmg_destroy(&msg);
		return(-1);
	}

#ifdef DEBUG
	printf("WooFMsgPut: woof: %s sending message to server at %s\n",
		woof_name, endpoint);
	fflush(stdout);
#endif

	err = zmsg_send(&msg,server);
	if(err < 0) {
		fprintf(stderr,"WooFMsgPut: woof: %s couldn't send msg for element size %lu name %s to server at %s\n",
			woof_name,el_size,hand_name,endpoint);
		perror("WooFMsgPut: couldn't send request");
//		zmsg_destroy(&msg);
		return(-1);
	}

	/*
	 * try to get back the seq_no from the remote put
	 */
	r_msg = zmsg_recv(server);
	if(r_msg == NULL) {
		fprintf(stderr,"WooFMsgPut: woof: %s couldn't recv msg for element size %lu name %s to server at %s\n",
			woof_name,el_size,hand_name,endpoint);
		perror("WooFMsgPut: no response received");
		fflush(stderr);
		return(-1);
	} else {
		r_frame = zmsg_first(r_msg);
		if(r_frame == NULL) {
			fprintf(stderr,"WooFMsgPut: woof: %s no recv frame for element size %lu name %s to server at %s\n",
				woof_name,el_size,hand_name,endpoint);
			perror("WooFMsgPut: no response frame");
//			zmsg_destroy(&r_msg);
			return(-1);
		}
		str = zframe_data(r_frame);
		seq_no = atol(str); 
//		zmsg_destroy(&r_msg);
	}
	
		
	zsock_destroy(&server);
#ifdef DEBUG
	printf("WooFMsgPut: woof: %s recvd seq_no %lu message to server at %s\n",
		woof_name,seq_no, endpoint);
	fflush(stdout);

#endif
	return(seq_no);
}


	

	

