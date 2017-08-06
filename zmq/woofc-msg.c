#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <zmq.h>
#include <czmq.h>

static int TempHash(char *namespace)
{
	return(6029);
}


void *WooFMsgThread(void *arg)
{
	zsock_t *receiver;
	zmsg_t *msg;
	zmsg_t *r_msg;
	zframe_t *frame;
	zframe_t *r_frame;
	char *str;
	int count;
	unsigned long seq_no;
	char buffer[255];
	int err;

	receiver = zsock_new_rep(">inproc://workers");
	if(receiver == NULL) {
		perror("WooFMsgThread: couldn't open receiver");
		pthread_exit(NULL);
	} 


	seq_no = 0;

	printf("msg thread about to call receive\n");
	fflush(stdout);
	msg = zmsg_recv(receiver);
	while(msg != NULL)
	{
printf("msg received\n");
fflush(stdout);
		count = 0;
		frame = zmsg_first(msg);
		if(frame == NULL) {
			perror("WooFMsgThread: couldn't set cursor in msg");
			exit(1);
		}
printf("cursor set\n");
fflush(stdout);
		str = (char *)zframe_data(frame);
		printf("WooFMsgThread: seq_no: %lu, received %s (%d)\n",
			seq_no,str,count);
		count++;
		frame = zmsg_next(msg);
		str = (char *)zframe_data(frame);
		printf("WooFMsgThread: seq_no: %lu, received %s (%d)\n",
			seq_no,str,count);
		count++;
		frame = zmsg_next(msg);
		str = (char *)zframe_data(frame);
		printf("WooFMsgThread: seq_no: %lu, received %s (%d)\n",
			seq_no,str,count);
		zmsg_destroy(&msg);

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
		seq_no++;
		msg = zmsg_recv(receiver);
	}

	pthread_exit(NULL);
}
		

/*
 * server can't use czmq since zproxy appears to be broken
 */
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


	/*
	 * set up the front end router socket
	 */
	memset(endpoint,0,sizeof(endpoint));
	port = TempHash(namespace);
	sprintf(endpoint,"tcp://*:%d",port);

#if 0
	frontend = zsock_new_router(endpoint);
	if(frontend == NULL) {
		fprintf(stderr,"WooFMsgServer: bad endpoint on create: %s\n",
			endpoint);
		perror("WooFMsgServer: frontend create");
		exit(1);
	}
#endif

	printf("fontend at %s\n",endpoint);

	/*
	 * may want to make inproc namespace specific name
	 * instead of 'workers'
	 */
#if 0
	workers = zsock_new_dealer("inproc://workers");

#endif

	proxy = zactor_new(zproxy,NULL);
	if(proxy == NULL) {
		perror("WooFMsgServer: couldn't create proxy");
		exit(1);
	}

	zstr_sendx(proxy,"FRONTEND","ROUTER",endpoint,NULL);
	zsock_wait(proxy);
	zstr_sendx(proxy,"BACKEND","DEALER","inproc://workers",NULL);
	zsock_wait(proxy);

	err = pthread_create(&tid,NULL,WooFMsgThread,NULL);
	if(err < 0) {
		fprintf(stderr,"WooFMsgServer: couldn't create thread\n");
		exit(1);
	}
	pthread_join(tid,NULL);

//	zactor_desroy(&proxy);
//	zsock_destroy(&frontend);
//	zsock_destroy(&workers);
	

	exit(0);
}

#define MY_IP "10.1.5.30"
unsigned long WooFMsgPut(char *woof_name, char *hand_name, void *element)
{
	zsock_t *server;
	char endpoint[255];
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

	port = TempHash(woof_name);

	memset(endpoint,0,sizeof(endpoint));
	sprintf(endpoint,">tcp://%s:%d",MY_IP,port);

	printf("trying enpoint %s\n",endpoint);

	server = zsock_new_req(endpoint);
	if(server == NULL) {
		perror("WooFMsgPut: creating server");
		exit(1);
	}

	msg = zmsg_new();
	if(msg == NULL) {
		perror("WooFMsgPut: allocating msg");
		exit(1);
	}

	frame = zframe_new("WooFPutMsg",strlen("WooFPutMsg"));
	if(frame == NULL) {
		perror("WooFMsgPut: couldn't get new frame");
		exit(1);
	}
	err = zmsg_append(msg,&frame);
	if(err != 0) {
		perror("WooFMsgPut: couldn't append 1st frame");
		exit(1);
	}

	memset(buffer,0,sizeof(buffer));
	gettimeofday(&tm,NULL);
	sprintf(buffer,"Now: %lu",tm.tv_sec);
	frame = zframe_new(buffer,strlen(buffer));
	err = zmsg_append(msg,&frame);
	if(err != 0) {
		perror("WooFMsgPut: couldn't append 2rd frame");
		exit(1);
	}

	frame = zframe_new("WooFPutMsg-done",strlen("WooFPutMsg-done"));
	err = zmsg_append(msg,&frame);
	if(err != 0) {
		perror("WooFMsgPut: couldn't append 3rd frame");
		exit(1);
	}

	printf("sending message to server\n");
	fflush(stdout);

	err = zmsg_send(&msg,server);
	if(err != 0) {
		perror("WooFMsgPut: couldn't send request");
		exit(1);
	}

	r_msg = zmsg_recv(server);
	if(r_msg == NULL) {
		fprintf(stderr,"WooFMsgPut: no response received\n");
		fflush(stderr);
		seq_no = -1;
	} else {

		r_frame = zmsg_first(r_msg);
		str = zframe_data(r_frame);
		seq_no = atol(str); 
		zmsg_destroy(&r_msg);
	}
	
		
	zmsg_destroy(&msg);
	zsock_destroy(&server);

	return(seq_no);
}

#ifdef SERVER
int main(int argc, char **argv)
{
	WooFMsgServer("dummynamespace");
	return(0);
}

#else // CLIENT

int main(int argc, char **argv)
{
	unsigned long seq_no;
	seq_no = WooFMsgPut("dummynamespace","dummy2",(void *)"dummy3");
	printf("WooFTestClient: got seq_no %lu\n",seq_no);
	fflush(stdout);
	return(0);
}

#endif

	

	

