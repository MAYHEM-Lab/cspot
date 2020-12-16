#include <stdlib.h>
#include <unistd.h>
#include <czmq.h>
#include <time.h>

#define TIMEOUT (1000)
#define DEBUG


#ifdef SERVER
#define ARGS "t:p:q"
char *Usage = "zproxy-thread -p port -t threads -q <quiet>\n";
Quiet = 0;

void *MsgThread(void * arg)
{
	zsock_t *receiver;
        zmsg_t *msg;
        zmsg_t *r_msg;
        zframe_t *frame;
        zframe_t *r_frame;
        unsigned long tag;
        char *str;
        int err;
	char buffer[255];

	receiver = zsock_new_rep(">inproc://workers");
        if (receiver == NULL)
        {
                perror("MsgThread: couldn't open receiver");
                pthread_exit(NULL);
        }
	msg = zmsg_recv(receiver);
	while(msg != NULL)
	{
#ifdef DEBUG
		printf("MsgThread: received\n");
		fflush(stdout);
#endif
		/*
		 * WooFMsg starts with a message tag for dispatch
		 */
		frame = zmsg_first(msg);
		if (frame == NULL)
		{
			perror("MsgThread: couldn't get tag");
			break;
		}
		/*
		 * tag in the first frame
		 */
		str = (char *)zframe_data(frame);
		tag = strtoul(str, (char **)NULL, 10);

		r_msg = zmsg_new();
		if (r_msg == NULL)
		{        
			perror("MsgThread: couldn't get r_msg\n");
			break;
		}
		memset(buffer, 0, sizeof(buffer));
        	sprintf(buffer, "%lu", tag);
		if(Quiet == 0) {
			printf("MsgThread: replying with tag: %s\n", buffer);
			fflush(stdout);
		}
        	r_frame = zframe_new(buffer, strlen(buffer));
        	if (r_frame == NULL)
		{       
			perror("MsgThread: no reply frame");
			zmsg_destroy(&r_msg);
			break;
		}
        	err = zmsg_append(r_msg, &r_frame);

		if (err != 0)   
		{       
			perror("WooFProcessPut: couldn't append to r_msg");
			zframe_destroy(&r_frame);
			zmsg_destroy(&r_msg);
			break;
		}       
        	err = zmsg_send(&r_msg, receiver); 
        	if (err != 0)   
		{               
			perror("WooFProcessGetElSize: couldn't send r_msg");
			zmsg_destroy(&r_msg);
			break;
		}       


		zmsg_destroy(&r_msg);
		zmsg_destroy(&msg);

		/*
		 * wait for next request
		 */
		msg = zmsg_recv(receiver);
	}

	zsock_destroy(&receiver);
	pthread_exit(NULL);
	return(0);
}

int main (int argc, char **argv)
{
	int c;
	int port;
        zactor_t *proxy;
        int err;
        char endpoint[255];
        pthread_t *tids;
        int i;
	int threads;
	int quiet;

        zsock_t *frontend;
        zsock_t *workers;
        zmsg_t *msg;

	threads = 0;
	port = 0;
	Quiet = 0;
	while((c=getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 't':
				threads = atoi(optarg);
				break;
			case 'p':
				port = atoi(optarg);
				break;
			case 'q':
				Quiet = 1;
				break;
			default:
				fprintf(stderr,
					"unrecognized argument %c\n",
						(char)c);
				fprintf(stderr,"%s",Usage);
				exit(1);
		}
	}

	if(threads == 0) {
		threads = 1;
	}
	if(port == 0) {
		port = 50000;
	}

	tids = (pthread_t *)malloc(threads*sizeof(pthread_t));
	if(tids == NULL) {
		exit(1);
	}
	sprintf(endpoint, "tcp://*:%d", port);

	/*
	 * create the proxy actor
	 */
        proxy = zactor_new(zproxy, NULL);
        if (proxy == NULL)
        {
                perror("zproxy-thread: couldn't create proxy");
                exit(1);
        } 

        /*
         * create and bind endpoint with port has to frontend zsock
         */
        zstr_sendx(proxy, "FRONTEND", "ROUTER", endpoint, NULL);
        zsock_wait(proxy);

	zstr_sendx(proxy, "BACKEND", "DEALER", "inproc://workers", NULL);
        zsock_wait(proxy);

	for (i = 0; i < threads; i++) {
                err = pthread_create(&tids[i], NULL, MsgThread, NULL);
                if (err < 0)
                {
                        fprintf(stderr, "WooFMsgServer: couldn't create thread %d\n", i);
                        exit(1);
                }
        }

	for(i=0; i < threads; i++) {
		pthread_join(tids[i],NULL);
	}


	return 0;
}

