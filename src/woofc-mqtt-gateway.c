#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <czmq.h>
pthread_mutex_t ELock; // for endpoint cache
#include "uriparser2.h"

#include "woofc.h" /* for WooFPut */
#include "woofc-access.h"
#include "woofc-mqtt.h"

#define WOOF_MQTT_MSG_THREADS (20)
#define WOOF_MQTT_MSG_REQ_TIMEOUT (10000)

void DeviceSend(char *s);

#define IPLEN (17)

char User_name[256];
char Password [256];
char Device_name_space[1024];
char Broker[IPLEN];
int Timeout;

struct timespec ardino_delay = {1,0}; /* 1/3 second for lwip tcp memory leak */
//#define PAUSE nanosleep(&ardino_delay,NULL)

#define DEFAULT_MQTT_TIMEOUT (3)


struct sem
{
        pthread_mutex_t lock;
        pthread_cond_t wait;
        int value;
	int waiters;
};

typedef struct sem sema;

sema *InitSemPT(int count)
{
	sema *s;

	s = (sema *)malloc(sizeof(sema));
	if(s == NULL) {
		return(NULL);
	}
	s->value = count;
	s->waiters = 0;
	pthread_cond_init(&(s->wait),NULL);
	pthread_mutex_init(&(s->lock),NULL);

	return(s);
}

void FreeSemPT(sema *s)
{
	free(s);
}

void PSemPT(sema *s)
{
	pthread_mutex_lock(&(s->lock));

	s->value--;

	while(s->value < 0) {
		/*
		 * maintain semaphore invariant
		 */
		if(s->waiters < (-1 * s->value)) {
			s->waiters++;
			pthread_cond_wait(&(s->wait),&(s->lock));
			s->waiters--;
		} else {
			break;
		}
	}

	pthread_mutex_unlock(&(s->lock));

	return;
}

void VSemPT(sema *s)
{
	
	pthread_mutex_lock(&(s->lock));

	s->value++;

	if(s->value <= 0)
	{
		pthread_cond_signal(&(s->wait));
	}

	pthread_mutex_unlock(&(s->lock));
}

struct resp_stc
{
	int msgid;
	sema *s;
	char *resp_string;
};

typedef struct resp_stc RESP;

pthread_mutex_t RLock;
RB *RespList;

RESP *MakeRESP(int msgid)
{
	RESP *r;

	r = (RESP *)malloc(sizeof(RESP));
	if(r == NULL) {
		return(NULL);
	}
	memset(r,0,sizeof(RESP));
	r->s = InitSemPT(0);
	if(r->s == NULL) {
		free(r);
		return(NULL);
	}
	r->msgid = msgid;
	pthread_mutex_lock(&RLock);
	RBInsertI(RespList,msgid,(Hval)(void *)r);
	pthread_mutex_unlock(&RLock);
	return(r);
}

void FreeRESP(RESP *r)
{
	RB *rb;
	if(r == NULL) {
		return;
	}
	if(r->resp_string != NULL) {
		free(r->resp_string);
	}
	pthread_mutex_lock(&RLock);
	rb = RBFindI(RespList,r->msgid);
	if(rb != NULL) {
		RBDeleteI(RespList,rb);
	}
	pthread_mutex_unlock(&RLock);
	FreeSemPT(r->s);
	free(r);
	return;
}



void FreeMsgList(char **msgs)
{
	int i;
	if(msgs == NULL) {
		return;
	}
	i = 0;
	while(msgs[i] != NULL) {
		free(msgs[i]);
		i++;
	}
	free(msgs);
	return;
}

int ExtractMsgID(char *str)
{
        char *curr;
        char *next;
        char buf[128];
        int max = sizeof(buf);
        int msgid;
        /*
         * msgID is third element
         */
        curr = strstr(str,"|");
        if(curr == NULL) {
                return(-1);
        }
        curr++;

        curr = strstr(curr,"|");

        if(curr == NULL) {
                return(-1);
        }
        curr++;

        /*
         * find the end
         */
        next = strstr(curr,"|");
        if(next == NULL) {
                /*
                 * if not found, the consider last part of string the msgid
                 */
                msgid = atoi(curr);
                return(msgid);
        }

        memset(buf,0,sizeof(buf));
        if((next - curr + 1) > max) {
                max = next - curr;
        }
        strncpy(buf,curr,max);
        buf[sizeof(buf)-1] = 0;
        msgid = atoi(buf);
        return(msgid);
}


/*
 * mosquitto could pack multiple messages into a single read()
 */
char **ExtractMsgList(char *msg)
{
	char *curr;
	char *next;
	int count;
	char **msgs;
	char *cmsg;
	int len;

	/*
	 * count the new lines
	 */
	count = 0;
	curr = msg;
	while(*curr != 0) {
		if(*curr == '\n') {
			count++;
		}
		curr++;
	}

	/*
	 * need count+ messages and 1 extra for NULL
	 */
	msgs = (char **)malloc((count+2)*sizeof(char *));
	if(msgs == NULL) {
		printf("no space for message list\n");
		return(NULL);
	}
	count = 0;
	curr = msg;
	next = strstr(curr,"\n");
	while(next != NULL) {
		len = next - curr + 1;
		cmsg = (char *)malloc(len);
		if(cmsg == NULL) {
			printf("no space for cmsg\n");
			FreeMsgList(msgs);
			return(NULL);
		}
		memset(cmsg,0,len);
		strncpy(cmsg,curr,len-1);
		msgs[count] = cmsg;
printf("extracted msg[%d]: %s\n",count,cmsg);
		curr = next+1;
		count++;
		next = strstr(curr,"\n");
	}
	msgs[count] = NULL;
	return(msgs);
}


RB *ServerEndpoints = NULL;

