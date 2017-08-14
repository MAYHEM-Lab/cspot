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
#include "woofc-access.h"

extern char Host_ip[25];

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

void WooFProcessPut(zmsg_t *req_msg, zsock_t *receiver)
{
	zmsg_t *r_msg;
	zframe_t *frame;
	zframe_t *r_frame;
	char *str;
	char woof_name[2048];
	char hand_name[2048];
	unsigned int copy_size;
	void *element;
	int count;
	unsigned long seq_no;
	char buffer[255];
	int err;

#ifdef DEBUG
printf("WooProcessPut: called\n");
fflush(stdout);
#endif
	/*
	 * WooFPut requires a woof_name, handler_name, and the data
	 *
	 * first frame is the message type
	 */
	frame = zmsg_first(req_msg);
	if(frame == NULL) {
		perror("WooFProcessPut: couldn't set cursor in msg");
		return;
	}

	frame = zmsg_next(req_msg);
	if(frame == NULL) {
		perror("WooFProcessPut: couldn't find woof_name in msg");
		return;
	}
	/*
	 * woof_name in the first frame
	 */
	memset(woof_name,0,sizeof(woof_name));
	str = (char *)zframe_data(frame);
	copy_size = zframe_size(frame);
	if(copy_size > (sizeof(woof_name)-1)) {
		copy_size = sizeof(woof_name)-1;
	}
	strncpy(woof_name,str,copy_size);

#ifdef DEBUG
printf("WooFProcessPut: received woof_name %s\n",woof_name);
fflush(stdout);
#endif
	/*
	 * handler name in the second frame
	 */
	memset(hand_name,0,sizeof(hand_name));
	frame = zmsg_next(req_msg);
	copy_size = zframe_size(frame);
	/*
	 * could be zero if there is no handler
	 */
	if(copy_size > 0) {
		str = (char *)zframe_data(frame);
		if(copy_size > (sizeof(hand_name)-1)) {
			copy_size = sizeof(hand_name)-1;
		}
		strncpy(hand_name,str,copy_size);
	}

#ifdef DEBUG
printf("WooFProcessPut: received handler name %s\n",hand_name);
fflush(stdout);
#endif
	/*
	 * raw data in the third frame
	 *
	 * need a maximum size but for now see if we can malloc() the space
	 */
	frame = zmsg_next(req_msg);
	copy_size = zframe_size(frame);
	element = malloc(copy_size);
	if(element == NULL) { /* element too big */
		fprintf(stderr,"WooFMsgProcessPut: woof_name: %s, handler: %s, size %lu too big\n",
			woof_name,hand_name,copy_size);
		fflush(stderr);
		seq_no = -1;
	} else {
		str = (char *)zframe_data(frame);
		memcpy(element,(void *)str,copy_size);
#ifdef DEBUG
printf("WooFProcessPut: received %lu bytes for the element\n", copy_size);
fflush(stdout);
#endif
		/*
		 * czmq docs say that this will free the internal frames within the
		 * message
		 */
//			zmsg_destroy(&msg);

		/*
		 * attempt to put the element into the local woof_name
		 *
		 * note that we could sanity check the woof_name against the local woof_name
		 * but WooFPut does this also
		 */
		if(hand_name[0] != 0) {
			seq_no = WooFPut(woof_name,hand_name,element);
		} else {
			seq_no = WooFPut(woof_name,NULL,element);
		}

		free(element);
	}

	/*
	 * send seq_no back
	 */
	r_msg = zmsg_new();
	if(r_msg == NULL) {
		perror("WooFProcessPut: no reply message");
		exit(1);
	}
	memset(buffer,0,sizeof(buffer));
	sprintf(buffer,"%lu",seq_no);
	r_frame = zframe_new(buffer,strlen(buffer));
	if(r_frame == NULL) {
		perror("WooFProcessPut: no reply frame");
		return;
	}
	err = zmsg_append(r_msg,&r_frame);
	if(err != 0) {
		perror("WooFProcessPut: couldn't append to r_msg");
		return;
	}
	err = zmsg_send(&r_msg,receiver);
	if(err != 0) {
		perror("WooFProcessPut: couldn't send r_msg");
		return;
	}

	return;
}

