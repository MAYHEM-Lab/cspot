#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <zmq.h>
#include <pthread.h>
#include <czmq.h>
#include <time.h>

static int TempHash(char *namespace)
{
	return(5555);
}


void *WooFMsgThread(void *arg)
{
	void *context = arg;
	void *receiver;
	zmq_msg_t msg;
	zmq_msg_t r_msg;
	char *str;
	int count;
	unsigned long seq_no;
	char buffer[255];
	int err;

	receiver = zmq_socket(context,ZMQ_REP);

	err = zmq_connect(receiver,"inproc://workers");
	if(err != 0) {
		perror("WooFMsgThread: couldn't connect");
		exit(1);
	}

	seq_no = 0;

	err = zmq_msg_init(&msg);
	if(err != 0) {
		perror("WooFMsgThread: bad message init");
		exit(1);
	}
printf("msg thread about to call receive\n");
fflush(stdout);
	err = zmq_msg_recv(&msg,receiver,0);
	while(err >= 0)
	{
printf("msg received\n");
fflush(stdout);
		count = 0;
		str = (char *)zmq_msg_data(&msg);
		printf("WooFMsgThread: seq_no: %lu, received %s (%d)\n",
			seq_no,str,count);
		count++;
		if(zmq_msg_more(&msg)) {
			zmq_msg_close(&msg);
			zmq_msg_init(&msg);
			err = zmq_msg_recv(&msg,receiver,0);
			if(err == -1) {
				perror("WooFMsgThread: bad 2nd part");
				exit(1);
			}
		}
		str = (char *)zmq_msg_data(&msg);
		printf("WooFMsgThread: seq_no: %lu, received %s (%d)\n",
			seq_no,str,count);

		if(zmq_msg_more(&msg)) {
			zmq_msg_close(&msg);
			zmq_msg_init(&msg);
			err = zmq_msg_recv(&msg,receiver,0);
			if(err == -1) {
				perror("WooFMsgThread: bad 2nd part");
				exit(1);
			}
		}
		str = (char *)zmq_msg_data(&msg);
		printf("WooFMsgThread: seq_no: %lu, received %s (%d)\n",
			seq_no,str,count);
		fflush(stdout);

		zmq_msg_close(&msg);

		memset(buffer,0,sizeof(buffer));
		sprintf(buffer,"%lu",seq_no);

		err = zmq_msg_init_size(&r_msg,strlen(buffer));
		if(err == -1) {
			perror("WooFMsgThread: bad reply init");
			exit(1);
		}

		memcpy(zmq_msg_data(&r_msg),buffer,strlen(buffer));

		err = zmq_msg_send(&r_msg,receiver,0);
		if(err < 0) {
			perror("WooFMsgThread: couldn't send r_msg");
			exit(1);
		}
		seq_no++;
		zmq_msg_init(&msg);
		err = zmq_msg_recv(&msg,receiver,0);
	}

printf("msg thread exit\n");
fflush(stdout);

	pthread_exit(NULL);
}
		
void *WooFMsgHandler(void *arg)
{
	
	void *receiver = arg;
	zmq_msg_t msg;
	zmq_msg_t r_msg;
	char *str;
	int count;
	unsigned long seq_no;
	char buffer[255];
	int err;

	err = zmq_msg_init(&msg);
	if(err != 0) {
		perror("WooFMsgThread: bad message init");
		exit(1);
	}
printf("handler thread about to call receive\n");
fflush(stdout);
	err = zmq_msg_recv(&msg,receiver,0);
	while(1)
	{
printf("msg received\n");
fflush(stdout);
		count = 0;
		str = (char *)zmq_msg_data(&msg);
		printf("WooFMsgThread: seq_no: %lu, received %s (%d)\n",
			seq_no,str,count);
		count++;
		if(zmq_msg_more(&msg)) {
			zmq_msg_close(&msg);
			err = zmq_msg_recv(&msg,receiver,0);
			if(err == -1) {
				perror("WooFMsgThread: bad 2nd part");
				exit(1);
			}
		}
		str = (char *)zmq_msg_data(&msg);
		printf("WooFMsgThread: seq_no: %lu, received %s (%d)\n",
			seq_no,str,count);

		if(zmq_msg_more(&msg)) {
			zmq_msg_close(&msg);
			err = zmq_msg_recv(&msg,receiver,0);
			if(err == -1) {
				perror("WooFMsgThread: bad 2nd part");
				exit(1);
			}
		}
		str = (char *)zmq_msg_data(&msg);
		printf("WooFMsgThread: seq_no: %lu, received %s (%d)\n",
			seq_no,str,count);
		fflush(stdout);

		zmq_msg_close(&msg);

		err = zmq_msg_init(&r_msg);
		if(err == -1) {
			perror("WooFMsgThread: bad reply init");
			exit(1);
		}

		memset(buffer,0,sizeof(buffer));
		sprintf(buffer,"%lu",seq_no);
		memcpy(zmq_msg_data(&r_msg),buffer,strlen(buffer));

		err = zmq_msg_send(&r_msg,receiver,0);
		if(err <= 0) {
			perror("WooFMsgThread: couldn't send r_msg");
			exit(1);
		}
		seq_no++;
		zmq_msg_init(&msg);
		err = zmq_msg_recv(&msg,receiver,0);
	}

printf("msg thread exit\n");
fflush(stdout);

	pthread_exit(NULL);
}
int WooFMsgServer (char *namespace)
{

	void *workers;
	void *frontend;
	void *context;
	zmq_msg_t p_msg;
	zmq_msg_t r_msg;
	int pid;

	int port;
	int err;
	pthread_t tid;
	char endpoint[255];

	context = zmq_ctx_new();

	/*
	 * set up the front end router socket
	 */
	memset(endpoint,0,sizeof(endpoint));
	port = TempHash(namespace);
	sprintf(endpoint,"tcp://*:%d",port);

	frontend = zmq_socket(context,ZMQ_ROUTER);
	if(frontend == NULL) {
		fprintf(stderr,"WooFMsgServer: bad endpoint on create: %s\n",
			endpoint);
		perror("WooFMsgServer: frontend create");
		exit(1);
	}
	printf("attempting bind to %s\n",endpoint);

	err = zmq_bind(frontend,endpoint);
	if(err != 0) {
		perror("WooFMsgServer: couldn't bind font end\n");
		exit(1);
	}

	printf("fontend at %s\n",endpoint);

	/*
	 * may want to make inproc namespace specific name
	 * instead of 'workers'
	 */
	workers = zmq_socket(context,ZMQ_DEALER);
	if(workers == NULL) {
		perror("WooFMsgServer: workers create");
		exit(1);
	}
	err = zmq_bind(workers,"inproc://workers");
	if(err != 0) {
		perror("WooFMsgServer: couldn't bind workers\n");
		exit(1);
	}

	err = pthread_create(&tid,NULL,WooFMsgThread,context);
	if(err < 0) {
		perror("WooFMsgServer: create failed\n");
		exit(1);
	}


	zmq_proxy(frontend,workers,NULL);

#if 0
	zmq_msg_init(&p_msg);
	while(1) {
printf("server calling recv\n");
fflush(stdout);
		zmq_msg_recv(&p_msg,frontend,0);
printf("server got message\n");
fflush(stdout);
		if(zmq_msg_more(&p_msg)) {
			zmq_msg_send(&p_msg,workers,ZMQ_SNDMORE);
printf("server forwraded message part\n");
fflush(stdout);
		} else {
			zmq_msg_send(&p_msg,workers,0);
printf("server forwraded last part\n");
fflush(stdout);
			zmq_msg_init(&r_msg);
printf("server waiting for reply\n");
fflush(stdout);
			zmq_msg_recv(&r_msg,workers,0);
			zmq_msg_send(&r_msg,frontend,0);
		}
	}
#endif
	pthread_join(tid,NULL);

	zmq_close (frontend);
	zmq_close (workers);
	zmq_ctx_destroy (context);	

	exit(0);
}

