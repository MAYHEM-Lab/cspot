#ifndef CMQ_FRAME_H
#define CMQ_FRAME_H

#define MAX_FRAME_SIZE (1024*8)

struct cmq_frame_stc
{
	unsigned int size;
	unsigned char *payload;
	struct cmq_frame_stc *next; // single linked list for now
};

typedef struct cmq_frame_stc CMQFRAME;

struct cmq_frame_list_stc
{
	CMQFRAME *head;
	CMQFRAME *tail;
	unsigned int count;
	unsigned int max_size;
};

typedef struct cmq_frame_list_stc CMQFRAMELIST;

#ifdef __cplusplus
extern "C" {
#endif
int cmq_frame_create(unsigned char **frame, unsigned char *ptr, unsigned int len);
void cmq_frame_destroy(unsigned char *frame);
unsigned char *cmq_frame_payload(unsigned char *frame);
unsigned int cmq_frame_size(unsigned char *frame);

int cmq_frame_list_create(unsigned char **fl);
void cmq_frame_list_destroy(unsigned char *fl);
int cmq_frame_pop(unsigned char *fl, unsigned char **frame);
int cmq_frame_append(unsigned char *fl, unsigned char *frame);
int cmq_frame_list_count(unsigned char *fl);
int cmq_frame_list_max_size(unsigned char *fl);
int cmq_frame_list_empty(unsigned char *fl);
unsigned char *cmq_frame_list_head(unsigned char *fl);
unsigned char *cmq_frame_list_tail(unsigned char *fl);
unsigned char *cmq_frame_next(unsigned char *f);

#ifdef __cplusplus
}
#endif


#endif

