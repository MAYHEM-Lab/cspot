#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cmq-frame.h"

//
// implements deep free (frees payload)
void cmq_frame_destroy(unsigned char *frame)
{
	if(frame == NULL) {
		return;
	}
	if((CMQFRAME *)frame->payload != NULL) {
		free(frame->payload);
	}
	free(frame);
	return;
}
	

//
// #frame# is an out parameter
// returns -1 on failure and zero on success
// makes a copy if #ptr# is not NULL
int cmq_frame_create(unsigned char **frame, unsigned char *ptr, unsigned int len)
{
	CMQFRAME *f;

	if(len < 0) {
		return(-1);
	}

	if(frame == NULL) {
		return(-1);
	}

	f = (CMQFRAME *)malloc(sizeof(CMQFRAME));
	if(f == NULL) {
		return(-1);
	}

	if(ptr == NULL) {
		memset(f,0,sizeof(CMQFRAME));
		*frame = (unsigned char *)f;
		return(0);
	}

	f->payload = (unsigned char *)malloc(len);
	if(f->ptr == NULL) {
		cmq_frame_destroy(f);
		return(-1);
	}

	f->size = len;
	memcpy(f->payload,ptr,len);
	*frame = (unsigned char *)f;
	return(0);
}

unsigned char *cmq_frame_payload(unsigned char *frame)
{
	if(frame == NULL) {
		return(NULL);
	}
	return((CMQFRAME *)frame->payload);
}

unsigned int cmq_frame_size(unsigned char *frame)
{
	if(frame == NULL) {
		return(0);
	}
	return((CMQFRAME *)frame->size);
}
		
//
// #fl# is an out parameter
// returns -1 on failure and zero on success
int cmq_frame_list_create(unsigned char **fl)
{
	CMQFRAMELIST *frame_list;

	frame_list = (CMQFRAMELIST *)malloc(sizeof(CMQFRAMELIST));
	if(frame_list == NULL) {
		return(-1);
	}

	memset(frame_list,0,sizeof(CMEFRAMELIST));
	*fl = (unsigned char *)frame_list;
	return(0);
}

//
// pops frame from the head
int cmq_frame_pop(unsigned char *fl, unsigned char **frame)
{
	CMQFRAMELIST *frame_list = (CMQFRAMELIST *)fl;
	CMQFRAME *f;

	if(fl == NULL) {
		return(-1);
	}
	if(frame == NULL) {
		return(-1);
	}

	if(frame_list->count == 0) {
		return(-1);
	}
	f = frame_list->head;
	frame_list->head = frame_list->head->next;
	frame_list->count--;

	if(frame_list->count == 0) {
		frame_list->head = NULL;
		frame_list->tail = NULL;
	}

	*frame = (unsigned char *)f;
	return(0);
}
	
//
// appends #frame# to #fl# at the tail
int cmq_frame_append(unsigned char *fl, unsigned char *frame)
{
	CMQFRAMELIST *frame_list = (CMQFRAMELIST *)frame_list;
	CMQFRAME *f = (CMQFRAME *)frame;

	if(fl == NULL) {
		return(-1);
	}
	if(frame == NULL) {
		return(-1);
	}

	if(frame_list->head == NULL) {
		frame_list->head = f;
		frame_list->tail = f;
		frame_list->count = 1;
	} else {
		frame_list->tail->next = f;
		frame_list->tail = f;
		frame_list->count++;
	}


//
// implements deep free
void cmq_frame_list_destroy(unsigned char *fl)
{
	CMQFRAMELIST *frame_list = (CMQFRAMELIST *)fl;
	unsigned char *frame;
	int err;

	if(fl == NULL) {
		return(-1);
	}

	while(frame_list->count != 0) {
		err = cmq_frame_pop(fl,&frame);
		if(err < 0) {
			break;
		}
		cmq_frame_destroy(frame);
	}

	free(fl);
	return;
}
	

#ifdef SELFTEST
int main(int argc, char **argv)
{
	unsigned char *fl;
	unsigned char *f;
	int err;
	char test_payload[4096];

	err = cmq_frame_list_create(&fl);
	if(err < 0) {
		fprintf(stderr,"cmq_frame SELFTEST: FAILED to create frame_list\n");
		exit(1);
	}

	// create an empty frame
	err = cmq_frame_create(&f,NULL,0);
	if(err < 0) {
		fprintf(stderr,"cmq_frame SELFTEST: FAILED to create empty frame\n");
		exit(1);
	}

	if(f == NULL) {
		fprintf(stderr,"cmq_frame SELFTEST: FAILED to return empty frame\n");
		exit(1);
	}

	// destroy empty frame (no way to add payload after the fact -- yet)
	cmq_frame_destroy(f);

	// create a frame and append it to the frame_list
	err = cmq_frame_create(&f,"first frame",strlen(first_frame));
	if(err < 0) {
		fprintf(stderr,"cmq_frame SELFTEST: FAILED to create empty frame\n");
		exit(1);
	}

	// make sure payload is correct after create
	memset(test_payload,0,sizeof(test_payload));
	memcpy(test_payload,(CMQFRAME *)f->payload,(CMQFRAME *)f->size);
	if(strcmp(test_payload,"first frame") != 0) {
		fprintf(stderr,"cmq_frame SELFTEST: payload FAILED after first create\n");
		exit(1);
	}

	// do the append
	err = cmq_frame_append(fl,f);
	if(err < 0) {
		fprintf(stderr,"cmq_frame SELFTEST: first append FAILED\n");
		exit(1);
	}

	// make sure frame list is not empty and count is 1

	if((CMQFRAMELIST *)fl->count != 1) {
		fprintf(stderr,"cmq_frame SELFTEST: first append count != 1\n");
		exit(1);
	}
	if((CMQFRAMELIST *)fl->head != (CMQFRAME *)f) {
		fprintf(stderr,"cmq_frame SELFTEST: first append head != frame\n");
		exit(1);
	}
	if((CMQFRAMELIST *)fl->tail != (CMQFRAME *)f) {
		fprintf(stderr,"cmq_frame SELFTEST: first append tail != frame\n");
		exit(1);
	}

	// pop the frame and test it
	f = NULL;
	err = cmq_frame_pop(fl,&f);
	if(err < 0) {
		fprintf(stderr,"cmq_frame SELFTEST: first pop FAILED\n");
		exit(1);
	}
	if(f == NULL) {
		fprintf(stderr,"cmq_frame SELFTEST: first pop returned NULL\n");
		exit(1);
	}
	memset(test_payload,0,sizeof(test_payload));
	memcpy(test_payload,(CMQFRAME *)f->payload,(CMQFRAME *)f->size);
	if(strcmp(test_payload,"first frame") != 0) {
		fprintf(stderr,"cmq_frame SELFTEST: payload FAILED after first pop\n");
		exit(1);
	}

	cmq_frame_destroy(f);

	// test the frame_list to make sure it is empty
	if((CMQFRAMELIST *)fl->count != 0) {
		fprintf(stderr,"cmq_frame SELFTEST: first pop count != 0\n");
		exit(1);
	}
	if((CMQFRAMELIST *)fl->head != NULL) {
		fprintf(stderr,"cmq_frame SELFTEST: first pop head != NULL\n");
		exit(1);
	}
	if((CMQFRAMELIST *)fl->tail != NULL) {
		fprintf(stderr,"cmq_frame SELFTEST: first pop tail != NULL\n");
		exit(1);
	}

	cmq_frame_list_destroy(fl);

	fprintf(stderr,"cmq_frame SELFTEST: SUCCESS\n");

	exit(0);
}

#endif
	
		



	

	

