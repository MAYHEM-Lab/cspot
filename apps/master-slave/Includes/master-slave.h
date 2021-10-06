#ifndef MASTER_SLAVE_H
#define MASTER_SLAVE_H

struct pulse_stc
{
	char wname[4096];
	struct timeval tm;
	unsigned long last_seq_no;
};

typedef struct pulse_stc PULSE;

#define IPSTRLEN (15)

struct state_stc
{
	char wname[4096];
	char my_ip[IPSTRLEN+1];
	unsigned char my_state; /* M, S, C, 0 */
	char other_ip[IPSTRLEN+1];
	unsigned char other_state; /* M, S, C, 0 */ 
	char other_color; /* R=>disconnected, G=>connected */
	char client_ip[IPSTRLEN+1];
	char client_color; /* R, G */
};
typedef struct state_stc STATE;

struct status_stc
{
	unsigned char local; /* M, S, C */
	unsigned char remote; /* ~local || C*/
	unsigned long remote_seq_no;
};
typedef struct status_stc STATUS;


struct pingpong_stc
{
	unsigned long seq_no;
	char return_woof[1024];
};
typedef struct pingpong_stc PINGPONG;

#define PPRETRIES (6)
#define PPSLEEP (5)

#define MAKE_EXTENDED_NAME(ename,wname,str) {\
        memset(ename,0,sizeof(ename));\
        sprintf(ename,"%s.%s",wname,str);\
}

#endif

