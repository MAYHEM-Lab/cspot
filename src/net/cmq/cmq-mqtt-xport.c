#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>
#include <yaml.h>
#include <errno.h>

#include "cmq-pkt.h"
#include "cmq-mqtt-xport.h"

CMQPROXY MQTT_Proxy; 


int ConvertASCIItoBinary(unsigned char *dest, char *src, int len)
{
	int count;
	char *curr;
	unsigned char c;
	unsigned char high;
	unsigned char low;

	/*
	 * assumes data is encoded as 112233445566778899AABBCCDDEEFF in 2 char hex
	 */

	curr = src;
	count = 0;
	while((count < len) && (*curr != 0)) {
		if((curr[0] >= '0') && (curr[0] <= '9')) {
			high = (curr[0] - '0') * 16;
		} else if ((curr[0] >= 'a') && (curr[0] <= 'f')) {
			high = (curr[0] - 'a' + 10) * 16;
		}
		if((curr[1] >= '0') && (curr[1] <= '9')) {
			low = (curr[1] - '0');
		} else if ((curr[1] >= 'a') && (curr[1] <= 'f')) {
			low = (curr[1] - 'a' + 10);
		}
		c = high+low;
		dest[count] = c;
		curr += 2;
		count++;
	}
	return(1);
}

int ConvertBinarytoASCII(char *dest, void *src, int len)
{
        unsigned char *csrc = (unsigned char *)src;
        int count;
        unsigned char *curr;
        char *wbuf;

        count = 0;
        curr = csrc;
        wbuf = dest;

        while(count < len) {
                sprintf(wbuf,"%2.2x",(unsigned int)curr[count]);
                wbuf += 2;
                count++;
        }
        return(count);
}