zmsg_t *WooFMQTTRequest(char *endpoint, zmsg_t *msg)
{
	static zsock_t *server;
	zpoller_t *resp_poll;
	zsock_t *server_resp;
	int err;
	zmsg_t *r_msg;
	RB *rb;
	char *ep;
	Hval hv;
	RB *my_endpoints;
	double tid = (double)pthread_self();
	
	ep = (char *)malloc(strlen(endpoint)+1);
	if(ep == NULL) {
		return(NULL);
	}
	memset(ep,0,strlen(endpoint)+1);
	strncpy((char *)ep,endpoint,strlen(endpoint));


	pthread_mutex_lock(&ELock);
	/*
	 * if threads map isn't initialized, initialize it
	 */
	if(ServerEndpoints == NULL) {
		ServerEndpoints = RBInitD();
		if(ServerEndpoints == NULL) {
			pthread_mutex_unlock(&ELock);
			return(NULL);
		}
		my_endpoints = RBInitS();
		if(my_endpoints == NULL) {
			pthread_mutex_unlock(&ELock);
			return(NULL);
		}
		RBInsertD(ServerEndpoints,tid,(Hval)((void *)my_endpoints));
//printf("inserted my_enpoints %p\n",my_endpoints);
	}
	/*
	 * if my RB map of endpoints isn't in the thread map, add it
	 */
	rb = RBFindD(ServerEndpoints,tid);
	if(rb == NULL) {
		my_endpoints = RBInitS();
//printf("Thread %f inited my_endpoints\n",tid);
		if(my_endpoints == NULL) {
			pthread_mutex_unlock(&ELock);
			return(NULL);
		}
//		hv.v = (void *)my_endpoints;
		RBInsertD(ServerEndpoints,tid,(Hval)((void *)my_endpoints));
	} else {
		my_endpoints = (RB *)rb->value.v;
//printf("found my_enpoints %p\n",my_endpoints);
	}
	pthread_mutex_unlock(&ELock);

	/*
	 * here my_endpoints is this thread's list of endpoints exclusively
	 */

//printf("WooFMQTTRequest (%f): find endpoint: %s (%d)\n",tid,ep,strlen(ep));
//printf("tring insert\n");
	rb = RBFindS(my_endpoints,ep);
	/*
	 * get a socket to the server
	 */
	if(rb == NULL) {
		server = zsock_new_req(ep);
		if (server == NULL)
		{
			fprintf(stderr, "WooFMQTTRequest: no server connection to %s\n",
					ep);
			fflush(stderr);
			zmsg_destroy(&msg);
			return (NULL);
		}
//		hv.v = (void *)server;
//printf("RBInsertS: %p %s %s\n",ep,ep,endpoint);
//printf("WooFMQTTRequest (%f): insert endpoint: %s (%d) ep: %s (%d)\n",tid,endpoint,strlen(endpoint),
//		ep,strlen(ep));
		RBInsertS(my_endpoints,ep,(Hval)(void *)server);
	} else {
		server = (zsock_t *)rb->value.v;
	}

	/*
	 * set up the poller for the reply
	 */
	resp_poll = zpoller_new(server, NULL);
	if (resp_poll == NULL)
	{
		fprintf(stderr, "WooFMQTTRequest: no poller for reply from %s\n",
				ep);
		fflush(stderr);
		zsock_destroy(&server);
		zmsg_destroy(&msg);
//printf("WooFMQTTRequest: find endpoint poller: %s (%d)\n",endpoint,strlen(endpoint));
		rb = RBFindS(my_endpoints,ep);
		if(rb != NULL) {
			RBDeleteS(my_endpoints,rb);
		}
		free(ep);
		return (NULL);
	}

	/*
	 * send the message to the server
	 */
	err = zmsg_send(&msg, server);
	if (err < 0)
	{
		fprintf(stderr, "WooFMQTTRequest: msg send to %s failed\n",
				endpoint);
		fflush(stderr);
		zsock_destroy(&server);
		zpoller_destroy(&resp_poll);
		zmsg_destroy(&msg);
//printf("WooFMQTTRequest: find endpoint send: %s (%d)\n",endpoint,strlen(endpoint));
		rb = RBFindS(my_endpoints,ep);
		if(rb != NULL) {
			RBDeleteS(my_endpoints,rb);
		}
		free(ep);
		return (NULL);
	}

	/*
	 * wait for the reply, but not indefinitely
	 */
	server_resp = (zsock_t *)zpoller_wait(resp_poll, WOOF_MQTT_MSG_REQ_TIMEOUT);
	if (server_resp != NULL)
	{
		r_msg = zmsg_recv(server_resp);
		if (r_msg == NULL)
		{
			fprintf(stderr, "WooFMQTTRequest: msg recv from %s failed\n",
					endpoint);
			fflush(stderr);
			zsock_destroy(&server);
			zpoller_destroy(&resp_poll);
//printf("WooFMQTTRequest: find endpoint wait: %s (%d)\n",endpoint,strlen(endpoint));
			rb = RBFindS(my_endpoints,ep);
			if(rb != NULL) {
				RBDeleteS(my_endpoints,rb);
			}
			free(ep);
			return (NULL);
		}
		//zsock_destroy(&server);
		zpoller_destroy(&resp_poll);
		return (r_msg);
	}
	if (zpoller_expired(resp_poll))
	{
		fprintf(stderr, "WooFMQTTRequest: msg recv timeout from %s after %d msec\n",
				endpoint, WOOF_MQTT_MSG_REQ_TIMEOUT);
		fflush(stderr);
		zsock_destroy(&server);
		zpoller_destroy(&resp_poll);
//printf("WooFMQTTRequest: find endpoint expired: %s (%d)\n",endpoint,strlen(endpoint));
		rb = RBFindS(my_endpoints,ep);
		if(rb != NULL) {
			RBDeleteS(my_endpoints,rb);
		}
		free(ep);
		return (NULL);
	}
	else if (zpoller_terminated(resp_poll))
	{
		fprintf(stderr, "WooFMQTTRequest: msg recv interrupted from %s\n",
				endpoint);
		fflush(stderr);
		zsock_destroy(&server);
		zpoller_destroy(&resp_poll);
//printf("WooFMQTTRequest: find endpoint terminated: %s (%d)\n",endpoint,strlen(endpoint));
		rb = RBFindS(my_endpoints,ep);
		if(rb != NULL) {
			RBDeleteS(my_endpoints,rb);
		}
		free(ep);
		return (NULL);
	}
	else
	{
		fprintf(stderr, "WooFMQTTRequest: msg recv failed from %s\n",
				endpoint);
		fflush(stderr);
		zsock_destroy(&server);
		zpoller_destroy(&resp_poll);
//printf("WooFMQTTRequest: find endpoint failed: %s (%d)\n",endpoint,strlen(endpoint));
		rb = RBFindS(my_endpoints,ep);
		if(rb != NULL) {
			RBDeleteS(my_endpoints,rb);
		}
		free(ep);
		return (NULL);
	}
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
	unsigned long seq_no;
	char buffer[255];
	int err;
	char sub_string[16*1024];
	char pub_string[16*1024];
	char resp_string[2048];
	FILE *fd;
	char *element_string;
	int msgid;
	int s;
	char *curr;

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
	if (frame == NULL)
	{
		perror("WooFProcessPut: couldn't set cursor in msg");
		return;
	}

	frame = zmsg_next(req_msg);
	if (frame == NULL)
	{
		perror("WooFProcessPut: couldn't find woof_name in msg");
		return;
	}
	/*
	 * woof_name in the first frame
	 */
	memset(woof_name, 0, sizeof(woof_name));
	str = (char *)zframe_data(frame);
	copy_size = zframe_size(frame);
	if (copy_size > (sizeof(woof_name) - 1))
	{
		copy_size = sizeof(woof_name) - 1;
	}
	strncpy(woof_name, str, copy_size);

#ifdef DEBUG
	printf("WooFProcessPut: received woof_name %s\n", woof_name);
	fflush(stdout);
#endif
	/*
	 * handler name in the second frame
	 */
	memset(hand_name, 0, sizeof(hand_name));
	frame = zmsg_next(req_msg);
	copy_size = zframe_size(frame);
	/*
	 * could be zero if there is no handler
	 */
	if (copy_size > 0)
	{
		str = (char *)zframe_data(frame);
		if (copy_size > (sizeof(hand_name) - 1))
		{
			copy_size = sizeof(hand_name) - 1;
		}
		strncpy(hand_name, str, copy_size);
	}

#ifdef DEBUG
	printf("WooFProcessPut: received handler name %s\n", hand_name);
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
	if (element == NULL)
	{ /* element too big */
		fprintf(stderr, "WooFMsgProcessPut: woof_name: %s, handler: %s, size %u too big\n",
				woof_name, hand_name, copy_size);
		fflush(stderr);
		seq_no = -1;
	}
	else
	{
		str = (char *)zframe_data(frame);
		memcpy(element, (void *)str, copy_size);
#ifdef DEBUG
		printf("WooFProcessPut: received %lu bytes for the element\n", copy_size);
		fflush(stdout);
#endif
printf("WooFProcessPut: called on %s with size %d\n",woof_name,copy_size);

		/*
		 * FIX ME: in a cloud, the calling process doesn't know the publically viewable
		 * IP address so it can't determine whether the access is local or not.
		 *
		 * For now, assume that all Process functions convert to a local access
		memset(local_name, 0, sizeof(local_name));
		err = WooFLocalName(woof_name, local_name, sizeof(local_name));
		if (err < 0)
		{
			strncpy(local_name, woof_name, sizeof(local_name));
		}
		 */

		element_string = (char *)malloc(copy_size*2+1); 
		if(element_string == NULL) {
			fprintf(stderr,"WooFProcessPut: not enough space to malloc %d for put element string\n",2*copy_size+1);
			fflush(stderr);
			free(element);
			seq_no = -1;
			goto out;
		}
		memset(element_string,0,copy_size*2+1);
		ConvertBinarytoASCII(element_string,element,copy_size);

		/*
		 * choose random number for inbound msgid
		 */
		msgid = (int)(rand());

		/*
		 * use msgid to get back specific response
		 */
#ifdef FP
		memset(sub_string,0,sizeof(sub_string));
		sprintf(sub_string,"/usr/bin/mosquitto_sub -c -i %d -q 0 -W %d -C 1 -h %s -t %s.%d -u \'%s\' -P \'%s\'",
				msgid,
				Timeout,
				Broker,
				Device_name_space,
				msgid,
				User_name,
				Password);
printf("sub_string: %s\n",sub_string);
		fd = popen(sub_string,"r");
		if(fd == NULL) {
			fprintf(stderr,"WooFProcessPut: open for %s failed\n",sub_string);
			free(element);
			free(element_string);
			seq_no = -1;
			goto out;
		}
#else
		RESP *rs;
		rs = MakeRESP(msgid);
		if(rs == NULL) {
			fprintf(stderr,"WooFProcessPut: open for %s failed\n",sub_string);
			free(element);
			free(element_string);
			seq_no = -1;
			goto out;
		}
		
#endif
		/*
		 * create the mqtt message to put to the device
		 */
		memset(pub_string,0,sizeof(pub_string));
		if(hand_name[0] == 0) {
			strncpy(hand_name,"NULL",sizeof(hand_name));
		} 
#ifdef FP
		sprintf(pub_string,"/usr/bin/mosquitto_pub -q 0 -h %s -t %s.input -u \'%s\' -P \'%s\' -m \'%s|%d|%d|%s|%s\'",
				Broker,
				Device_name_space, 
				User_name,
				Password,
				woof_name,
				WOOF_MQTT_PUT,
				msgid,
				hand_name,
				element_string);
printf("WooFProcessPut: sending %s to device\n",pub_string);
		system(pub_string);
		memset(resp_string,0,sizeof(resp_string));
		s = read(fileno(fd),resp_string,sizeof(resp_string));
		if(s <= 0) {
			fprintf(stderr,"WooFProcessPut: no resp string\n");
			seq_no = -1;
			pclose(fd);
			goto out;
		}
		pclose(fd);
#else
		sprintf(pub_string,"%s|%d|%d|%s|%s\n",
				woof_name,
				WOOF_MQTT_PUT,
				msgid,
				hand_name,
				element_string);
		DeviceSend(pub_string);
printf("WooFProcessPut: sending %s to device\n",pub_string);
printf("WooFProcessPut: calling P on %d\n",msgid);
		PSemPT(rs->s);
printf("WooFProcessPut: awake on %d\n",msgid);
		strncpy(resp_string,rs->resp_string,sizeof(resp_string));
		FreeRESP(rs);
#endif

		free(element);
		free(element_string);

printf("WooFProcessPut resp string FROM DEVICE: %s\n",resp_string);
		/*
		 * resp should be 
		 * woof_name|WOOF_MQTT_PUT_RESP|msgid|seqno
		 */
		curr = strstr(resp_string,"|"); // skip woof name
		if(curr == NULL) {
			seq_no = -1;
			goto out;
		}
		curr++;
		curr = strstr(curr,"|"); // skip resp code
		if(curr == NULL) {
			seq_no = -1;
			goto out;
		}
		curr++;
		curr = strstr(curr,"|"); // skip msgid
		if(curr == NULL) {
			seq_no = -1;
			goto out;
		}
		curr++;
		seq_no = atoi(curr);

	}

out:
	/*
	 * send seq_no back
	 */
	r_msg = zmsg_new();
	if (r_msg == NULL)
	{
		perror("WooFProcessPut: no reply message");
		return;
	}
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%lu", seq_no);
	r_frame = zframe_new(buffer, strlen(buffer));
	if (r_frame == NULL)
	{
		perror("WooFProcessPut: no reply frame");
		zmsg_destroy(&r_msg);
		return;
	}
	err = zmsg_append(r_msg, &r_frame);
	if (err != 0)
	{
		perror("WooFProcessPut: couldn't append to r_msg");
		zframe_destroy(&r_frame);
		zmsg_destroy(&r_msg);
		return;
	}
	err = zmsg_send(&r_msg, receiver);
	if (err != 0)
	{
		perror("WooFProcessPut: couldn't send r_msg");
		zmsg_destroy(&r_msg);
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
	unsigned int el_size;
	char buffer[255];
	int err;
	char sub_string[16*1024];
	char pub_string[16*1024];
	char resp_string[2048];
	FILE *fd;
	int msgid;
	int s;
	char *curr;

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
	if (frame == NULL)
	{
		perror("WooFProcessGetElSize: couldn't set cursor in msg");
		return;
	}

	frame = zmsg_next(req_msg);
	if (frame == NULL)
	{
		perror("WooFProcessGetElSize: couldn't find woof_name in msg");
		return;
	}
	/*
	 * woof_name in the first frame
	 */
	memset(woof_name, 0, sizeof(woof_name));
	str = (char *)zframe_data(frame);
	copy_size = zframe_size(frame);
	if (copy_size > (sizeof(woof_name) - 1))
	{
		copy_size = sizeof(woof_name) - 1;
	}
	strncpy(woof_name, str, copy_size);

	/*
	 * choose random number for inbound msgid
	 */
	msgid = (int)(rand());

	/*
	 * use msgid to get back specific response
	 */
#ifdef FP
	memset(sub_string,0,sizeof(sub_string));
	sprintf(sub_string,"/usr/bin/mosquitto_sub -c -i %d -q 0 -W %d -C 1 -h %s -t %s.%d -u \'%s\' -P \'%s\'",
			msgid,
			Timeout,
			Broker,
			Device_name_space,
			msgid,
			User_name,
			Password);
printf("sub_string: %s\n",sub_string);
	fd = popen(sub_string,"r");
	if(fd == NULL) {
		fprintf(stderr,"WooFProcessGetElSize: open for %s failed\n",sub_string);
		el_size = -1;
		goto out;
	}
#else
	RESP *rs;
	rs = MakeRESP(msgid);
	if(rs == NULL) {
		fprintf(stderr,"WooFProcessGetElSize: open for %s failed\n",sub_string);
		el_size = -1;
	}
#endif
		/*
		 * create the mqtt message to put to the device
		 */
	memset(pub_string,0,sizeof(pub_string));
#ifdef FP
	sprintf(pub_string,"/usr/bin/mosquitto_pub -q 0 -h %s -t %s.input -u \'%s\' -P \'%s\' -m \'%s|%d|%d\'",
		Broker,
		Device_name_space, 
		User_name,
		Password,
		woof_name,
		WOOF_MQTT_GET_EL_SIZE,
		msgid);
printf("WooFProcessGetElSize: sending %s to device\n",pub_string);
	system(pub_string);
	memset(resp_string,0,sizeof(resp_string));
	s = read(fileno(fd),resp_string,sizeof(resp_string));
	if(s <= 0) {
		fprintf(stderr,"WooFProcessElSize: no resp string\n");
		el_size = -1;
		pclose(fd);
		goto out;
	}
	pclose(fd);
#else
	sprintf(pub_string,"%s|%d|%d\n",
		woof_name,
		WOOF_MQTT_GET_EL_SIZE,
		msgid);
	DeviceSend(pub_string);
printf("WooFProcessGetElSize: sending %s to device\n",pub_string);
printf("WooFProcessGetElSize: calling P on %d\n",msgid);
	PSemPT(rs->s);
printf("WooFProcessGetElSize: awake on %d\n",msgid);
	strncpy(resp_string,rs->resp_string,sizeof(resp_string));
	FreeRESP(rs);
#endif

printf("WooFProcessGetElSize resp string FROM DEVICE: %s\n",resp_string);
	/*
	 * resp should be 
	 * woof_name|WOOF_MQTT_GET_EL_SIZE_RESP|msgid|size
	 */
	curr = strstr(resp_string,"|"); // skip woof name
	if(curr == NULL) {
		el_size = -1;
		goto out;
	}
	curr++;
	curr = strstr(curr,"|"); // skip resp code
	if(curr == NULL) {
		el_size = -1;
		goto out;
	}
	curr++;
	curr = strstr(curr,"|"); // skip msgid
	if(curr == NULL) {
		el_size = -1;
		goto out;
	}
	curr++;
	el_size = atoi(curr);

#ifdef DEBUG
	printf("WooFProcessGetElSize: woof_name %s has element size: %u\n", woof_name, el_size);
	fflush(stdout);
#endif

out:
	/*
	 * send el_size back
	 */
	r_msg = zmsg_new();
	if (r_msg == NULL)
	{
		perror("WooFProcessGetElSize: no reply message");
		return;
	}
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%lu", el_size);
printf("WooFProcesGetElSize: woof: %s sending back %s (%lu)\n",woof_name,buffer,el_size);
	r_frame = zframe_new(buffer, strlen(buffer));
	if (r_frame == NULL)
	{
		perror("WooFProcessGetElSize: no reply frame");
		zmsg_destroy(&r_msg);
		return;
	}
	err = zmsg_append(r_msg, &r_frame);
	if (err != 0)
	{
		perror("WooFProcessPut: couldn't append to r_msg");
		zframe_destroy(&r_frame);
		zmsg_destroy(&r_msg);
		return;
	}
	err = zmsg_send(&r_msg, receiver);
	if (err != 0)
	{
		perror("WooFProcessGetElSize: couldn't send r_msg");
		zmsg_destroy(&r_msg);
		return;
	}


	return;
}

void WooFProcessGetLatestSeqno(zmsg_t *req_msg, zsock_t *receiver)
{
	zmsg_t *r_msg;
	zframe_t *frame;
	zframe_t *r_frame;
	char *str;
	char woof_name[2048];
	unsigned int copy_size;
	unsigned long latest_seq_no;
	char buffer[255];
	int err;
	char sub_string[16*1024];
	char pub_string[16*1024];
	char resp_string[2048];
	FILE *fd;
	int msgid;
	int s;
	char *curr;

#ifdef DEBUG
	printf("WooFProcessGetLatestSeqno: called\n");
	fflush(stdout);
#endif
	/*
	 * WooFLatestSeqno requires a woof_name
	 *
	 * first frame is the message type
	 */
	frame = zmsg_first(req_msg);
	if (frame == NULL)
	{
		perror("WooFProcessGetLatestSeqno: couldn't set cursor in msg");
		return;
	}

	frame = zmsg_next(req_msg);
	if (frame == NULL)
	{
		perror("WooFProcessGetLatestSeqno: couldn't find woof_name in msg");
		return;
	}
	/*
	 * woof_name in the first frame
	 */
	memset(woof_name, 0, sizeof(woof_name));
	str = (char *)zframe_data(frame);
	copy_size = zframe_size(frame);
	if (copy_size > (sizeof(woof_name) - 1))
	{
		copy_size = sizeof(woof_name) - 1;
	}
	strncpy(woof_name, str, copy_size);

	/*
	 * choose random number for inbound msgid
	 */
	msgid = (int)(rand());

	/*
	 * use msgid to get back specific response
	 */
#ifdef FP
	memset(sub_string,0,sizeof(sub_string));
	sprintf(sub_string,"/usr/bin/mosquitto_sub -c -i %d -q 0 -W %d -C 1 -h %s -t %s.%d -u \'%s\' -P \'%s\'",
			msgid,
			Timeout,
			Broker,
			Device_name_space,
			msgid,
			User_name,
			Password);
printf("sub_string: %s\n",sub_string);
	fd = popen(sub_string,"r");
	if(fd == NULL) {
		fprintf(stderr,"WooFProcessGetElSize: open for %s failed\n",sub_string);
		latest_seq_no = -1;
		goto out;
	}
#else
	RESP *rs;
	rs = MakeRESP(msgid);
	if(rs == NULL) {
		fprintf(stderr,"WooFProcessGetElSize: open for %s failed\n",sub_string);
		latest_seq_no = -1;
		goto out;
	}
#endif
		/*
		 * create the mqtt message to put to the device
		 */
	memset(pub_string,0,sizeof(pub_string));
#ifdef FP
	sprintf(pub_string,"/usr/bin/mosquitto_pub -q 0 -h %s -t %s.input -u \'%s\' -P \'%s\' -m \'%s|%d|%d\'",
		Broker,
		Device_name_space, 
		User_name,
		Password,
		woof_name,
		WOOF_MQTT_GET_LATEST_SEQNO,
		msgid);
printf("WooFProcessGetLatestSeqno: sending %s to device\n",pub_string);
	system(pub_string);
	memset(resp_string,0,sizeof(resp_string));
	s = read(fileno(fd),resp_string,sizeof(resp_string));
	if(s <= 0) {
		fprintf(stderr,"WooFProcessGetlatestSeqno: no resp string\n");
			latest_seq_no = -1;
			pclose(fd);
			goto out;
	}
	pclose(fd);
#else
	sprintf(pub_string,"%s|%d|%d\n",
		woof_name,
		WOOF_MQTT_GET_LATEST_SEQNO,
		msgid);
	DeviceSend(pub_string);
printf("WooFProcessGetLatestSeqno: sending %s to device\n",pub_string);
printf("WooFProcessGetLatestSeqno: calling P on %d\n",msgid);
	PSemPT(rs->s);
printf("WooFProcessGetLatestSeqno: awake on %d\n",msgid);
	strncpy(resp_string,rs->resp_string,sizeof(resp_string));
	FreeRESP(rs);
#endif
printf("WooFProcessGetLatestSeqno resp string FROM DEVICE: %s\n",resp_string);
	/*
	 * resp should be 
	 * woof_name|WOOF_MQTT_GET_LATEST_SEQNO_RESP|msgid|latest_seqno
	 */
	curr = strstr(resp_string,"|"); // skip woof name
	if(curr == NULL) {
		latest_seq_no = -1;
		goto out;
	}
	curr++;
	curr = strstr(curr,"|"); // skip resp code
	if(curr == NULL) {
		latest_seq_no = -1;
		goto out;
	}
	curr++;
	curr = strstr(curr,"|"); // skip msgid
	if(curr == NULL) {
		latest_seq_no = -1;
		goto out;
	}
	curr++;
	latest_seq_no = atoi(curr);

out:
#ifdef DEBUG
	printf("WooFProcessGetLatestSeqno: woof_name %s has latest seq_no: %lu\n", woof_name, latest_seq_no);
	fflush(stdout);
#endif

	/*
	 * send latest_seq_no back
	 */
	r_msg = zmsg_new();
	if (r_msg == NULL)
	{
		perror("WooFProcessGetLatestSeqno: no reply message");
		return;
	}
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%lu", latest_seq_no);
printf("WooFProcesGetLatestSeqno: woof: %s sending back %s (%lu)\n",woof_name,buffer,latest_seq_no);
	r_frame = zframe_new(buffer, strlen(buffer));
	if (r_frame == NULL)
	{
		perror("WooFProcessGetLatestSeqno: no reply frame");
		zmsg_destroy(&r_msg);
		return;
	}
	err = zmsg_append(r_msg, &r_frame);
	if (err != 0)
	{
		perror("WooFProcessPut: couldn't append to r_msg");
		zframe_destroy(&r_frame);
		zmsg_destroy(&r_msg);
		return;
	}
	err = zmsg_send(&r_msg, receiver);
	if (err != 0)
	{
		perror("WooFProcessGetLatestSeqno: couldn't send r_msg");
		zmsg_destroy(&r_msg);
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
	char woof_name[1024];
	unsigned int copy_size;
	void *element;
	unsigned long el_size;
	unsigned long seq_no;
	int err;
	char sub_string[3*1024];
	char pub_string[3*1024];
	char resp_string[2*1024];
	FILE *fd;
	int msgid;
	int s;
	char *curr;
	char *next;
	char lsize_buf[50];

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
	if (frame == NULL)
	{
		perror("WooFProcessGet: couldn't set cursor in msg");
		return;
	}

	frame = zmsg_next(req_msg);
	if (frame == NULL)
	{
		perror("WooFProcessGet: couldn't find woof_name in msg");
		return;
	}
	/*
	 * woof_name in the first frame
	 */
	memset(woof_name, 0, sizeof(woof_name));
	str = (char *)zframe_data(frame);
	copy_size = zframe_size(frame);
	if (copy_size > (sizeof(woof_name) - 1))
	{
		copy_size = sizeof(woof_name) - 1;
	}
	strncpy(woof_name, str, copy_size);

#ifdef DEBUG
	printf("WooFProcessGet: received woof_name %s\n", woof_name);
	fflush(stdout);
#endif
	/*
	 * seq_no name in the second frame
	 */
	frame = zmsg_next(req_msg);
	copy_size = zframe_size(frame);
	if (copy_size > 0)
	{
		seq_no = strtoul((const char *)zframe_data(frame), (char **)NULL, 10);
#ifdef DEBUG
		printf("WooFProcessGet: received seq_no name %lu\n", seq_no);
		fflush(stdout);
#endif
printf("WooFProcessGet: called on %s for seqno %lu\n",woof_name,seq_no);
		/*
		 * create sub for response
		 */
		msgid = rand();
#ifdef FP
		memset(sub_string,0,sizeof(sub_string));
		sprintf(sub_string,"/usr/bin/mosquitto_sub -c -i %d -q 0 -W %d -C 1 -h %s -t %s.%d -u \'%s\' -P \'%s\'",
				msgid,
				Timeout,
				Broker,
				Device_name_space,
				msgid,
				User_name,
				Password);
printf("sub_string: %s\n",sub_string);
		fd = popen(sub_string,"r");
		if(fd == NULL) {
			fprintf(stderr,"WooFProcessGet: open for %s failed\n",sub_string);
			element = NULL;
			el_size = 0;
			goto out;
		}
#else
		RESP *rs;
		rs = MakeRESP(msgid);
		if(rs == NULL) {
			fprintf(stderr,"WooFProcessGet: open for %s failed\n",sub_string);
			element = NULL;
			el_size = 0;
			goto out;
		}
#endif

		/*
		 * request the Get
		 */
		memset(pub_string,0,sizeof(pub_string));
#ifdef FP
		sprintf(pub_string,"/usr/bin/mosquitto_pub -q 0 -h %s -t %s.input -u \'%s\' -P \'%s\' -m \'%s|%d|%d|%d\'",
				Broker,
				Device_name_space, 
				User_name,
				Password,
				woof_name,
				WOOF_MQTT_GET,
				msgid,
				(int)seq_no);

printf("WooFProcessGet: send %s to device\n",pub_string);
//printf("pub_string: %s\n",pub_string);
		system(pub_string);
		/*
		 * read the response
		 */
		memset(resp_string,0,sizeof(resp_string));
		s = read(fileno(fd),resp_string,sizeof(resp_string));
		if(s <= 0) {
			fprintf(stderr,"WooFProcessGet: no resp string\n");
			element = NULL;
			el_size = 0;
			pclose(fd);
			goto out;
		}
		pclose(fd);
#else
		sprintf(pub_string,"%s|%d|%d|%d\n",
				woof_name,
				WOOF_MQTT_GET,
				msgid,
				(int)seq_no);
		DeviceSend(pub_string);
printf("WooFProcessGetLatest: calling P on %d\n",msgid);
		PSemPT(rs->s);
printf("WooFProcessGetLatest: awake on %d\n",msgid);
		strncpy(resp_string,rs->resp_string,sizeof(resp_string));
		FreeRESP(rs);
printf("WooFProcessGet: send %s to device\n",pub_string);
#endif

printf("WooFProcessGet resp_string FROM DEVICE: %s\n",resp_string);
		/*
		 * format is
		 * woof_name | code | msgid | size | ASCII contents
		 * FIXME: could sanity check here to make sure we have a GET_RESP
		 */
		curr = strstr(resp_string,"|");
		if(curr == NULL) {
			fprintf(stderr,"no code delim in %s\n",resp_string);
			element = NULL;
			el_size = 0;
			goto out;
		}
		curr++;
		curr = strstr(curr,"|");
		if(curr == NULL) {
			fprintf(stderr,"no msgid delim in %s\n",resp_string);
			element = NULL;
			el_size = 0;
			goto out;
		}
		curr++;
		curr = strstr(curr,"|");
		if(curr == NULL) {
			fprintf(stderr,"no size delim in %s\n",resp_string);
			element = NULL;
			el_size = 0;
			goto out;
		}
		curr++;
		next = strstr(curr,"|");
		if(next == NULL) {
			fprintf(stderr,"no payload delim in %s\n",resp_string);
			element = NULL;
			el_size = 0;
			goto out;
		}
		/*
		 * copy out the element size
		 */
		memset(lsize_buf,0,sizeof(lsize_buf));
		strncpy(lsize_buf,curr,(next - curr));
		el_size = atoi(lsize_buf);
		curr = next + 1;

		element = malloc(el_size);
		if(element == NULL) {
			fprintf(stderr,"WooFProcessGet: no space\n");
			el_size = 0;
			goto out;
		}
printf("WoofProcessGet: created element size %d\n",el_size);
		ConvertASCIItoBinary((unsigned char *)element,curr,el_size);
printf("WoofProcessGet: converted %s to binary\n",curr);
	}
	else
	{ /* copy_size <= 0 */
		fprintf(stderr, "WooFProcessGet: no seq_no frame data for %s\n",
				woof_name);
		fflush(stderr);
		element = NULL;
		el_size = 0;
	}

out:
	/*
	 * send element back or empty frame indicating no dice
	 */
	r_msg = zmsg_new();
	if (r_msg == NULL)
	{
		perror("WooFProcessGet: no reply message");
		if (element != NULL)
		{
			free(element);
		}
		return;
	}
	if (element != NULL)
	{
#ifdef DEBUG
		printf("WooFProcessGet: creating frame for %lu bytes of element at seq_no %lu\n",
			   el_size, seq_no);
		fflush(stdout);
#endif

		r_frame = zframe_new(element, el_size);
		if (r_frame == NULL)
		{
			perror("WooFProcessGet: no reply frame");
			free(element);
			zmsg_destroy(&r_msg);
			return;
		}
	}
	else
	{
#ifdef DEBUG
		printf("WooFProcessGet: creating empty frame\n");
		fflush(stdout);
#endif
		r_frame = zframe_new_empty();
		if (r_frame == NULL)
		{
			perror("WooFProcessGet: no empty reply frame");
			zmsg_destroy(&r_msg);
			return;
		}
	}
	err = zmsg_append(r_msg, &r_frame);
	if (element != NULL)
	{
		free(element);
	}
	if (err != 0)
	{
		perror("WooFProcessGet: couldn't append to r_msg");
		zframe_destroy(&r_frame);
		zmsg_destroy(&r_msg);
		return;
	}
	err = zmsg_send(&r_msg, receiver);
	if (err != 0)
	{
		perror("WooFProcessGet: couldn't send r_msg");
		zmsg_destroy(&r_msg);
		return;
	}

	return;
}

#define DEBUG
void *WooFMsgThread(void *arg)
{
	zsock_t *receiver;
	zmsg_t *msg;
	zframe_t *frame;
	unsigned long tag;
	char *str;

	/*
	 * right now, we use req-rep pattern from zeromq.  need a way to timeout, however, as this
	 * pattern blocks indefinitely on network partition
	 */

	/*
	 * create a reply zsock and connect it to the back end of the proxy in the msg server
	 */
again:
	receiver = zsock_new_rep(">inproc://workers");
	if (receiver == NULL)
	{
		perror("WooFMsgThread: couldn't open receiver");
		pthread_exit(NULL);
	}

#ifdef DEBUG
	printf("WooFMsgThread: about to call receive\n");
	fflush(stdout);
#endif
	msg = zmsg_recv(receiver);
	while (msg != NULL)
	{
#ifdef DEBUG
		printf("WooFMsgThread: received\n");
		fflush(stdout);
#endif
		/*
		 * woofmsg starts with a message tag for dispatch
		 */
		frame = zmsg_first(msg);
		if (frame == NULL)
		{
			perror("WooFMsgThread: couldn't get tag");
			exit(1);
		}
		/*
		 * tag in the first frame
		 */
		str = (char *)zframe_data(frame);
		str[1] = 0;
		tag = strtoul(str, (char **)NULL, 10);
#ifdef DEBUG
		printf("WooFMsgThread: processing msg with tag: %d\n", tag);
		fflush(stdout);
#endif

		switch (tag)
		{
		case WOOF_MSG_PUT:
			WooFProcessPut(msg, receiver);
			break;
		case WOOF_MSG_GET_EL_SIZE:
			WooFProcessGetElSize(msg, receiver);
			break;
		case WOOF_MSG_GET:
			WooFProcessGet(msg, receiver);
			break;
//		case WOOF_MSG_GET_tail:
//			WooFProcessGettail(msg, receiver);
//			break;
		case WOOF_MSG_GET_LATEST_SEQNO:
			WooFProcessGetLatestSeqno(msg, receiver);
			break;
		default:
			fprintf(stderr, "WooFMsgThread: unknown tag %s\n",
					str);
			break;
		}
		/*
		 * process routines do not destroy request msg
		 */
		zmsg_destroy(&msg);

		/*
		 * wait for next request
		 */
		msg = zmsg_recv(receiver);
	}
printf("WooFMsgThread: STARTING OVER on NULL msg\n");

	zsock_destroy(&receiver);
goto again;
	pthread_exit(NULL);
} 


#undef DEBUG

int WooFMsgServerMQTT(const char *wnamespace)
{

	int port;
	zactor_t *proxy;
	int err;
	char endpoint[255];
	pthread_t tids[WOOF_MSG_THREADS];
	int i;


	if (wnamespace[0] == 0)
	{
		fprintf(stderr, "WooFMsgServer: couldn't find namespace\n");
		fflush(stderr);
		exit(1);
	}

#ifdef DEBUG
	printf("WooFMsgServer: started for namespace %s\n", wnamespace);
	fflush(stdout);
#endif

	/*
	 * set up the front end router socket
	 */
	memset(endpoint, 0, sizeof(endpoint));

	port = WooFPortHash(wnamespace);

	/*
	 * listen to any tcp address on port hash of namespace
	 */
	sprintf(endpoint, "tcp://*:%d", port);

#ifdef DEBUG
	printf("WooFMsgServer: fontend at %s\n", endpoint);
	fflush(stdout);
#endif

	/*
	 * create zproxy actor
	 */
	proxy = zactor_new(zproxy, NULL);
	if (proxy == NULL)
	{
		perror("WooFMsgServer: couldn't create proxy");
		exit(1);
	}

	/*
	 * create and bind endpoint with port has to frontend zsock
	 */
	zstr_sendx(proxy, "FRONTEND", "ROUTER", endpoint, NULL);
	zsock_wait(proxy);

	/*
	 * inproc:// is a process internal enpoint for zeromq
	 *
	 * if backend isn't in this process, this endpoint will need to be
	 * some kind of ipc endpoit.  for now, assume it is within the process
	 * and handled by threads
	 */
	zstr_sendx(proxy, "BACKEND", "DEALER", "inproc://workers", NULL);
	zsock_wait(proxy);

	/*
	 * create a single thread for now.  the dealer pattern can handle multiple threads, however
	 * so this can be increased if need be
	 */
	pthread_mutex_init(&ELock,NULL); // init global lock for thread map
	for (i = 0; i < WOOF_MSG_THREADS; i++)
	{
		err = pthread_create(&tids[i], NULL, WooFMsgThread, NULL);
		if (err < 0)
		{
			fprintf(stderr, "WooFMsgServer: couldn't create thread %d\n", i);
			exit(1);
		}
	}

	/*
	 * right now, there is no way for these threads to exit so the msg server will block
	 * indefinitely in this join
	 */
	for (i = 0; i < WOOF_MSG_THREADS; i++)
	{
		pthread_join(tids[i], NULL);
	}

	/*
	 * we'll get here if the msg thread is ever pthread_canceled()
	 */
	zactor_destroy(&proxy);

	exit(0);
}

unsigned long WooFMQTTMsgGetElSize(char *woof_name)
{
	char endpoint[255];
	char wnamespace[2048];
	char ip_str[25];
	int port;
	zmsg_t *msg;
	zmsg_t *r_msg;
	zframe_t *frame;
	zframe_t *r_frame;
	char buffer[255];
	unsigned char *str;
	unsigned long el_size;
	int err;
#ifdef woofcache
	unsigned long *el_size_cached;
#endif

#ifdef woofcache
	if (woof_cache == NULL)
	{
		woof_cache = woofcacheinit(woof_msg_cache_size);
		if (woof_cache == NULL)
		{
			fprintf(stderr, "WooFMQTTMsgGetElSize: woof: %s cache init failed\n",
					woof_name);
			fflush(stderr);
			return (-1);
		}
	}

	el_size_cached = (unsigned long *)woofcachefind(woof_cache, woof_name);
	if (el_size_cached != NULL)
	{
#ifdef DEBUG
		fprintf(stdout, "WooFMQTTMsgGetElSize: found %lu size for %s\n",
				*el_size_cached, woof_name);
		fflush(stdout);
#endif
		return (*el_size_cached);
	}
#endif

	memset(wnamespace, 0, sizeof(wnamespace));
	err = WooFNameSpaceFromURI(woof_name, wnamespace, sizeof(wnamespace));
	if (err < 0)
	{
		fprintf(stderr, "WooFMQTTMsgGetElSize: woof: %s no name space\n",
				woof_name);
		fflush(stderr);
		return (-1);
	}

	memset(ip_str, 0, sizeof(ip_str));
	err = WooFIPAddrFromURI(woof_name, ip_str, sizeof(ip_str));
	if (err < 0)
	{
		/*
		 * try local addr
		 */
		err = WooFLocalIP(ip_str, sizeof(ip_str));
		if (err < 0)
		{
			fprintf(stderr, "WooFMQTTMsgGetElSize: woof: %s invalid ip address\n",
					woof_name);
			fflush(stderr);
			return (-1);
		}
	}

	err = WooFPortFromURI(woof_name, &port);
	if (err < 0)
	{
		port = WooFPortHash(wnamespace);
	}

	memset(endpoint, 0, sizeof(endpoint));
	sprintf(endpoint, ">tcp://%s:%d", ip_str, port);

#ifdef DEBUG
	printf("WooFMQTTMsgGetElSize: woof: %s trying enpoint %s\n", woof_name, endpoint);
	fflush(stdout);
#endif

	msg = zmsg_new();
	if (msg == NULL)
	{
		fprintf(stderr, "WooFMQTTMsgGetElSize: woof: %s no outbound msg to server at %s\n",
				woof_name, endpoint);
		perror("WooFMQTTMsgGetElSize: allocating msg");
		fflush(stderr);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMQTTMsgGetElSize: woof: %s got new msg\n", woof_name);
	fflush(stdout);
#endif

	/*
	 * this is a getelsize message
	 */
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%u", WOOF_MSG_GET_EL_SIZE);
	frame = zframe_new(buffer, strlen(buffer));
	if (frame == NULL)
	{
		fprintf(stderr, "WooFMQTTMsgGetElSize: woof: %s no frame for WOOF_MSG_GET_EL_SIZE command in to server at %s\n",
				woof_name, endpoint);
		perror("WooFMQTTMsgGetElSize: couldn't get new frame");
		fflush(stderr);
		zmsg_destroy(&msg);
		return (-1);
	}

#ifdef DEBUG
	printf("WooFMQTTMsgGetElSize: woof: %s got WOOF_MSG_GET_EL_SIZE command frame frame\n", woof_name);
	fflush(stdout);
#endif
	err = zmsg_append(msg, &frame);
	if (err < 0)
	{
		fprintf(stderr, "WooFMQTTMsgGetElSize: woof: %s can't append WOOF_MSG_GET_EL_SIZE command frame to msg for server at %s\n",
				woof_name, endpoint);
		perror("WooFMQTTMsgGetElSize: couldn't append woof_name frame");
		zframe_destroy(&frame);
		zmsg_destroy(&msg);
		return (-1);
	}

	/*
	 * make a frame for the woof_name
	 */
	frame = zframe_new(woof_name, strlen(woof_name));
	if (frame == NULL)
	{
		fprintf(stderr, "WooFMQTTMsgGetElSize: woof: %s no frame for woof_name to server at %s\n",
				woof_name, endpoint);
		perror("WooFMQTTMsgGetElSize: couldn't get new frame");
		fflush(stderr);
		zmsg_destroy(&msg);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMQTTMsgGetElSize: woof: %s got woof_name namespace frame\n", woof_name);
	fflush(stdout);
#endif
	/*
	 * add the woof_name frame to the msg
	 */
	err = zmsg_append(msg, &frame);
	if (err < 0)
	{
		fprintf(stderr, "WooFMQTTMsgGetElSize: woof: %s can't append woof_name to frame to server at %s\n",
				woof_name, endpoint);
		perror("WooFMQTTMsgGetElSize: couldn't append woof_name namespace frame");
		zframe_destroy(&frame);
		zmsg_destroy(&msg);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMQTTMsgGetElSize: woof: %s added woof_name namespace to frame\n", woof_name);
	fflush(stdout);
#endif

#ifdef DEBUG
	printf("WooFMQTTMsgGetElSize: woof: %s sending message to server at %s\n",
		   woof_name, endpoint);
	fflush(stdout);
#endif

	r_msg = WooFMQTTRequest(endpoint, msg);

	if (r_msg == NULL)
	{
		fprintf(stderr, "WooFMQTTMsgGetElSize: woof: %s couldn't recv msg for element size from server at %s\n",
				woof_name, endpoint);
		perror("WooFMQTTMsgGetElSize: no response received");
		fflush(stderr);
		return (-1);
	}
	else
	{
		r_frame = zmsg_first(r_msg);
		if (r_frame == NULL)
		{
			fprintf(stderr, "WooFMQTTMsgGetElSize: woof: %s no recv frame for from server at %s\n",
					woof_name, endpoint);
			perror("WooFMQTTMsgGetElSize: no response frame");
			zmsg_destroy(&r_msg);
			return (-1);
		}
		str = zframe_data(r_frame);
		el_size = atol((char *)str);
		zmsg_destroy(&r_msg);
	}

#ifdef DEBUG
	printf("WooFMQTTMsgGetElSize: woof: %s recvd size: %lu message from server at %s\n",
		   woof_name, el_size, endpoint);
	fflush(stdout);

#endif

#ifdef woofcache
	el_size_cached = (unsigned long *)malloc(sizeof(unsigned long));
	if (el_size_cached != NULL)
	{
		*el_size_cached = el_size;
		err = woofcacheinsert(woof_cache, woof_name, (void *)el_size_cached);
		if (err < 0)
		{
			payload = woofcacheage(woof_cache);
			if (payload != NULL)
			{
				free(payload);
			}
			err = woofcacheinsert(woof_cache, woof_name, (void *)el_size_cached);
			if (err < 0)
			{
				fprintf(stderr, "WooFMQTTMsgGetElSize: cache insert failed\n");
				fflush(stderr);
				free(el_size_cached);
			}
		}
	}
#endif

	return (el_size);
}


int WooFMsgGetMQTT(const char *woof_name, void *element, unsigned long el_size, unsigned long seq_no)
{
	char endpoint[255];
	char wnamespace[2048];
	char ip_str[25];
	int port;
	zmsg_t *msg;
	zmsg_t *r_msg;
	int r_size;
	zframe_t *frame;
	zframe_t *r_frame;
	char buffer[255];
	int err;

	memset(wnamespace, 0, sizeof(wnamespace));
	err = WooFNameSpaceFromURI(woof_name, wnamespace, sizeof(wnamespace));
	if (err < 0)
	{
		fprintf(stderr, "WooFMsgGet: woof: %s no name space\n",
				woof_name);
		fflush(stderr);
		return (-1);
	}


	memset(ip_str, 0, sizeof(ip_str));
	err = WooFIPAddrFromURI(woof_name, ip_str, sizeof(ip_str));
	if (err < 0)
	{
		/*
		 * assume it is local
		 */
		err = WooFLocalIP(ip_str, sizeof(ip_str));
		if (err < 0)
		{
			fprintf(stderr, "WooFMsgGet: woof: %s invalid ip address\n",
					woof_name);
			fflush(stderr);
			return (-1);
		}
	}

	err = WooFPortFromURI(woof_name, &port);
	if (err < 0)
	{
		port = WooFPortHash(wnamespace);
	}

	memset(endpoint, 0, sizeof(endpoint));
	sprintf(endpoint, ">tcp://%s:%d", ip_str, port);

#ifdef DEBUG
	printf("WooFMsgGet: woof: %s trying enpoint %s\n", woof_name, endpoint);
	fflush(stdout);
#endif

	msg = zmsg_new();
	if (msg == NULL)
	{
		fprintf(stderr, "WooFMsgGet: woof: %s no outbound msg to server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgGet: allocating msg");
		fflush(stderr);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMsgGet: woof: %s got new msg\n", woof_name);
	fflush(stdout);
#endif

	/*
	 * this is a put message
	 */
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%u", WOOF_MSG_GET);
	frame = zframe_new(buffer, strlen(buffer));
	if (frame == NULL)
	{
		fprintf(stderr, "WooFMsgGet: woof: %s no frame for WOOF_MSG_GET command in to server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgGet: couldn't get new frame");
		fflush(stderr);
		zmsg_destroy(&msg);
		return (-1);
	}

#ifdef DEBUG
	printf("WooFMsgGet: woof: %s got WOOF_MSG_GET command frame frame\n", woof_name);
	fflush(stdout);
#endif
	err = zmsg_append(msg, &frame);
	if (err < 0)
	{
		fprintf(stderr, "WooFMsgGet: woof: %s can't append WOOF_MSG_GET command frame to msg for server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgGet: couldn't append woof_name frame");
		zframe_destroy(&frame);
		zmsg_destroy(&msg);
		return (-1);
	}

	/*
	 * make a frame for the woof_name
	 */
	frame = zframe_new(woof_name, strlen(woof_name));
	if (frame == NULL)
	{
		fprintf(stderr, "WooFMsgGet: woof: %s no frame for woof_name to server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgGet: couldn't get new frame");
		fflush(stderr);
		zmsg_destroy(&msg);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMsgGet: woof: %s got woof_name namespace frame\n", woof_name);
	fflush(stdout);
#endif
	/*
	 * add the woof_name frame to the msg
	 */
	err = zmsg_append(msg, &frame);
	if (err < 0)
	{
		fprintf(stderr, "WooFMsgGet: woof: %s can't append woof_name to frame to server at %s\n",
				woof_name, endpoint);
		perror("WooFMsgGet: couldn't append woof_name namespace frame");
		zframe_destroy(&frame);
		zmsg_destroy(&msg);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMsgGet: woof: %s added woof_name namespace to frame\n", woof_name);
	fflush(stdout);
#endif

	/*
	 * make a frame for the seq_no
	 */

	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%lu", seq_no);
	frame = zframe_new(buffer, strlen(buffer));

	if (frame == NULL)
	{
		fprintf(stderr, "WooFMsgGet: woof: %s no frame for seq_no %lu server at %s\n",
				woof_name, seq_no, endpoint);
		perror("WooFMsgGet: couldn't get new seq_no frame");
		fflush(stderr);
		zmsg_destroy(&msg);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMsgGet: woof: %s got frame for seq_no %lu\n", woof_name, seq_no);
	fflush(stdout);
#endif

	err = zmsg_append(msg, &frame);
	if (err < 0)
	{
		fprintf(stderr, "WooFMsgGet: woof: %s couldn't append frame for seq_no %lu to server at %s\n",
				woof_name, seq_no, endpoint);
		perror("WooFMsgGet: couldn't append seq_no frame");
		fflush(stderr);
		zframe_destroy(&frame);
		zmsg_destroy(&msg);
		return (-1);
	}
#ifdef DEBUG
	printf("WooFMsgGet: woof: %s appended frame for seq_no %lu\n", woof_name, seq_no);
	fflush(stdout);
#endif

#ifdef DEBUG
	printf("WooFMsgGet: woof: %s sending message to server at %s\n",
		   woof_name, endpoint);
	fflush(stdout);
#endif

	r_msg = WooFMQTTRequest(endpoint, msg);
	if (r_msg == NULL)
	{
		fprintf(stderr, "WooFMsgGet: woof: %s couldn't recv msg for seq_no %lu to server at %s\n",
				woof_name, seq_no, endpoint);
		perror("WooFMsgGet: no response received");
		fflush(stderr);
		return (-1);
	}
	else
	{
		r_frame = zmsg_first(r_msg);
		if (r_frame == NULL)
		{
			fprintf(stderr, "WooFMsgGet: woof: %s no recv frame for seq_no %lu to server at %s\n",
					woof_name, seq_no, endpoint);
			perror("WooFMsgGet: no response frame");
			zmsg_destroy(&r_msg);
			return (-1);
		}
		r_size = zframe_size(r_frame);
		if (r_size == 0)
		{
			fprintf(stderr, "WooFMsgGet: woof: %s no data in frame for seq_no %lu to server at %s\n",
					woof_name, seq_no, endpoint);
			fflush(stderr);
			zmsg_destroy(&r_msg);
			return (-1);
		}
		else
		{
			if ((unsigned)r_size > el_size)
			{
				r_size = el_size;
			}
			memcpy(element, zframe_data(r_frame), r_size);
#ifdef DEBUG
			printf("WooFMsgGet: woof: %s copied %lu bytes for seq_no %lu message to server at %s\n",
				   woof_name, r_size, seq_no, endpoint);
			fflush(stdout);

#endif
			zmsg_destroy(&r_msg);
		}
	}

#ifdef DEBUG
	printf("WooFMsgGet: woof: %s recvd seq_no %lu message to server at %s\n",
		   woof_name, seq_no, endpoint);
	fflush(stdout);

#endif
	return (1);
}

/*
 * thread subscribes to device output topic and forwards request to cspot
 * assumes that mqtt-sn gateway is transparent (e.g. message is coming from
 * mosquitto)
 */


void *MQTTDeviceOutputThread(void *arg)
{
	char *device_name; /* copy in device name */
	int len;
	char sub_string[16*1024];
	char pub_string[16*1024];
	FILE *fd;
	char *mqtt_msg;
	char *resp_string;
	char *mstring;
	int msgid;
	WMQTT *wm;
	int size;
	unsigned long seqno;
	unsigned long lsize;
	void *element_buff;
	char *element_string;
	int err;
	char **msgs;
	int i;
	RB *rb;
	RESP *rs;

	len = strlen((char *)arg) + 1;
	device_name = (char *)malloc(len);
	if(device_name == NULL) {
		fprintf(stderr,"MQTTDeviceOutputThread: no space\n");
		pthread_exit(NULL);
	}
	mqtt_msg = (char *)malloc(WOOF_MQTT_MAX_SIZE);
	if(mqtt_msg == NULL) {
		free(device_name);
		fprintf(stderr,"MQTTDeviceOutputThread: no space for msg\n");
		pthread_exit(NULL);
	}

	resp_string = (char *)malloc(WOOF_MQTT_MAX_SIZE);
	if(resp_string == NULL) {
		free(device_name);
		fprintf(stderr,"MQTTDeviceOutputThread: no space for resp\n");
		pthread_exit(NULL);
	}

	memset(device_name,0,len);
	strncpy(device_name,(char *)arg,len);


	/*
	 * no timeout here
	 */
	memset(sub_string,0,sizeof(sub_string));
	sprintf(sub_string,"/usr/bin/mosquitto_sub -q 0 -h %s -t %s.output -u \'%s\' -P \'%s\'",
			Broker,
			device_name,
			User_name,
			Password);
printf("sub_string: %s\n",sub_string);

	fd = popen(sub_string,"r");
	while(fd != NULL) {
		memset(mqtt_msg,0,WOOF_MQTT_MAX_SIZE);
		size = read(fileno(fd),mqtt_msg,WOOF_MQTT_MAX_SIZE);
		if(size <= 0) {
			break;
		}
		msgs = ExtractMsgList(mqtt_msg);
		if(msgs != NULL) {
			i = 0;
			while(msgs[i] != NULL) {
printf("input msg from DEVICE: %s\n",msgs[i]);
				msgid = ExtractMsgID(msgs[i]);
				pthread_mutex_lock(&RLock);
				rb = RBFindI(RespList,msgid);
				if(rb != NULL) {
printf("FOUND msg id %d\n",msgid);
					rs = (RESP *)rb->value.v;
					mstring = (char *)malloc(strlen(msgs[i])+1);
					if(mstring == NULL) {
						RBDeleteI(RespList,rb);
						pthread_mutex_unlock(&RLock);
						FreeRESP(rs);
						i++;
						continue;
					}
					memset(mstring,0,strlen(msgs[i])+1);
					strncpy(mstring,msgs[i],strlen(msgs[i]));
					rs->resp_string = mstring;
					pthread_mutex_unlock(&RLock);
					VSemPT(rs->s);
					i++;
					continue;
				} else {
					pthread_mutex_unlock(&RLock);
printf("ORIGIN msg id %d\n",msgid);
					wm = ParseMQTTString(msgs[i]);
					if(wm == NULL) {
						fprintf(stderr,"MQTTDeviceOutputThread: couldn't parse %s\n",
							msgs[i]);
						i++;
						continue;
					}
				/*
				 * main processing dispatch
				 */
					memset(resp_string,0,WOOF_MQTT_MAX_SIZE);
					switch(wm->command) {
					case WOOF_MQTT_PUT:
						seqno = WooFPut(wm->woof_name,
								wm->handler_name,
								wm->element);
		//printf("woof: %s, handler: %s\n",wm->woof_name,wm->handler_name);
						sprintf(resp_string,"%s|%d|%d|%d",
								wm->woof_name,
								WOOF_MQTT_PUT_RESP,
								wm->msgid,
								(int)seqno);
						break;
					case WOOF_MQTT_GET_EL_SIZE:
						//lsize = WooFMQTTMsgGetElSize(wm->woof_name);
						lsize = WooFMsgGetElSize(wm->woof_name);
						sprintf(resp_string,"%s|%d|%d|%d",
								wm->woof_name,
								WOOF_MQTT_GET_EL_SIZE_RESP,
								wm->msgid,
								(int)lsize);
						break;
					case WOOF_MQTT_GET_LATEST_SEQNO:
						seqno = WooFGetLatestSeqno(wm->woof_name);
						sprintf(resp_string,"%s|%d|%d|%d",
								wm->woof_name,
								WOOF_MQTT_GET_LATEST_SEQNO_RESP,
								wm->msgid,
								(int)seqno);
						break;
					case WOOF_MQTT_GET:
						//lsize = WooFMQTTMsgGetElSize(wm->woof_name);
						lsize = WooFMsgGetElSize(wm->woof_name);
						if(lsize == (unsigned long)-1) {
							/*
							 * use 0 length to indicate err
							 */
							sprintf(resp_string,"%s|%d|%d|%d",
								wm->woof_name,
								WOOF_MQTT_GET_RESP,
								wm->msgid,
								(int)0);
							break;
						}
						element_buff = (void *)malloc(lsize);
						if(element_buff == NULL) {
							sprintf(resp_string,"%s|%d|%d|%d",
								wm->woof_name,
								WOOF_MQTT_GET_RESP,
								wm->msgid,
								(int)0);
							break;
						}
						/*
						 * calling MsgGet instead of Get saves a call to GetElSize
						err = WooFMsgGetMQTT(wm->woof_name,
								element_buff,
								lsize,
								wm->seqno);
						*/
						err = WooFGet(wm->woof_name,
								element_buff,
								wm->seqno);
						if(err < 0) {
							free(element_buff);
							/*
							 * use 0 length to indicate err
							 */
							sprintf(resp_string,"%s|%d|%d|%d",
								wm->woof_name,
								WOOF_MQTT_GET_RESP,
								wm->msgid,
								(int)0);
							break;
						}
						element_string = (char *)malloc(2*lsize+1);
						if(element_string == NULL) {
							free(element_buff);
							sprintf(resp_string,"%s|%d|%d|%d",
								wm->woof_name,
								WOOF_MQTT_GET_RESP,
								wm->msgid,
								(int)0);
							break;
						}
						memset(element_string,0,(lsize*2+1));
						err = ConvertBinarytoASCII(element_string,element_buff,lsize);
						if(err <= 0) {
							free(element_buff);
							free(element_string);
							sprintf(resp_string,"%s|%d|%d|%d",
								wm->woof_name,
								WOOF_MQTT_GET_RESP,
								wm->msgid,
								(int)0);
							break;
						}
						sprintf(resp_string,"%s|%d|%d|%d|%s",
								wm->woof_name,
								WOOF_MQTT_GET_RESP,
								wm->msgid,
								(int)lsize,
								element_string);
						free(element_string);
						free(element_buff);
						break;
					default:
						sprintf(resp_string,"%s|%d|%d|%d",
								wm->woof_name,
								WOOF_MQTT_BAD_RESP,
								wm->msgid,
								-1);
						break;
					}

		//printf("output msg response to DEVICE: %s\n",resp_string);
				/*
				 * send the respond back on the input channel
				 */
					memset(pub_string,0,sizeof(pub_string));
#ifdef FP
					sprintf(pub_string,"/usr/bin/mosquitto_pub -q 0 -h %s -t %s.input -u \'%s\' -P \'%s\' -m \'%s\'",
						Broker,
						device_name, 
						User_name,
						Password,
						resp_string);
		printf("output msg pub TO DEVICE %s\n",pub_string);
					system(pub_string);
#else
					sprintf(pub_string,"%s\n",
						resp_string);
					DeviceSend(pub_string);
		printf("output msg pub TO DEVICE %s\n",pub_string);

#endif
					FreeWMQTT(wm);
					wm = NULL;
					i++;
			}
		}
		FreeMsgList(msgs);
	}
	}


	FreeWMQTT(wm);
	pclose(fd);
	free(mqtt_msg);
	free(resp_string);
	free(device_name);
	pthread_exit(NULL);
}


FILE *InputPD; // for message going to the device
void DeviceSend(char *s)
{
	int size;
	size = write(fileno(InputPD),s,strlen(s));
	if(size != strlen(s)) {
		printf("DeviceSend failed for %s\n",s);
	}
	return;
}



#define ARGS "n:u:p:t:b:"
const char *usage = "woofc-mqtt-gateway -n device-name-space\n\
\t -u mqtt broker user name\n\
\t -p mqtt broker pw\n\
\t -b IP address of mosquitto broker\n\
\t -t mosquitto timeout\n";

int main(int argc, char **argv)
{
	int c;
	pthread_t sub_thread;
	int err;
	char *cred;
	char *tmp;
	char pub_string[128];

	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'n':
				strncpy(Device_name_space,optarg,sizeof(Device_name_space)-1);
				break;
			case 'u':
				strncpy(User_name,optarg,sizeof(User_name)-1);
				break;
			case 'p':
				strncpy(Password,optarg,sizeof(Password)-1);
				break;
			case 'b':
				strncpy(Broker,optarg,sizeof(Broker));
				break;
			case 't':
				Timeout = atoi(optarg);
				break;
			default:
				fprintf(stderr,"woofc-mqtt-gateway: unrecognized command %c\n",
						(char)c);
				fprintf(stderr,"usage: %s",usage);
				exit(1);
		}
	}
	if(Device_name_space[0] == 0) {
		fprintf(stderr,"woofc-mqtt-gateway: must specify device name space\n");
		fprintf(stderr,"usage: %s",usage);
		fprintf(stderr,"device name space must begin with word 'devices'\n");
		exit(1);
	}

	if(User_name[0] == 0) {
		cred = getenv("WOOFC_MQTT_USER");
		if(cred == NULL) {
			fprintf(stderr,"couldn't find user name for mqtt broker\n");
			fprintf(stderr,"either specify WOOFC_MQTT_USER environment variable or\n");
			fprintf(stderr,"usage: %s",usage);
			exit(1);
		} else {
			strncpy(User_name,cred,sizeof(User_name));
		}
	}

	if(Password[0] == 0) {
		cred = getenv("WOOFC_MQTT_PW");
		if(cred == NULL) {
			fprintf(stderr,"couldn't find Password for mqtt broker\n");
			fprintf(stderr,"either specify WOOFC_MQTT_PW environment variable or\n");
			fprintf(stderr,"usage: %s",usage);
			exit(1);
		} else {
			strncpy(Password,cred,sizeof(Password));
		}
	}

	/*
	 * device namespace must begin with a /
	 */
	if(Device_name_space[0] != '/') {
		tmp = (char *)malloc(strlen(Device_name_space));
		if(tmp == NULL) {
			exit(1);
		}
		strcpy(tmp,Device_name_space);
		Device_name_space[0] = '/';
		strncpy(&Device_name_space[1],tmp,sizeof(Device_name_space)-2);
		Device_name_space[sizeof(Device_name_space)-1] = 0;
		free(tmp);
	}

	if(Broker[0] == 0) {
		strncpy(Broker,"127.0.0.1",sizeof(Broker));
	}

	if(Timeout == 0) {
		Timeout = DEFAULT_MQTT_TIMEOUT;
	}


	/*
	 * for responses
	 */
	RespList = RBInitI();
	if(RespList == NULL) {
		printf("no space for resp list\n");
		pthread_exit(NULL);
	}
	pthread_mutex_init(&RLock,NULL);

	/*
	 * create thread that subscribes to device output
	 */
	err = pthread_create(&sub_thread,NULL,MQTTDeviceOutputThread,(void *)Device_name_space);
	if(err < 0) {
		printf("woofc-mqtt-gateway: couldn't create subscriber thread\n");
		exit(1);
	}

	/*
	 * create connection to broker for device input pubs
	 */
	memset(pub_string,0, sizeof(pub_string));
	sprintf(pub_string,"/usr/bin/mosquitto_pub --stdin-line -q 0 -h %s -t %s.input -u \'%s\' -P \'%s\'",
                                Broker,
                                Device_name_space,
                                User_name,
                                Password);
	InputPD = popen(pub_string,"w");
	if(InputPD == NULL) {
		printf("woofc-mqq-gateway: could not contact broker with %s\n",pub_string);
		exit(1);
	}

	/*
	 * woofmsgsserver blocks forever
	 */
	err = WooFMsgServerMQTT(Device_name_space);
	if(err < 0) {
		printf("woofc-mqtt-gateway: couldn't create zmq msg server\n");
		exit(1);
	}
	pthread_join(sub_thread,NULL);
	exit(0);
}