void WooFProcessGetElSize(zmsg_t *req_msg, zsock_t *receiver)
{
	zmsg_t *r_msg;
	zframe_t *frame;
	zframe_t *r_frame;
	char *str;
	char woof_name[2048];
	unsigned int copy_size;
	void *element;
	unsigned el_size;
	char buffer[255];
	int err;
	WOOF *wf;

#ifdef DEBUG
	printf("WooFProcessGetElSize: called\n");
	fflush(stdout);
#endif
	/*
	 * WooFElSize requires a woof_name
	 *
	 * first frame is the message type
	 */
	frame = zmsg_first(req_msg);
	if(frame == NULL) {
		perror("WooFProcessGetElSize: couldn't set cursor in msg");
		return;
	}

	frame = zmsg_next(req_msg);
	if(frame == NULL) {
		perror("WooFProcessGetElSize: couldn't find woof_name in msg");
		return;
	}
	/*
	 * woof_name in the first frame
	 */
	memset(woof_name,0,sizeof(woof_name));
	str = (char *)zframe_data(frame);
	copy_size = zframe_size(frame);
	if(copy_size > (sizeof(woof_name)-1)) {
		copy_size = sizeof(woof_name)-1;
	}
	strncpy(woof_name,str,copy_size);

	wf = WooFOpen(woof_name);
	if(wf == NULL) {
		fprintf(stderr,"WooFProcessGetElSize: couldn't open %s\n",woof_name);
		fflush(stderr);
		el_size = -1;
	} else {
		el_size = wf->shared->element_size;
		WooFFree(wf);
	}

#ifdef DEBUG
	printf("WooFProcessGetElSize: woof_name %s has element size: %lu\n",woof_name,el_size);
	fflush(stdout);
#endif

	/*
	 * send el_size back
	 */
	r_msg = zmsg_new();
	if(r_msg == NULL) {
		perror("WooFProcessGetElSize: no reply message");
		exit(1);
	}
	memset(buffer,0,sizeof(buffer));
	sprintf(buffer,"%lu",el_size);
	r_frame = zframe_new(buffer,strlen(buffer));
	if(r_frame == NULL) {
		perror("WooFProcessGetElSize: no reply frame");
		return;
	}
	err = zmsg_append(r_msg,&r_frame);
	if(err != 0) {
		perror("WooFProcessPut: couldn't append to r_msg");
		return;
	}
	err = zmsg_send(&r_msg,receiver);
	if(err != 0) {
		perror("WooFProcessGetElSize: couldn't send r_msg");
		return;
	}

	return;
}


