#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cmq-frame.h"

//
// implements deep free (frees payload)
void cmq_frame_destroy(unsigned char *f)
{
	CMQFRAME *frame = (CMQFRAME *)f;
	if(frame == NULL) {
		return;
	}
	if(frame->payload != NULL) {
		free(frame->payload);
	}
	free(f);
	return;
}
	

//
// #frame# is an out parameter
// returns -1 on failure and zero on success
// makes a copy if #ptr# is not NULL
int cmq_frame_create(unsigned char **f, unsigned char *ptr, unsigned int len)
{
	CMQFRAME *frame;

	if(len < 0) {
		return(-1);
	}

	if(f == NULL) {
		return(-1);
	}

	frame = (CMQFRAME *)malloc(sizeof(CMQFRAME));
	if(frame == NULL) {
		return(-1);
	}

	if(ptr == NULL) {
		memset(frame,0,sizeof(CMQFRAME));
		*f = (unsigned char *)frame;
		return(0);
	}

	frame->payload = (unsigned char *)malloc(len);
	if(frame->payload == NULL) {
		cmq_frame_destroy((unsigned char *)frame);
		return(-1);
	}

	frame->size = len;
	memcpy(frame->payload,ptr,len);
	*f = (unsigned char *)frame;
	return(0);
}

unsigned char *cmq_frame_payload(unsigned char *f)
{
	CMQFRAME *frame = (CMQFRAME *)f;
	if(frame == NULL) {
		return(NULL);
	}
	return(frame->payload);
}

unsigned int cmq_frame_size(unsigned char *f)
{
	CMQFRAME *frame = (CMQFRAME *)f;
	if(frame == NULL) {
		return(0);
	}
	return(frame->size);
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

	memset(frame_list,0,sizeof(CMQFRAMELIST));
	*fl = (unsigned char *)frame_list;
	return(0);
}

//
// pops frame from the head
int cmq_frame_pop(unsigned char *fl, unsigned char **f)
{
	CMQFRAMELIST *frame_list = (CMQFRAMELIST *)fl;
	CMQFRAME *frame;

	if(fl == NULL) {
		return(-1);
	}
	if(f == NULL) {
		return(-1);
	}

	if(frame_list->count == 0) {
		return(-1);
	}
	frame = frame_list->head;
	frame_list->head = frame_list->head->next;
	frame_list->count--;

	if(frame_list->count == 0) {
		frame_list->head = NULL;
		frame_list->tail = NULL;
	}

	*f = (unsigned char *)frame;
	return(0);
}
	
//
// appends #frame# to #fl# at the tail
int cmq_frame_append(unsigned char *fl, unsigned char *f)
{
	CMQFRAMELIST *frame_list = (CMQFRAMELIST *)fl;
	CMQFRAME *frame = (CMQFRAME *)f;

	if(fl == NULL) {
		return(-1);
	}
	if(f == NULL) {
		return(-1);
	}

	if(frame_list->head == NULL) {
		frame_list->head = frame;
		frame_list->tail = frame;
		frame_list->count = 1;
	} else {
		frame_list->tail->next = frame;
		frame_list->tail = frame;
		frame_list->count++;
	}

	return(0);
}


