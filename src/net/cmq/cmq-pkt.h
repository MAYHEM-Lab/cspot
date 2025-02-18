#ifndef CMQ_PKT_H
#define CMQ_PKT_H

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "cmq-frame.h"

#define CMQ_PKT_VERSION (1)
#define CMQ_MAX_PKT_SIZE (1460)

struct cmq_pkt_header_stc
{
	unsigned long version;
	unsigned long msg_no;
	unsigned long start_fno;
	unsigned long end_fno;
	unsigned long frame_count;
};

typedef struct cmq_pkt_header_stc CMQPKTHEADER; 

int cmq_pkt_xfer_size(unsigned char *fl);
int cmq_pkt_pack_frame_list(unsigned char *buffer, unsigned char *fl);
int cmq_pkt_send_frame_list_request(char *dst_addr, 
			    unsigned short dst_port, 
			    unsigned char *fl,
			    int **sd);
int cmq_pkt_recv_frame_list_reply(int sd, unsigned char **fl);



#endif