void WooFProcessGet(zmsg_t *req_msg, zsock_t *receiver)
{
	zmsg_t *r_msg;
	zframe_t *frame;
	zframe_t *r_frame;
	char *str;
	char woof_name[2048];
	char hand_name[2048];
	unsigned int copy_size;
	void *element;
	unsigned long el_size;
	int count;
	unsigned long seq_no;
	char buffer[255];
	int err;
	WOOF *wf;

#ifdef DEBUG
	printf("WooFProcessGet: called\n");
	fflush(stdout);
#endif
	/*
	 * WooFGet requires a woof_name, and the seq_no
	 *
	 * first frame is the message type
	 */
	frame = zmsg_first(req_msg);
	if(frame == NULL) {
		perror("WooFProcessGet: couldn't set cursor in msg");
		return;
	}

	frame = zmsg_next(req_msg);
	if(frame == NULL) {
		perror("WooFProcessGet: couldn't find woof_name in msg");
		return;
	}
	/*
	 * woof_name in the first frame
	 */
	memset(woof_name,0,sizeof(woof_name));
	str = (char *)zframe_data(frame);
	copy_size = zframe_size(frame);
	if(copy_size > (sizeof(woof_name)-1)) {
		copy_size = sizeof(woof_name)-1;
	}
	strncpy(woof_name,str,copy_size);

#ifdef DEBUG
	printf("WooFProcessGet: received woof_name %s\n",woof_name);
	fflush(stdout);
#endif
	/*
	 * seq_no name in the second frame
	 */
	frame = zmsg_next(req_msg);
	copy_size = zframe_size(frame);
	if(copy_size > 0) {
		seq_no = atol((char *)zframe_data(frame));
#ifdef DEBUG
	printf("WooFProcessGet: received seq_no name %lu\n",seq_no);
	fflush(stdout);
#endif
		/*
		 * czmq docs say that this will free the internal frames within the
		 * message
		 */
//		zmsg_destroy(&msg);

		/*
		 * attempt to get the element from the local woof_name
		 */
		wf = WooFOpen(woof_name);
		if(wf == NULL) {
			fprintf(stderr,"WooFProcessGet: couldn't open woof: %s\n",
				woof_name);
			fflush(stderr);
			element = NULL;
			el_size = 0;
		} else {
			el_size = wf->shared->element_size;
			element = malloc(el_size);
			if(element == NULL) {
				fprintf(stderr,"WooFProcessGet: no space woof: %s\n",
					woof_name);
				fflush(stderr);
			} else {
				err = WooFRead(wf,element,seq_no);
				if(err < 0) {
					fprintf(stderr,
				"WooFProcessGet: read failed: %s at %lu\n",
					 woof_name,seq_no);
					free(element);
					element = NULL;
					el_size = 0;
				}
			}
			WooFFree(wf);
		}

	} else {
		fprintf(stderr,"WooFProcessGet: no seq_no frame data for %s\n",
			woof_name);
		fflush(stderr);
		element = NULL;
		el_size = 0;
	}

	/*
	 * send element back or empty frame indicating no dice
	 */
	r_msg = zmsg_new();
	if(r_msg == NULL) {
		perror("WooFProcessGet: no reply message");
		if(element != NULL) {
			free(element);
		}
		return;
	}
	if(element != NULL) {
#ifdef DEBUG
	printf("WooFProcessGet: creating frame for %lu bytes of element at seq_no %lu\n",
			el_size,seq_no);
	fflush(stdout);
#endif

		r_frame = zframe_new(element,el_size);
		if(r_frame == NULL) {
			perror("WooFProcessGet: no reply frame");
			free(element);
			return;
		}
	} else {
#ifdef DEBUG
	printf("WooFProcessGet: creating empty frame\n");
	fflush(stdout);
#endif
		r_frame = zframe_new_empty();
		if(r_frame == NULL) {
			perror("WooFProcessGet: no empty reply frame");
			return;
		}
	}
	err = zmsg_append(r_msg,&r_frame);
	if(element != NULL) {
		free(element);
	}
	if(err != 0) {
		perror("WooFProcessGet: couldn't append to r_msg");
		return;
	}
	err = zmsg_send(&r_msg,receiver);
	if(err != 0) {
		perror("WooFProcessGet: couldn't send r_msg");
		return;
	}

	return;
}