int cmq_mqtt_proxy_init()
{
	char *home_dir;
	struct stat file_stat; 
	char file_path[1024];
	char m_string[2048];
	char credfile[512];
	yaml_parser_t parser;
	yaml_event_t event;
	int state; // 0 => looking, 1 => woof: 2 => check: 
	int done = 0;
	int found = 0;
	FILE *file;

	if(MQTT_Proxy.init == 1) {
		return(1);
	}

	memset(credfile,0,sizeof(credfile));

	// check local first
	// must be user read but otherwise not accessible
	if(access("./mqtt-proxy.yaml", R_OK) == 0) {
		if(stat("./mqtt-proxy.yaml",&file_stat) == 0) {
			if((file_stat.st_mode & S_IRUSR) != 0) {
				  if((file_stat.st_mode & 
				      (S_IRGRP | S_IWGRP | S_IXGRP |
				       S_IROTH | S_IWOTH | S_IROTH)) == 0) {
					  strncpy(credfile,"./mqtt-proxy.yaml",sizeof(credfile)-1);
					  found = 1;
				  }
			}
		}
	}

	home_dir = getenv("HOME");
	if(home_dir != NULL) {
		memset(file_path,0,sizeof(file_path));
		snprintf(file_path,sizeof(file_path),"%s/.cspot/mqtt-proxy.yaml",home_dir);
		if(access(file_path, R_OK) == 0) {
			if(stat(file_path,&file_stat) == 0) {
				if((file_stat.st_mode & S_IRUSR) != 0) {
					  if((file_stat.st_mode & 
					      (S_IRGRP | S_IWGRP | S_IXGRP |
					       S_IROTH | S_IWOTH | S_IROTH)) == 0) {
						  strncpy(credfile,file_path,sizeof(credfile)-1);
						  found = 1;
					  }
				}
			}
		}
	}

	// error out if we can't find creds file
	if(found == 0) {
		return(-1);
	}

	file = fopen(credfile,"r");
	if(file == NULL) {
		return(-1);
	}

	if(!yaml_parser_initialize(&parser)) {
		return(-1);
	}

	yaml_parser_set_input_file(&parser, file);

	state = 0;
	found = 0;
	while(done == 0) {
		if(!yaml_parser_parse(&parser, &event)) {
			break;
		}

		switch(event.type) {
			case YAML_SCALAR_EVENT:
//printf("Key/Value: %s\n", event.data.scalar.value);
				// NULL implies just print
				if(state == 0) {
					if(strncmp(event.data.scalar.value,
						"proxy",strlen("proxy")) == 0) {
						state = 1; //next state
					}
				} else if(state == 1) {
					if(strncmp(event.data.scalar.value,
						"namespace",strlen("namespace")) == 0) {
						state = 2;
					}
					if(strncmp(event.data.scalar.value,
                                                "proxy",strlen("proxy")) == 0) {
                                        	state = 1;
                                        }

				} else if(state == 2) {
					strncpy(MQTT_Proxy.namespace,event.data.scalar.value,
                                                        sizeof(MQTT_Proxy.namespace)-1);
//printf("namespace found: %s\n",MQTT_Proxy.namespace);
						state = 3;
				} else if(state == 3) {
					if(strncmp(event.data.scalar.value, "host-ip",
							strlen("host-ip")) == 0) {
						state = 4;
					}
					if(strncmp(event.data.scalar.value,"proxy",
							strlen("proxy")) == 0) {
						state = 1;
					}
				} else if(state == 4) {
					strncpy(MQTT_Proxy.host_ip,event.data.scalar.value,
							sizeof(MQTT_Proxy.host_ip)-1);
//printf("host_ip found: %s\n",MQTT_Proxy.host_ip);
					state = 5;
				} else if(state == 5) {
					if(strncmp(event.data.scalar.value,
							"broker-ip", strlen("broker-ip")) == 0) {
						state = 6;
					}
					if(strncmp(event.data.scalar.value,"proxy",
							strlen("proxy")) == 0) {
						state = 1;
					}
				} else if(state == 6) {
					strncpy(MQTT_Proxy.broker_ip,event.data.scalar.value,
							sizeof(MQTT_Proxy.broker_ip)-1);
//printf("broker_ip: %s\n",MQTT_Proxy.broker_ip);
					state = 7;
				} else if(state == 7) {
					if(strncmp(event.data.scalar.value,
							"user", strlen("user")) == 0) {
						state = 8;
					}
					if(strncmp(event.data.scalar.value,"proxy",
							strlen("proxy")) == 0) {
						state = 1;
					}
				} else if(state == 8) {
					strncpy(MQTT_Proxy.user,event.data.scalar.value,
							sizeof(MQTT_Proxy.user)-1);
//printf("user: %s\n",MQTT_Proxy.user);
					state = 9;
				} else if(state == 9) {
					if(strncmp(event.data.scalar.value,
							"pw", strlen("pw")) == 0) {
						state = 10;
					}
					if(strncmp(event.data.scalar.value,"proxy",
							strlen("proxy")) == 0) {
						state = 1;
					}
				} else if(state == 10) {
					strncpy(MQTT_Proxy.pw,event.data.scalar.value,
							sizeof(MQTT_Proxy.pw)-1);
//printf("pw: %s\n",MQTT_Proxy.pw);
					found = 1;
					done = 1;
					state = 0;
				}
				break;
			case YAML_STREAM_END_EVENT:
				done = 1;
				break;
			default:
				break;
		}
		yaml_event_delete(&event);
	}

    yaml_parser_delete(&parser);
    fclose(file);
    if(found == 1) {
	    MQTT_Proxy.connections = RBInitI();
	    pthread_mutex_init(&MQTT_Proxy.lock,NULL); // for thread safe accept
	    if(MQTT_Proxy.connections == NULL) {
		    return(-1);
	    }
	    MQTT_Proxy.init = 1;
	    return(1);
    } else {
	    return(-1);
    }
}

