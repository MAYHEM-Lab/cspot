#include <stdin.h>
#include <unistd.h>
#include <stdio.h>
#include <strng.h>

#include "cmq-pkt.h"

int cmq_pkt_xfer_size(unsigned char *fl)
{
	CMQFRAMELIST *frame_list = (CMQFRAMELIST *)fl;
	int total = 0;
	CMQFRAME *frame;

	if(fl == NULL) {
		return(0);
	}
	if(cmq_frame_list_empty(fl)) {
		return(0);
	}

	total += sizeof(CMQPKTHEADER);

	if(frame_list->last_packed != NULL) {
		// total from last packed frame for multipart
		if(frame_list->last_packed->next == NULL) {
			return(0);
		}
		frame = frame_list->last_packed->next;
	} else {
		frame = frame_list->head;
	}
	while(frame != NULL) {
		total += sizeof(unsigned long); // add in size designator
		total += frame->size;
		frame = frame->next;
	}

	return(total);
}

int cmq_pkt_pack_frame_list(unsigned char *buffer, int buffer_size, 
		unsigned char *fl, unsigned int *start_fno, unsigned int *end_fno)
{
	CMQFRAMELIST *frame_list = (CMQFRAMELIST *)fl;
	CMQFRAME *frame;
	CMQFRAME *last_packed;
	CMQPKTHEADER *header;
	unsigned char *curr;
	unsigned long size;
	unsigned long xfer_size;
	unsigned long next_size;
	int start;
	int end;

	if(fl == NULL) {
		return(-1);
	}
	if(cmq_frame_list_empty(fl)) {
		return(-1);
	}
	if(buffer == NULL) {
		return(-1);
	}

	xfer_size = 0;
	// find size to transfer in this buffer
	// set last_packed for next transfer if size exceeds buffer size
	xfer_size += sizeof(CMQPKTHEADER);
	last_packed = frame_list->last_packed;
	if(frame_list->last_packed == NULL) {
		frame = frame_list->head;
		start = 1;
		end = 1;
	} else {
		frame = frame_list->last_packed;
		if(frame->next == NULL) {
			return(-1);
		}
		frame = frame->next;
		start = frame->frame_no;
	}
	while(xfer_size < buffer_size) {
		next_size = sizeof(unsigned long) + frame->size;
		if((xfer_size + next_size) > buffer_size) {
			break;
		}
		xfer_size += next_size;
		end = frame->frame_no;
		last_packed = frame;
		frame = frame->next;
		if(frame == NULL) {
			break;
		}
	}


	header = (CMQPKTHEADER *)buffer;
	header->version = htonl(CMQ_PKT_VERSION); // put in version
	header->xfer_size = htonl(xfer_size); // needed for unpack
	header->msg_no = 0; // relaibility will set this
	header->start_fno = htonl(start);
	header->end_fno = htonl(end);
	header->frame_count = htonl(frame_list->count);

	curr = (buffer + sizeof(CMQPKTHEADER));
	if(frame_list->last_packed == NULL) {
		frame = frame_list->head;
	} else {
		frame = frame_list->last_packed;
		if(frame->next == NULL) {
			return(-1);
		}
		frame = frame->next;
	}

	while(frame != NULL) {
		size = htonl(frame->size);
		*((unsigned long *)curr) = size; // put in size
		curr += sizeof(unsigned long);
		memcpy(curr,frame->payload,frame->size);
		frame = frame->next;
	}
	frame_list->last_packed = last_packed;

	return(xfer_size);
}