void *WooFMsgThread(void *arg)
{
	zsock_t *receiver;
	zmsg_t *msg;
	zmsg_t *r_msg;
	zframe_t *frame;
	unsigned int tag;
	char *str;
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
		 * WooFMsg starts with a message tag for dispatch
		 */
		frame = zmsg_first(msg);
		if(frame == NULL) {
			perror("WooFMsgThread: couldn't get tag");
			exit(1);
		}
		/*
		 * tag in the first frame
		 */
		str = (char *)zframe_data(frame);
		tag = atoi(str);
#ifdef DEBUG
	printf("WooFMsgThread: processing msg with tag: %d\n",tag);
	fflush(stdout);
#endif

		switch(tag) {
			case WOOF_MSG_PUT:
				WooFProcessPut(msg,receiver);
				break;
			case WOOF_MSG_GET:
				WooFProcessGet(msg,receiver);
				break;
			case WOOF_MSG_GET_EL_SIZE:
				WooFProcessGetElSize(msg,receiver);
				break;
			default:
				fprintf(stderr,"WooFMsgThread: unknown tag %s\n",
						str);
				break;
		}
		/*
		 * wait for next request
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

static zmsg_t *ServerRequest(char *endpoint, zmsg_t *msg)
{
	zsock_t *server;
	zpoller_t *resp_poll;
	zsock_t *server_resp; 
	int err;
	zmsg_t *r_msg;

	/*
	 * get a socket to the server
	 */
	server = zsock_new_req(endpoint);
	if(server == NULL) {
		fprintf(stderr,"ServerRequest: no server connection to %s\n",
			endpoint);
		fflush(stderr);
		return(NULL);
	}

	/*
	 * set up the poller for the reply
	 */
	resp_poll = zpoller_new(server,NULL);
	if(resp_poll == NULL) {
		fprintf(stderr,"ServerRequest: no poller for reply from %s\n",
			endpoint);
		fflush(stderr);
		zsock_destroy(&server);
		return(NULL);
	}

	/*
	 * send the message to the server
	 */
	err = zmsg_send(&msg,server);
	if(err < 0) {
		fprintf(stderr,"ServerRequest: msg send to %s failed\n",
			endpoint);
		fflush(stderr);
		zsock_destroy(&server);
		zpoller_destroy(&resp_poll);
		return(NULL);
	}

	/*
	 * wait for the reply, but not indefinitely
	 */
	server_resp = zpoller_wait(resp_poll,WOOF_MSG_REQ_TIMEOUT);
	if(server_resp != NULL) {
		r_msg = zmsg_recv(server_resp);
		if(r_msg == NULL) {
			fprintf(stderr,"ServerRequest: msg recv from %s failed\n",
				endpoint);
			fflush(stderr);
			zsock_destroy(&server);
			zpoller_destroy(&resp_poll);
			return(NULL);
		}
		zsock_destroy(&server);
		zpoller_destroy(&resp_poll);
		return(r_msg);
	} if(zpoller_expired(resp_poll)) {
		fprintf(stderr,"ServerRequest: msg recv timeout from %s after %d msec\n",
				endpoint,WOOF_MSG_REQ_TIMEOUT);
		fflush(stderr);
		zsock_destroy(&server);
		zpoller_destroy(&resp_poll);
		return(NULL);
	} else if(zpoller_terminated(resp_poll)) {
		fprintf(stderr,"ServerRequest: msg recv interrupted from %s\n",
				endpoint);
		fflush(stderr);
		zsock_destroy(&server);
		zpoller_destroy(&resp_poll);
		return(NULL);
	} else {
		fprintf(stderr,"ServerRequest: msg recv failed from %s\n",
				endpoint);
		fflush(stderr);
		zsock_destroy(&server);
		zpoller_destroy(&resp_poll);
		return(NULL);
	}

}
	