CMQCONN *cmq_mqtt_create_conn(int type, int port)
{
	int err;
	CMQCONN *conn;
	char m_string[4096];
	RB *rb;

	err = cmq_mqtt_proxy_init();
	if(err < 0) {
		return(NULL);
	}

	// see if this connection exists
	rb = RBFindI(MQTT_Proxy.connections,port);
	if(rb != NULL) {
		conn = (CMQCONN *)rb->value.v;
		return(conn);
	}

	conn = (CMQCONN *)malloc(sizeof(CMQCONN));
	if(conn == NULL) {
		return(NULL);
	}
	memset(conn,0,sizeof(CMQCONN));
	conn->type = type;
	conn->port = port;
	if((type == CMQCONNConnect) || (type == CMQCONNAccept)) {
		conn->sd = (int)(drand48()*50000) + 1;
	} else {
		conn->sd = port;
	}

	// set up in-bound channel
	snprintf(m_string,sizeof(m_string),"/usr/bin/mosquitto_sub -h %s -t %s/%s/%8.8d/input -u \'%s\' -P \'%s\'",
			    MQTT_Proxy.broker_ip,
			    MQTT_Proxy.host_ip,
			    MQTT_Proxy.namespace,
			    conn->sd,
			    MQTT_Proxy.user,
			    MQTT_Proxy.pw);
	conn->sub_fd = popen(m_string,"r");
	if(conn->sub_fd == NULL) {
		perror("ERROR: could not create sub channel\n");
		return(NULL);
	}
	//
	// set out-bound send channel
	snprintf(m_string,sizeof(m_string),"/usr/bin/mosquitto_pub -h %s -t %s/%s/%8.8d/output -u \'%s\' -P \'%s\'",
			    MQTT_Proxy.broker_ip,
			    MQTT_Proxy.host_ip,
			    MQTT_Proxy.namespace,
			    conn->port,
			    MQTT_Proxy.user,
			    MQTT_Proxy.pw);
	conn->pub_fd = popen(m_string,"w");
	if(conn->pub_fd == NULL) {
		perror("ERROR: could not create pub channel");
		pclose(conn->sub_fd);
		return(NULL);
	}
	(void)RBInsertI(MQTT_Proxy.connections,conn->sd,(Hval)((void *)conn));

	return(conn);
}

void cmq_mqtt_destroy_conn(CMQCONN *conn)
{
	RB *rb;
	int err;

	err = cmq_mqtt_proxy_init();
	if(err < 0) {
		return;
	}

	rb = RBFindI(MQTT_Proxy.connections,conn->sd);
	if(rb != NULL) {
		RBDeleteI(MQTT_Proxy.connections,rb);
	}
	if(conn->sub_fd != NULL) {
		pclose(conn->sub_fd);
	}
	if(conn->pub_fd != NULL) {
		pclose(conn->pub_fd);
	}
	free(conn);
	return;
}

// mimics write() system call on a socket
int cmq_mqtt_conn_buffer_write(CMQCONN *conn, unsigned char *buf, int size)
{
	if((conn->cursor + size) > sizeof(conn->buffer)) {
		return(-1);
	}

	ConvertBinarytoASCII(&(conn->buffer[conn->cursor]),buf,size);
	conn->cursor += size;
	return(size);
}

// mimics read() system call on socket
int cmq_mqtt_conn_buffer_read(CMQCONN *conn, unsigned char *buf, int size)
{
	int len;
	if((conn->cursor + size) > sizeof(conn->buffer)) {
		len = sizeof(conn->buffer) - conn->cursor;
	} else {
		len = size;
	}

	if(len <= 0) {
		return(0);
	}

	ConvertASCIItoBinary(buf,&(conn->buffer[conn->cursor]),len);
	conn->cursor += len;
	return(len);
}

int cmq_mqtt_conn_buffer_seek(CMQCONN *conn, int loc)
{
	conn->cursor = loc;
	return(loc);
}
	
// creates a pub/sub pair with random ID for client connection to server
int cmq_mqtt_connect(char *addr, unsigned short port, unsigned long timeout)
{
	int err;
	CMQCONN *conn;
	char m_string[1024];

	err = cmq_mqtt_proxy_init();
	if(err < 0) {
		return(-1);
	}

	signal(SIGPIPE,SIG_IGN);

	conn = cmq_mqtt_create_conn(CMQCONNConnect,port);
	if(conn == NULL) {
		return(-1);
	}

	strncpy(conn->addr,addr,sizeof(conn->addr));
	conn->timeout.tv_sec = 0;
	conn->timeout.tv_usec = timeout/1000;

	return(conn->sd);
}