#endif


#ifdef CLIENT

struct arg_stc
{
	int tid;
	unsigned long *count;
	int max;
	char *ip;
	int port;
	pthread_mutex_t *lock;
};

pthread_mutex_t Lock;
unsigned long Counter;
int Max;
int Bytes;

typedef struct arg_stc TARG;
#define ARGS "a:t:p:m:q"
char *Usage = "zproxy-thread-client -a ip_addr_for_server -p port -t threads -m maxcount -q <quiet>\n";

int Quiet;

zmsg_t *ServerRequest(char *endpoint, zmsg_t *msg)
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
	if (server == NULL)
	{
		fprintf(stderr, "ServerRequest: no server connection to %s\n",
				endpoint);
		fflush(stderr);
		zmsg_destroy(&msg);
		return (NULL);
	}

	/*
	 * set up the poller for the reply
	 */
	resp_poll = zpoller_new(server, NULL);
	if (resp_poll == NULL)
	{
		fprintf(stderr, "ServerRequest: no poller for reply from %s\n",
				endpoint);
		fflush(stderr);
		zsock_destroy(&server);
		zmsg_destroy(&msg);
		return (NULL);
	}

	/*
	 * send the message to the server
	 */
	err = zmsg_send(&msg, server);
	if (err < 0)
	{
		fprintf(stderr, "ServerRequest: msg send to %s failed\n",
				endpoint);
		fflush(stderr);
		zsock_destroy(&server);
		zpoller_destroy(&resp_poll);
		zmsg_destroy(&msg);
		return (NULL);
	}

	/*
	 * wait for the reply, but not indefinitely
	 */
	server_resp = zpoller_wait(resp_poll, TIMEOUT);
	if (server_resp != NULL)
	{
		r_msg = zmsg_recv(server_resp);
		if (r_msg == NULL)
		{
			fprintf(stderr, "ServerRequest: msg recv from %s failed\n",
					endpoint);
			fflush(stderr);
			zsock_destroy(&server);
			zpoller_destroy(&resp_poll);
			return (NULL);
		}
		zsock_destroy(&server);
		zpoller_destroy(&resp_poll);
		return (r_msg);
	}
	if (zpoller_expired(resp_poll))
	{
		fprintf(stderr, "ServerRequest: msg recv timeout from %s after %d msec\n",
				endpoint, TIMEOUT);
		fflush(stderr);
		zsock_destroy(&server);
		zpoller_destroy(&resp_poll);
		return (NULL);
	}
	else if (zpoller_terminated(resp_poll))
	{
		fprintf(stderr, "ServerRequest: msg recv interrupted from %s\n",
				endpoint);
		fflush(stderr);
		zsock_destroy(&server);
		zpoller_destroy(&resp_poll);
		return (NULL);
	}
	else
	{
		fprintf(stderr, "ServerRequest: msg recv failed from %s\n",
				endpoint);
		fflush(stderr);
		zsock_destroy(&server);
		zpoller_destroy(&resp_poll);
		return (NULL);
	}
}