unsigned long WooFMsgGetElSize(char *woof_name)
{
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
	unsigned long el_size;
	int err;

	memset(namespace,0,sizeof(namespace));
	err = WooFNameSpaceFromURI(woof_name,namespace,sizeof(namespace));
	if(err < 0) {
		fprintf(stderr,"WooFMsgGetElSize: woof: %s no name space\n",
			woof_name);
		fflush(stderr);
		return(-1);
	}

	memset(ip_str,0,sizeof(ip_str));
	err = WooFIPAddrFromURI(woof_name,ip_str,sizeof(ip_str));
	if(err < 0) {
		fprintf(stderr,"WooFMsgGetElSize: woof: %s invalid IP address\n",
			woof_name);
		fflush(stderr);
		return(-1);
	}

	port = WooFPortHash(namespace);

	memset(endpoint,0,sizeof(endpoint));
	sprintf(endpoint,">tcp://%s:%d",ip_str,port);

#ifdef DEBUG
	printf("WooFMsgGetElSize: woof: %s trying enpoint %s\n",woof_name,endpoint);
	fflush(stdout);
#endif

	msg = zmsg_new();
	if(msg == NULL) {
		fprintf(stderr,"WooFMsgGetElSize: woof: %s no outbound msg to server at %s\n",
			woof_name,endpoint);
		perror("WooFMsgGetElSize: allocating msg");
		fflush(stderr);
		return(-1);
	}
#ifdef DEBUG
	printf("WooFMsgGetElSize: woof: %s got new msg\n",woof_name);
	fflush(stdout);
#endif

	/*
	 * this is a GetElSize message
	 */
	memset(buffer,0,sizeof(buffer));
	sprintf(buffer,"%lu",WOOF_MSG_GET_EL_SIZE);
	frame = zframe_new(buffer,strlen(buffer));
	if(frame == NULL) {
		fprintf(stderr,"WooFMsgGetElSize: woof: %s no frame for WOOF_MSG_GET_EL_SIZE command in to server at %s\n",
			woof_name,endpoint);
		perror("WooFMsgGetElSize: couldn't get new frame");
		fflush(stderr);
//		zmsg_destroy(&msg);
		return(-1);
	}

#ifdef DEBUG
	printf("WooFMsgGetElSize: woof: %s got WOOF_MSG_GET_EL_SIZE command frame frame\n",woof_name);
	fflush(stdout);
#endif
	err = zmsg_append(msg,&frame);
	if(err < 0) {
		fprintf(stderr,"WooFMsgGetElSize: woof: %s can't append WOOF_MSG_GET_EL_SIZE command frame to msg for server at %s\n",
			woof_name,endpoint);
		perror("WooFMsgGetElSize: couldn't append woof_name frame");
//		zmsg_destroy(&msg);
		return(-1);
	}

	/*
	 * make a frame for the woof_name
	 */
	frame = zframe_new(woof_name,strlen(woof_name));
	if(frame == NULL) {
		fprintf(stderr,"WooFMsgGetElSize: woof: %s no frame for woof_name to server at %s\n",
			woof_name,endpoint);
		perror("WooFMsgGetElSize: couldn't get new frame");
		fflush(stderr);
//		zmsg_destroy(&msg);
		return(-1);
	}
#ifdef DEBUG
	printf("WooFMsgGetElSize: woof: %s got woof_name namespace frame\n",woof_name);
	fflush(stdout);
#endif
	/*
	 * add the woof_name frame to the msg
	 */
	err = zmsg_append(msg,&frame);
	if(err < 0) {
		fprintf(stderr,"WooFMsgGetElSize: woof: %s can't append woof_name to frame to server at %s\n",
			woof_name,endpoint);
		perror("WooFMsgGetElSize: couldn't append woof_name namespace frame");
//		zmsg_destroy(&msg);
		return(-1);
	}
#ifdef DEBUG
	printf("WooFMsgGetElSize: woof: %s added woof_name namespace to frame\n",woof_name);
	fflush(stdout);
#endif

#ifdef DEBUG
	printf("WooFMsgGetElSize: woof: %s sending message to server at %s\n",
		woof_name, endpoint);
	fflush(stdout);
#endif

	r_msg = ServerRequest(endpoint,msg);

	if(r_msg == NULL) {
		fprintf(stderr,"WooFMsgGetElSize: woof: %s couldn't recv msg for element size from server at %s\n",
			woof_name,endpoint);
		perror("WooFMsgGetElSize: no response received");
		fflush(stderr);
		return(-1);
	} else {
		r_frame = zmsg_first(r_msg);
		if(r_frame == NULL) {
			fprintf(stderr,"WooFMsgGetElSize: woof: %s no recv frame for from server at %s\n",
				woof_name,endpoint);
			perror("WooFMsgGetElSize: no response frame");
//			zmsg_destroy(&r_msg);
			return(-1);
		}
		str = zframe_data(r_frame);
		el_size = atol(str); 
//		zmsg_destroy(&r_msg);
	}
	
		
#ifdef DEBUG
	printf("WooFMsgGetElSize: woof: %s recvd size: %lu message from server at %s\n",
		woof_name,el_size, endpoint);
	fflush(stdout);

#endif
	return(el_size);
}