// creates a sub connection on specific input port
int cmq_mqtt_listen(unsigned long port)
{
	int err;
	CMQCONN *conn;
	char m_string[1024];

	err = cmq_mqtt_proxy_init();
	if(err < 0) {
		return(-1);
	}

	conn = cmq_mqtt_create_conn(CMQCONNListen,port);
	if(conn == NULL) {
		return(-1);
	}

	return(conn->sd);
}


int cmq_mqtt_accept(int sd, unsigned long timeout)
{
	int err;
	CMQCONN *conn;
	CMQCONN *new_conn;
	char m_string[1024];
	RB *rb;

	err = cmq_mqtt_proxy_init();
	if(err < 0) {
		return(-1);
	}

	rb = RBFindI(MQTT_Proxy.connections,(int)sd);
	if(rb == NULL) {
		printf("ERROR: could not find listen socket %d\n",sd);
		return(-1);
	}
	conn = (CMQCONN *)rb->value.v;
	
	// create new connection for next accept and so we can use its buffer
	new_conn = cmq_mqtt_create_conn(CMQCONNAccept,sd);
	if(new_conn == NULL) {
		return(-1);
	}
	
	// one thread at a time should read
	pthread_mutex_lock(&MQTT_Proxy.lock);
	// block reading input -- note that read() system call should be a transaction
	err = read(fileno(conn->sub_fd),new_conn->buffer,sizeof(new_conn->buffer));
	pthread_mutex_unlock(&MQTT_Proxy.lock);
	if(err <= 0) {
		cmq_mqtt_destroy_conn(new_conn);
		return(-1);
	}

	return(new_conn->sd);
}

int cmq_mqtt_send_msg(int sd, unsigned char *fl)
{
	int err;
	CMQFRAMELIST *frame_list = (CMQFRAMELIST *)fl;
	CMQFRAME *frame;
	CMQPKTHEADER header;
	unsigned long size;
	RB *rb;
	CMQCONN *conn;

	err = cmq_mqtt_proxy_init();
	if(err < 0) {
		return(-1);
	}

	rb = RBFindI(MQTT_Proxy.connections,sd);
	if(rb == NULL) {
		printf("ERROR: cmq_mqtt_send_msg could not find sd %d\n",sd);
		return(-1);
	}

	conn = (CMQCONN *)rb->value.v;
	cmq_mqtt_conn_buffer_seek(conn,0); // reset the cursor

	header.version = htonl(CMQ_PKT_VERSION);
	header.frame_count = htonl(frame_list->count);
	header.max_size = htonl(frame_list->max_size);

	// send the header using write
	err = cmq_mqtt_conn_buffer_write(conn,(unsigned char *)&header,sizeof(header));
	if(err < sizeof(header)) {
		return(-1);
	}

	// send the frames (if they are there)
	frame = frame_list->head;
	while(frame != NULL) {
		// write the frame size
		size = htonl(frame->size);
		err = cmq_mqtt_conn_buffer_write(conn,(unsigned char *)&size,sizeof(size));
		if(err < sizeof(size)) {
			return(-1);
		}
		// write the frame data if the size > 0
		if(frame->size > 0) {
			err = cmq_mqtt_conn_buffer_write(conn,frame->payload,frame->size);
			if(err < frame->size) {
				return(-1);
			}
		}
		frame = frame->next;
	}
	// now write the data to the pub socket
	err = write(fileno(conn->pub_fd),conn->buffer,conn->cursor);

	if(err < conn->cursor) {
		return(-1);
	}

	return(0);
}

