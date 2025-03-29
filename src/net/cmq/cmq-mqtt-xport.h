#ifndef CMQ_MQTT_XPORT_H
#define CMQ_MQTT_XPORT_H

#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>
#include <redblack.h>

#define IPLEN 17
#define CONNBUFFERSIZE (256*1024)

struct cmq_mqtt_proxy_stc
{
	int init;
	char namespace[2014];
	char host_ip[IPLEN];
	char broker_ip[IPLEN]; // IP adress of mosquitto broker
	char user[1024];
	char pw[1024];
	pthread_mutex_t lock; // for thread safe accept
	RB *connections;
};

typedef struct cmq_mqtt_proxy_stc CMQPROXY;

struct cmq_mqtt_conn_stc
{
	int sd; 		// fake socket id
	int type;		// connect, listen, or accept	
	FILE *pub_fd;
	FILE *sub_fd;
	pid_t sub_pid; // for mosquitto_sub shutdown
	struct timeval timeout;
	unsigned char buffer[CONNBUFFERSIZE]; // max msg size is 16K
	int cursor; // emulate file r/w pointer
	int read_len; // size of last read
};

typedef struct cmq_mqtt_conn_stc CMQCONN;
#define CMQCONNConnect (1)
#define CMQCONNListen (2)
#define CMQCONNAccept (3)


#endif