unsigned long WooFMsgPut(char *woof_name, char *hand_name, void *element, unsigned long el_size)
{
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
	void *el_buffer;

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

	msg = zmsg_new();
	if(msg == NULL) {
		fprintf(stderr,"WooFMsgPut: woof: %s no outbound msg to server at %s\n",
			woof_name,endpoint);
		perror("WooFMsgPut: allocating msg");
		fflush(stderr);
		return(-1);
	}
#ifdef DEBUG
	printf("WooFMsgPut: woof: %s got new msg\n",woof_name);
	fflush(stdout);
#endif

	/*
	 * this is a put message
	 */
	memset(buffer,0,sizeof(buffer));
	sprintf(buffer,"%lu",WOOF_MSG_PUT);
	frame = zframe_new(buffer,strlen(buffer));
	if(frame == NULL) {
		fprintf(stderr,"WooFMsgPut: woof: %s no frame for WOOF_MSG_PUT command in to server at %s\n",
			woof_name,endpoint);
		perror("WooFMsgPut: couldn't get new frame");
		fflush(stderr);
//		zmsg_destroy(&msg);
		return(-1);
	}

#ifdef DEBUG
	printf("WooFMsgPut: woof: %s got WOOF_MSG_PUT command frame frame\n",woof_name);
	fflush(stdout);
#endif
	err = zmsg_append(msg,&frame);
	if(err < 0) {
		fprintf(stderr,"WooFMsgPut: woof: %s can't append WOOF_MSG_PUT command frame to msg for server at %s\n",
			woof_name,endpoint);
		perror("WooFMsgPut: couldn't append woof_name frame");
//		zmsg_destroy(&msg);
		return(-1);
	}

	/*
	 * make a frame for the woof_name
	 */
	frame = zframe_new(woof_name,strlen(woof_name));
	if(frame == NULL) {
		fprintf(stderr,"WooFMsgPut: woof: %s no frame for woof_name to server at %s\n",
			woof_name,endpoint);
		perror("WooFMsgPut: couldn't get new frame");
		fflush(stderr);
//		zmsg_destroy(&msg);
		return(-1);
	}
#ifdef DEBUG
	printf("WooFMsgPut: woof: %s got woof_name namespace frame\n",woof_name);
	fflush(stdout);
#endif
	/*
	 * add the woof_name frame to the msg
	 */
	err = zmsg_append(msg,&frame);
	if(err < 0) {
		fprintf(stderr,"WooFMsgPut: woof: %s can't append woof_name to frame to server at %s\n",
			woof_name,endpoint);
		perror("WooFMsgPut: couldn't append woof_name namespace frame");
//		zmsg_destroy(&msg);
		return(-1);
	}
#ifdef DEBUG
	printf("WooFMsgPut: woof: %s added woof_name namespace to frame\n",woof_name);
	fflush(stdout);
#endif

	/*
	 * make a frame for the handler name
	 * could be NULL
	 */
	
	if(hand_name != NULL) {
		memset(buffer,0,sizeof(buffer));
		strncpy(buffer,hand_name,sizeof(buffer));
		frame = zframe_new(buffer,strlen(buffer));
	} else { /* czmq indicate this will work */
		frame = zframe_new_empty();
	}
	if(frame == NULL) {
		fprintf(stderr,"WooFMsgPut: woof: %s no frame for handler name %s to server at %s\n",
			woof_name,hand_name,endpoint);
		perror("WooFMsgPut: couldn't get new handler frame");
		fflush(stderr);
//		zmsg_destroy(&msg);
		return(-1);
	}
#ifdef DEBUG
	printf("WooFMsgPut: woof: %s got frame for handler name %s\n",woof_name,hand_name);
	fflush(stdout);
#endif

	err = zmsg_append(msg,&frame);
	if(err < 0) {
		fprintf(stderr,"WooFMsgPut: woof: %s couldn't append frame for handler name %s to server at %s\n",
			woof_name,hand_name,endpoint);
		perror("WooFMsgPut: couldn't append handler frame");
		fflush(stderr);
//		zmsg_destroy(&msg);
		return(-1);
	}
#ifdef DEBUG
	printf("WooFMsgPut: woof: %s appended frame for handler name %s\n",woof_name,hand_name);
	fflush(stdout);
#endif

#ifdef DEBUG
	printf("WooFMsgPut: woof: %s for new frame for 0x%x, size %lu\n",
		woof_name, element,el_size);
	fflush(stdout);
#endif
	el_buffer = malloc(el_size);
	if(el_buffer == NULL) {
		exit(1);
	}
	memcpy(el_buffer,element,el_size);
	frame = zframe_new(el_buffer,el_size);
	if(frame == NULL) {
		fprintf(stderr,"WooFMsgPut: woof: %s no frame size %lu for handler name %s to server at %s\n",
			woof_name,el_size, hand_name,endpoint);
		perror("WooFMsgPut: couldn't get element frame");
		fflush(stderr);
//		zmg_destroy(&msg);
		return(-1);
	}
#ifdef DEBUG
	printf("WooFMsgPut: woof: %s got frame for element size %d\n",woof_name,el_size);
	fflush(stdout);
#endif
	
	err = zmsg_append(msg,&frame);
	if(err < 0) {
		fprintf(stderr,"WooFMsgPut: woof: %s couldn't append frame for element size %lu name %s to server at %s\n",
			woof_name,el_size,hand_name,endpoint);
		perror("WooFMsgPut: couldn't append element frame");
//		zmg_destroy(&msg);
		free(el_buffer);
		return(-1);
	}
#ifdef DEBUG
	printf("WooFMsgPut: woof: %s appended frame for element size %d\n",woof_name,el_size);
	fflush(stdout);
#endif

	free(el_buffer);

#ifdef DEBUG
	printf("WooFMsgPut: woof: %s sending message to server at %s\n",
		woof_name, endpoint);
	fflush(stdout);
#endif

	r_msg = ServerRequest(endpoint,msg);
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
	
		
#ifdef DEBUG
	printf("WooFMsgPut: woof: %s recvd seq_no %lu message to server at %s\n",
		woof_name,seq_no, endpoint);
	fflush(stdout);

#endif
	return(seq_no);
}

