#ifndef CMQ_PKT_H
#define CMQ_PKT_H

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "cmq-frame.h"

#define CMQ_PKT_VERSION (1)

struct cmq_pkt_header_stc
{
	unsigned long version;
	unsigned long frame_count;
	// add max hint as optimization
};

typedef struct cmq_pkt_header_stc CMQPKTHEADER; 

int cmq_pkt_endpoint(char *addr, unsigned short port);
int cmq_pkt_listen(unsigned long port);
int cmq_pkt_send_msg(int endpoint, unsigned char *fl);
int cmq_pkt_recv_msg(int endpoint, unsigned char **fl);


#endif