#define MY_IP "127.0.0.1"
unsigned long WooFMsgPut(char *woof_name, char *hand_name, void *element)
{
	char endpoint[255];
	int port;
	char buffer[255];
	char *str;
	struct timeval tm;
	unsigned long seq_no;
	int err;

	void *context;
	void *server;
	zmq_msg_t msg;
	zmq_msg_t msg1;
	zmq_msg_t msg2;
	zmq_msg_t r_msg;

	port = TempHash(woof_name);

	memset(endpoint,0,sizeof(endpoint));
	sprintf(endpoint,"tcp://%s:%d",MY_IP,port);

	printf("trying enpoint %s\n",endpoint);

	context = zmq_ctx_new();
	if(context == NULL) {
		perror("WoofMsgPut: context init failed");
		exit(1);
	}

    //  Socket to talk to server
	server = zmq_socket (context, ZMQ_REQ);
	if(server == NULL) {
		perror("WoofMsgPut: socket failed");
		exit(1);
	}
	err = zmq_connect (server, endpoint);
	if(err < 0) {
		perror("WoofMsgPut: connect failed");
		exit(1);
	}


	memset(buffer,0,sizeof(buffer));
	strncpy(buffer,"WooFPutMsg",strlen("WooFPutMsg"));
	err = zmq_msg_init_size(&msg,strlen(buffer));
	if(err < 0) {
		perror("WoofMsgPut: msg init failed");
		exit(1);
	}
	strncpy(zmq_msg_data(&msg),buffer,strlen(buffer));
	zmq_msg_send(&msg,server,ZMQ_SNDMORE);

	memset(buffer,0,sizeof(buffer));
	gettimeofday(&tm,NULL);
	sprintf(buffer,"Now: %lu",tm.tv_sec);
	err = zmq_msg_init_size(&msg1,strlen(buffer));
	if(err < 0) {
		perror("WoofMsgPut: msg1 init failed");
		exit(1);
	}
	strncpy(zmq_msg_data(&msg1),buffer,strlen(buffer));
	zmq_msg_send(&msg1,server,ZMQ_SNDMORE);
	
	memset(buffer,0,sizeof(buffer));
	strncpy(buffer,"WooFPutMsg-done",strlen("WooFPutMsg-done"));
	err = zmq_msg_init_size(&msg2,strlen(buffer));
	if(err < 0) {
		perror("WoofMsgPut: msg2 init failed");
		exit(1);
	}
	strncpy(zmq_msg_data(&msg2),buffer,strlen(buffer));
	zmq_msg_send(&msg2,server,0);


	printf("sending message to server\n");
	fflush(stdout);

	err = zmq_msg_init(&r_msg);
	if(err < 0) {
		perror("WoofMsgPut: r_msg init failed");
		exit(1);
	}

	err = zmq_msg_recv(&r_msg,server,0);
	if(err < 0) {
		perror("WoofMsgPut: r_msg receive failed");
		seq_no = -1;
	} else {
		str = zmq_msg_data(&r_msg);
		seq_no = atol(str); 
		zmq_msg_close(&r_msg);
	}
	
		
	zmq_close(server);
	zmq_ctx_destroy(context);

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

	

	