int WooFMsgGet(char *woof_name, void *element, unsigned long el_size, unsigned long seq_no)
{
	char endpoint[255];
	char namespace[2048];
	char ip_str[25];
	int port;
	zmsg_t *msg;
	zmsg_t *r_msg;
	int r_size;
	zframe_t *frame;
	zframe_t *r_frame;
	char buffer[255];
	char *str;
	struct timeval tm;
	int err;

	memset(namespace,0,sizeof(namespace));
	err = WooFNameSpaceFromURI(woof_name,namespace,sizeof(namespace));
	if(err < 0) {
		fprintf(stderr,"WooFMsgGet: woof: %s no name space\n",
			woof_name);
		fflush(stderr);
		return(-1);
	}

	memset(ip_str,0,sizeof(ip_str));
	err = WooFIPAddrFromURI(woof_name,ip_str,sizeof(ip_str));
	if(err < 0) {
		fprintf(stderr,"WooFMsgGet: woof: %s invalid IP address\n",
			woof_name);
		fflush(stderr);
		return(-1);
	}

	port = WooFPortHash(namespace);

	memset(endpoint,0,sizeof(endpoint));
	sprintf(endpoint,">tcp://%s:%d",ip_str,port);

#ifdef DEBUG
	printf("WooFMsgGet: woof: %s trying enpoint %s\n",woof_name,endpoint);
	fflush(stdout);
#endif

	msg = zmsg_new();
	if(msg == NULL) {
		fprintf(stderr,"WooFMsgGet: woof: %s no outbound msg to server at %s\n",
			woof_name,endpoint);
		perror("WooFMsgGet: allocating msg");
		fflush(stderr);
		return(-1);
	}
#ifdef DEBUG
	printf("WooFMsgGet: woof: %s got new msg\n",woof_name);
	fflush(stdout);
#endif

	/*
	 * this is a put message
	 */
	memset(buffer,0,sizeof(buffer));
	sprintf(buffer,"%lu",WOOF_MSG_GET);
	frame = zframe_new(buffer,strlen(buffer));
	if(frame == NULL) {
		fprintf(stderr,"WooFMsgGet: woof: %s no frame for WOOF_MSG_GET command in to server at %s\n",
			woof_name,endpoint);
		perror("WooFMsgGet: couldn't get new frame");
		fflush(stderr);
//		zmsg_destroy(&msg);
		return(-1);
	}

#ifdef DEBUG
	printf("WooFMsgGet: woof: %s got WOOF_MSG_GET command frame frame\n",woof_name);
	fflush(stdout);
#endif
	err = zmsg_append(msg,&frame);
	if(err < 0) {
		fprintf(stderr,"WooFMsgGet: woof: %s can't append WOOF_MSG_GET command frame to msg for server at %s\n",
			woof_name,endpoint);
		perror("WooFMsgGet: couldn't append woof_name frame");
//		zmsg_destroy(&msg);
		return(-1);
	}

	/*
	 * make a frame for the woof_name
	 */
	frame = zframe_new(woof_name,strlen(woof_name));
	if(frame == NULL) {
		fprintf(stderr,"WooFMsgGet: woof: %s no frame for woof_name to server at %s\n",
			woof_name,endpoint);
		perror("WooFMsgGet: couldn't get new frame");
		fflush(stderr);
//		zmsg_destroy(&msg);
		return(-1);
	}
#ifdef DEBUG
	printf("WooFMsgGet: woof: %s got woof_name namespace frame\n",woof_name);
	fflush(stdout);
#endif
	/*
	 * add the woof_name frame to the msg
	 */
	err = zmsg_append(msg,&frame);
	if(err < 0) {
		fprintf(stderr,"WooFMsgGet: woof: %s can't append woof_name to frame to server at %s\n",
			woof_name,endpoint);
		perror("WooFMsgGet: couldn't append woof_name namespace frame");
//		zmsg_destroy(&msg);
		return(-1);
	}
#ifdef DEBUG
	printf("WooFMsgGet: woof: %s added woof_name namespace to frame\n",woof_name);
	fflush(stdout);
#endif

	/*
	 * make a frame for the seq_no
	 */
	
	memset(buffer,0,sizeof(buffer));
	sprintf(buffer,"%lu",seq_no);
	frame = zframe_new(buffer,strlen(buffer));

	if(frame == NULL) {
		fprintf(stderr,"WooFMsgGet: woof: %s no frame for seq_no %lu server at %s\n",
			woof_name,seq_no,endpoint);
		perror("WooFMsgGet: couldn't get new seq_no frame");
		fflush(stderr);
//		zmsg_destroy(&msg);
		return(-1);
	}
#ifdef DEBUG
	printf("WooFMsgGet: woof: %s got frame for seq_no %lu\n",woof_name,seq_no);
	fflush(stdout);
#endif

	err = zmsg_append(msg,&frame);
	if(err < 0) {
		fprintf(stderr,"WooFMsgGet: woof: %s couldn't append frame for seq_no %lu to server at %s\n",
			woof_name,seq_no,endpoint);
		perror("WooFMsgGet: couldn't append seq_no frame");
		fflush(stderr);
//		zmsg_destroy(&msg);
		return(-1);
	}
#ifdef DEBUG
	printf("WooFMsgGet: woof: %s appended frame for seq_no %lu\n",woof_name,seq_no);
	fflush(stdout);
#endif

#ifdef DEBUG
	printf("WooFMsgGet: woof: %s sending message to server at %s\n",
		woof_name, endpoint);
	fflush(stdout);
#endif

	r_msg = ServerRequest(endpoint,msg);
	if(r_msg == NULL) {
		fprintf(stderr,"WooFMsgGet: woof: %s couldn't recv msg for seq_no %lu name %s to server at %s\n",
			woof_name,seq_no,endpoint);
		perror("WooFMsgGet: no response received");
		fflush(stderr);
		return(-1);
	} else {
		r_frame = zmsg_first(r_msg);
		if(r_frame == NULL) {
			fprintf(stderr,"WooFMsgGet: woof: %s no recv frame for seq_no %lu to server at %s\n",
				woof_name,seq_no,endpoint);
			perror("WooFMsgGet: no response frame");
//			zmsg_destroy(&r_msg);
			return(-1);
		}
		r_size = zframe_size(r_frame);
		if(r_size == 0) {
			fprintf(stderr,"WooFMsgGet: woof: %s no data in frame for seq_no %lu to server at %s\n",
				woof_name,seq_no,endpoint);
			perror("WooFMsgGet: no data frame");
//			zmsg_destroy(&r_msg);
			return(-1);

		} else {
			if(r_size > el_size) {
				r_size = el_size;
			}
			memcpy(element,zframe_data(r_frame),r_size);
#ifdef DEBUG
	printf("WooFMsgGet: woof: %s copied %lu bytes for seq_no %lu message to server at %s\n",
		woof_name,r_size, seq_no, endpoint);
	fflush(stdout);

#endif
//			zmsg_destroy(&r_msg);
		}
	}
	
		
#ifdef DEBUG
	printf("WooFMsgGet: woof: %s recvd seq_no %lu message to server at %s\n",
		woof_name,seq_no, endpoint);
	fflush(stdout);

#endif
	return(1);
}
	