int cmq_pkt_unpack_frame_list(unsigned char *buffer, int buffer_size, unsigned char **fl)
{
	CMQFRAMELIST *frame_list;
	CMQPKTHEADER *header;
	unsigned long size;
	unsigned long version;
	unsigned char *curr;
	unsigned char *f;
	unsigned char *l_fl;
	unsigned long xfer_size;
	unsigned long frame_count;
	unsigned int xfer_size;

	if(buffer == NULL) {
		return(-1);
	}

	header = (CMQPKTHEADER *)buffer;
	version = ntohl(header->version);
	if(version != CMQ_PKT_VERSION) { // check version number
		return(-1);
	}

	frame_count = ntohl(header->frame_count);
	if(frame_count == 0) { // this is an ack packet
		*fl = NULL;
		return(0);
	}

	err = cmq_frame_list_create(&l_fl);
	if(err < 0) {
		return(-1);
	}


	xfer_size = sizeof(CMQPKTHEADER);
	curr = buffer + sizeof(CMQPKTHEADER);
	while(xfer_size < buffer_size) {
		size = ntohl(*((unsigned long *)curr));
		if((xfer_size + size) > buffer_size) {
			*fl = l_fl;
			return(0);
		}
		curr += sizeof(unsigned long);
		err = cmq_frame_create(&f,buffer,size);
		if(err < 0) {
			cmq_frame_list_destroy(l_fl);
			return(-1);
		}
		err = cmq_frame_append(l_fl,f);
		if(err < 0) {
			cmq_frameList_destroy(l_fl);
			return(-1);
		}
		curr += size;
		xfer_size += size;
	}

	*fl = l_fl;
	return(0);

}
	

//
// client sends requests to server identified by address and port
// #fl# is a frame_list to send
// frame list can span multiple packets
// response acks may or may not have accompaning payload
// there can be multiple acks
int cmq_pkt_send_rpc(char *dst_addr, unsigned short dst_port, 
	unsigned char *fl, unsigned char **r_fl)
{
	CMQFRAMELIST *frame_list (CMQFRAMELIST *)fl;
	struct sockaddr_in dst;
	int sock_fd;
	int xfer_size;
	int err;
	unsigned char *buffer;
	unsigned int start;
	unsigned int end;
	unsigned int total;
	int xfer_size;
	unsigned char *l_rfl;

	if((dst_addr == NULL) || (fl == NULL)) {
		return(-1);
	}

	sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock_fd < 0) {
		return(-1);
	}
	buffer = (unsigned char *)malloc(CMQ_MAK_PKT_SIZE);
	if(buffer == NULL) {
		return(-1);
	}

	memset(&dst,0,sizeof(dst));
	dst.sin_family = AF_INET;
	dst.sin_port = htons(dst_port);
	dst.sin_addr.s_addr = inet_addr(dst_addr);


	frame_list->last_packed = NULL; // reset packing cursor
	total = 0;

	while(total < frame_list->count) {
		xfer_size = cmq_pkt_pack_frame_list(buffer,CMQ_MAX_PKT_SIZE,fl,&start,&end);
		if(xfer_size < 0) {
			free(buffer);
			return(-1);
		}
		err = sendto(sock_fd,
		     (const char *)buffer,
		     xfer_size,
		     MSG_CONFIRM,
		     (const struct sockaddr *)&dst, 
		     sizeof(dst));

		if(err < 0) {
			return(-1);
		}
		total += ((end - start) + 1);
	}

	l_rfl = NULL;
	start = 1;
	while(recvfrom(sd,buffer,CMQ_MAX_PKT_SIZE,0,NULL,NULL) >= 0) {
		header = (CMQPKTHEADER *)buffer;
		end = ntohl(header->end_fno);
		frame_count = ntohl(header->frame_count)
		if(frame_count > 0) {
			err = cmq_unpack_frame_list(buffer,CMQ_MAX_PKT_SIZE,&l_rfl);
			if(err < 0) {
				free(buffer);
				close(sd);
				return(-1);
			}
		}
		if((end - start + 1) >= frame_list->count) { // if all frames acked
			break;
		}
	}

	*r_fl = l_rfl;
	close(sd);
	free(buffer);
	return(0);
	
}
	
	


	


	
	

	