//
// implements deep free
void cmq_frame_list_destroy(unsigned char *fl)
{
	CMQFRAMELIST *frame_list = (CMQFRAMELIST *)fl;
	unsigned char *frame;
	int err;

	if(fl == NULL) {
		return;
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
	
int cmq_frame_list_count(unsigned char *fl)
{
	CMQFRAMELIST *frame_list = (CMQFRAMELIST *)fl;
	if(fl == NULL) {
		return(0);
	}

	return(frame_list->count);
}

int cmq_frame_list_empty(unsigned char *fl)
{
	CMQFRAMELIST *frame_list = (CMQFRAMELIST *)fl;
	if(frame_list->head == NULL) {
		return(1);
	}
	return(0);
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
	err = cmq_frame_create(&f,(unsigned char *)"first frame",strlen("first frame"));
	if(err < 0) {
		fprintf(stderr,"cmq_frame SELFTEST: FAILED to create frame with payload\n");
		exit(1);
	}

	// make sure payload is correct after create
	memset(test_payload,0,sizeof(test_payload));
	memcpy(test_payload,cmq_frame_payload(f),cmq_frame_size(f));
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

	if(cmq_frame_list_count(fl) != 1) {
		fprintf(stderr,"cmq_frame SELFTEST: first append count != 1\n");
		exit(1);
	}
	if(cmq_frame_list_empty(fl) == 1) {
		fprintf(stderr,"cmq_frame SELFTEST: first append results in empty framelist\n");
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
	memcpy(test_payload,cmq_frame_payload(f),cmq_frame_size(f));
	if(strcmp(test_payload,"first frame") != 0) {
		fprintf(stderr,"cmq_frame SELFTEST: payload FAILED after first pop\n");
		exit(1);
	}

	cmq_frame_destroy(f);

	// test the frame_list to make sure it is empty
	if(cmq_frame_list_count(fl) != 0) {
		fprintf(stderr,"cmq_frame SELFTEST: first pop count != 0\n");
		exit(1);
	}
	if(cmq_frame_list_empty(fl) != 1) {
		fprintf(stderr,"cmq_frame SELFTEST: first pop head nonempty\n");
		exit(1);
	}


	//
	// add three strings and then pop them, one at a time
	err = cmq_frame_create(&f,(unsigned char *)"first frame",strlen("first frame"));
	if(err < 0) {
		fprintf(stderr,"cmq_frame SELFTEST: FAILED to create first in list\n");
		exit(1);
	}
	err = cmq_frame_append(fl,f);
	if(err < 0) {
		fprintf(stderr,"cmq_frame SELFTEST: FAILED to append first in list\n");
		exit(1);
	}

	err = cmq_frame_create(&f,(unsigned char *)"second frame",strlen("second frame"));
	if(err < 0) {
		fprintf(stderr,"cmq_frame SELFTEST: FAILED to create second in list\n");
		exit(1);
	}
	err = cmq_frame_append(fl,f);
	if(err < 0) {
		fprintf(stderr,"cmq_frame SELFTEST: FAILED to append second in list\n");
		exit(1);
	}

	err = cmq_frame_create(&f,(unsigned char *)"third frame",strlen("third frame"));
	if(err < 0) {
		fprintf(stderr,"cmq_frame SELFTEST: FAILED to create third in list\n");
		exit(1);
	}
	err = cmq_frame_append(fl,f);
	if(err < 0) {
		fprintf(stderr,"cmq_frame SELFTEST: FAILED to append third in list\n");
		exit(1);
	}

	while(cmq_frame_list_count(fl) != 0) {
		memset(test_payload,0,sizeof(test_payload));
		err = cmq_frame_pop(fl,&f);
		if(err < 0) {
			fprintf(stderr,"cmq_frame SELFTEST: FAILED to pop with count %d\n",
				cmq_frame_list_count(fl));
			exit(1);
		}
		if(cmq_frame_list_count(fl) == 2) {
			memcpy(test_payload,cmq_frame_payload(f),strlen("first frame"));
			if(memcmp(test_payload,
			   cmq_frame_payload(f),
			   strlen("first frame")) != 0) {
				fprintf(stderr,
				"cmq_frame_list SELFTEST: FAILED payload first pop\n");
					exit(1);
			  }
		} else {
			cmq_frame_destroy(f);
			continue;
		}
		if(cmq_frame_list_count(fl) == 1) {
			memcpy(test_payload,cmq_frame_payload(f),strlen("second frame"));
			if(memcmp(test_payload,
			   cmq_frame_payload(f),
			   strlen("second frame")) != 0) {
				fprintf(stderr,
				"cmq_frame_list SELFTEST: FAILED payload second pop\n");
					exit(1);
			  }
		} else {
			cmq_frame_destroy(f);
			continue;
		}
		if(cmq_frame_list_count(fl) == 0) {
			memcpy(test_payload,cmq_frame_payload(f),strlen("third frame"));
			if(memcmp(test_payload,
			   cmq_frame_payload(f),
			   strlen("second frame")) != 0) {
				fprintf(stderr,
				"cmq_frame_list SELFTEST: FAILED payload third pop\n");
					exit(1);
			  }
		} else {
			cmq_frame_destroy(f);
			continue;
		}
	}
	
	cmq_frame_list_destroy(fl);

	fprintf(stderr,"cmq_frame SELFTEST: SUCCESS\n");

	exit(0);
}

#endif
	
		



	

	