void *MsgThread(void *arg)
{
	TARG *ta = (TARG *)arg;
	char endpoint[255];
        int port;
        zmsg_t *msg;
        zmsg_t *r_msg;
        zframe_t *frame;
        zframe_t *r_frame;
        char buffer[255];
        char *str;
        unsigned long value;
	unsigned long r_val;
        int err;
	int i;
	int reqcnt;

	memset(endpoint, 0, sizeof(endpoint));
        sprintf(endpoint, ">tcp://%s:%d", ta->ip, ta->port);

	reqcnt = 0;
	for(i=0; i < ta->max; i++) {
		if(*(ta->count) == 0) {
			pthread_exit(NULL);
		}
		msg = zmsg_new();

		if (msg == NULL) 
		{
			fprintf(stderr, "MsgThread(%d): msg new %d failed\n",ta->tid,i);
			fflush(stderr);
			pthread_exit(NULL);
		}
		pthread_mutex_lock(ta->lock);
		value = (unsigned long)(*(ta->count));
		(*ta->count)--;
		pthread_mutex_unlock(ta->lock);

		memset(buffer, 0, sizeof(buffer));
        	sprintf(buffer, "%lu", value);
		if(Quiet == 0) {
			printf("MsgThread(%d) sending %s (%lu) len: %d\n",ta->tid,buffer,value,strlen(buffer));
			fflush(stdout);
		}
		frame = zframe_new(buffer, strlen(buffer));
		if (frame == NULL)
		{       
			fprintf(stderr, "MsgThred(%d): no frame for msg %d\n",
					ta->tid, i);
			fflush(stderr);
			zmsg_destroy(&msg);
			pthread_exit(NULL);
		}
	        err = zmsg_append(msg, &frame);
        	if (err < 0)
		{               
			fprintf(stderr, "MsgThread(%d): append frame to msg for %d failed\n",
					ta->tid,i);
			zframe_destroy(&frame);
			zmsg_destroy(&msg);
			pthread_exit(NULL);
		}
		r_msg = ServerRequest(endpoint, msg);
	        if (r_msg == NULL)
		{       
			fprintf(stderr, "MsgThread(%d): couldn't recv msg for %d\n",
				ta->tid, i);
			pthread_exit(NULL);
		}       
		r_frame = zmsg_first(r_msg);
                if (r_frame == NULL)
                {       
                        fprintf(stderr, "MsgThread(%d): no recv frame for %d\n",
                                        ta->tid, i);
                        zmsg_destroy(&r_msg);
			pthread_exit(NULL);
                }
                str = zframe_data(r_frame);
                r_val = strtoul(str, (char **)NULL, 10);
		if(r_val != value) {
			fprintf(stderr,"MsgThread(%d) %lu does not match %lu at %d\n",
				ta->tid,value,r_val,i);
                	zmsg_destroy(&r_msg);
			pthread_exit(NULL);
		}
		zmsg_destroy(&r_msg);
		reqcnt++;
	}
	if(Quiet == 0) {
		printf("thread: %d sent %d requests\n",ta->tid,reqcnt);
		fflush(stdout);
	}
	pthread_exit(NULL);
}

int main (int argc, char **argv)
{
	int c;
	int port;
        int err;
        char endpoint[255];
	char ip_addr[255];
        pthread_t *tids;
        int i;
	int threads;
	TARG *tas;
	struct timeval start;
	struct timeval end;
	double elapsed;


	memset(ip_addr,0,sizeof(ip_addr));
	threads = 0;
	port = 0;
	Max = 1;
	while((c=getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 't':
				threads = atoi(optarg);
				break;
			case 'p':
				port = atoi(optarg);
				break;
			case 'a':
				strcpy(ip_addr,optarg);
				break;
			case 'm':
				Max = atoi(optarg);
				break;
			case 'q':
				Quiet = 1;
				break;
			default:
				fprintf(stderr,
					"unrecognized argument %c\n",
						(char)c);
				fprintf(stderr,"%s",Usage);
				exit(1);
		}
	}

	if(threads == 0) {
		threads = 1;
	}
	if(port == 0) {
		port = 50000;
	}

	tids = (pthread_t *)malloc(threads*sizeof(pthread_t));
	if(tids == NULL) {
		exit(1);
	}
	tas = (TARG *)malloc(threads*sizeof(TARG));
	if(tas == NULL) {
		exit(1);
	}
	memset(tas,0,threads*sizeof(TARG));

	Counter = (unsigned long)(Max*threads);

	pthread_mutex_init(&Lock,NULL);
	gettimeofday(&start,NULL);
	for (i = 0; i < threads; i++) {
		tas[i].tid = i;
		tas[i].count = &Counter;
		tas[i].max = Max;
		tas[i].lock = &Lock;
		tas[i].ip = ip_addr;
		tas[i].port = port;

                err = pthread_create(&tids[i], NULL, MsgThread, (void *)&(tas[i]));
                if (err < 0)
                {
                        fprintf(stderr, "proxy-thread-client: couldn't create thread %d\n", i);
                        exit(1);
                }
        }

	for(i=0; i < threads; i++) {
		pthread_join(tids[i],NULL);
	}
	gettimeofday(&end,NULL);

	elapsed = ((double)end.tv_sec+(double)end.tv_usec/1000000.0) -
		  ((double)start.tv_sec+(double)start.tv_usec/1000000.0);

	Bytes = sizeof(unsigned long);
	if(Counter == 0) {
		printf("%f msgs %f bytes in %f secs is %f MB/s %f msg/s\n",
			(double)Max*threads,
			(double)(Max*threads*Bytes),
			elapsed,
			(double)(Max*threads*Bytes)/(1000000.0*elapsed),
			(double)(Max*threads)/elapsed);
		fflush(stdout);
	}


	return 0;
}

#endif