int cmq_mqtt_recv_msg(int sd, unsigned char **fl)
{
	unsigned char *l_fl;
	CMQPKTHEADER header;
	unsigned char *f;
	int err;
	int i;
	unsigned long size;
	unsigned char *payload;
	unsigned long max_size;
	RB *rb;
	CMQCONN *conn;

	err = cmq_mqtt_proxy_init();
	if(err < 0) {
		return(-1);
	}

	rb = RBFindI(MQTT_Proxy.connections,sd);
	if(rb == NULL) {
		printf("ERROR: cmq_mqtt_recv_msg could not find sd %d\n",sd);
		return(-1);
	}

	conn = (CMQCONN *)rb->value.v;

	// read the sub socket
	err = read(fileno(conn->sub_fd),conn->buffer,sizeof(conn->buffer));
	if(err <= 0) {
		printf("ERROR: cmq_mqtt_recv_msg could not read sub on sd %d\n",sd);
		return(-1);
	}
	cmq_mqtt_conn_buffer_seek(conn,0); // reset the cursor

	// read the header
	err = cmq_mqtt_conn_buffer_read(conn,(unsigned char *)&header,sizeof(header));
	if(err < sizeof(header)) {
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
		err = cmq_mqtt_conn_buffer_read(conn,(unsigned char *)&size,sizeof(size));
		if(err < sizeof(size)) {
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
			err = cmq_mqtt_conn_buffer_read(conn,payload,ntohl(size));
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

void cmq_mqtt_close(int sd)
{
	int err;
	CMQCONN *conn;
	RB *rb;

	err = cmq_mqtt_proxy_init();
	if(err < 0) {
		return;
	}

	rb = RBFindI(MQTT_Proxy.connections,sd);
	if(rb != NULL) {
		conn = (CMQCONN *)rb->value.v;
		cmq_mqtt_destroy_conn(conn);
	}
	return;
}
		

#ifdef TESTCLIENT


#define PORT 8079
#define TIMEOUT 3000 // 3 second timeout

#define ARGS "h:p:c:"
char *Usage = "cmq-mqtt-test-client -h host_ip\n\
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
		
	endpoint = cmq_mqtt_connect(host_ip,host_port,TIMEOUT);
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
	err = cmq_mqtt_send_msg(endpoint,fl);
	if(err < 0) {
		fprintf(stderr,"ERROR: failed to send msg\n");
		exit(1);
	}

	// destroy the frame list
	cmq_frame_list_destroy(fl);

	// receive an echo of the frame list
	printf("receiving frame list from server %s:%lu\n",host_ip,host_port);
	err = cmq_mqtt_recv_msg(endpoint,&fl);
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

	err = cmq_mqtt_send_msg(endpoint,fl);
	if(err < 0) {
		fprintf(stderr,"ERROR: failed to send zero frame\n");
		exit(1);
	}

	cmq_frame_list_destroy(fl);
	// recv a zero frame back
	err = cmq_mqtt_recv_msg(endpoint,&fl);
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

	cmq_mqtt_close(endpoint);
	return(0);
}

#endif

#ifdef TESTSERVER

#define PORT 8079
#define TIMEOUT 3000 // 3 second accept timeout

#define ARGS "p:"
char *Usage = "cmq-mqtt-test-server -p host_port\n";

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
		
	server_sd = cmq_mqtt_listen(host_port);
	if(server_sd < 0) {
		fprintf(stderr,"ERROR: failed to create server_sd\n");
		perror("listen");
		exit(1);
	}

	endpoint = cmq_mqtt_accept(server_sd, 0); // zero timeout implies wait forever
	if(endpoint < 0) {
		fprintf(stderr,"ERROR: failed to accept endpoint\n");
		perror("listen");
		exit(1);
	}


	err = cmq_mqtt_recv_msg(endpoint,&fl);
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
	err = cmq_mqtt_send_msg(endpoint,fl);
	if(err < 0) {
		fprintf(stderr,"ERROR: failed to send msg\n");
		exit(1);
	}

	// destroy the frame list
	cmq_frame_list_destroy(fl);

	// test zero frame recv and echo
	err = cmq_mqtt_recv_msg(endpoint,&fl);
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
	err = cmq_mqtt_send_msg(endpoint,fl);
	if(err < 0) {
		fprintf(stderr,"ERROR: failed to send zero frame response\n");
		exit(1);
	}

	cmq_frame_list_destroy(fl);
	printf("server sent zero frame echo\n");

	cmq_mqtt_close(endpoint);
	return(0);
}

#endif
	


	
	
	

	
	

	

