#ifndef CMQ_FRAME_H
#define CMQ_FRAME_H

#define MAX_FRAME_SIZE (1024*8)

struct cmq_frame_stc
{
	unsigned int size;
	unsigned char *payload;
	int frame_no; // for multipart msg
	struct cmq_frame_stc *next; // single linked list for now
};

typedef struct cmq_frame_stc CMQFRAME;

struct cmq_frame_list_stc
{
	CMQFRAME *head;
	CMQFRAME *tail;
	CMQFRAME *last_packed; // for multipart msg
	unsigned int count;
};

typedef struct cmq_frame_list_stc CMQFRAMELIST;

int cmq_frame_create(unsigned char **frame, unsigned char *ptr, unsigned int len);
void cmq_frame_destroy(unsigned char *frame);
unsigned char *cmq_frame_payload(unsigned char *frame);
unsigned int cmq_frame_size(unsigned char *frame);

int cmq_frame_list_create(unsigned char **fl);
void cmq_frame_list_destroy(unsigned char *fl);
int cmq_frame_pop(unsigned char *fl, unsigned char **frame);
int cmq_frame_append(unsigned char *fl, unsigned char *frame);
int cmq_frame_list_count(unsigned char *fl);
int cmq_frame_list_empty(unsigned char *fl);


#endif

