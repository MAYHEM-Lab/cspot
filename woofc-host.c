#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <time.h>

#include "log.h"
#include "woofc.h"

char WooF_dir[2048];
char Host_log_name[2048];
unsigned long Host_id;
LOG *Host_log;

static int WooFDone;

void WooFShutdown(int sig)
{
	int val;

	WooFDone = 1;
	while(sem_getvalue(&Host_log->tail_wait,&val) >= 0) {
		if(val > 0) {
			break;
		}
		V(&Host_log->tail_wait);
	}

	return;
}

	

void *WooFLauncher(void *arg);

int WooFInit(unsigned long host_id)
{
	struct timeval tm;
	int err;
	char log_name[2048];
	char log_path[2048];
	char putbuf[25];
	pthread_t tid;
	char *str;

	gettimeofday(&tm,NULL);
	srand48(tm.tv_sec+tm.tv_usec);

	str = getenv("WOOFC_DIR");
	if(str == NULL) {
		str = DEFAULT_WOOF_DIR;
	}

	if(strcmp(str,".") == 0) {
		fprintf(stderr,"WOOFC_DIR cannot be .\n");
		fflush(stderr);
		exit(1);
	}


	if(str[0] != '/') { /* not an absolute path name */
		getcwd(WooF_dir,sizeof(WooF_dir));
		if(str[0] == '.') {
			strncat(WooF_dir,&(str[1]),
				sizeof(WooF_dir)-strlen(WooF_dir));
		} else {
			strncat(WooF_dir,"/",sizeof(WooF_dir)-strlen(WooF_dir));
			strncat(WooF_dir,str,
				sizeof(WooF_dir)-strlen(WooF_dir));
		}
	} else {
		strncpy(WooF_dir,str,sizeof(WooF_dir));
	}

	if(strcmp(WooF_dir,"/") == 0) {
		fprintf(stderr,"WooFInit: WOOFC_DIR can't be %s\n",
				WooF_dir);
		exit(1);
	}

	if(strlen(str) >= (sizeof(WooF_dir)-1)) {
		fprintf(stderr,"WooFInit: %s too long for directory name\n",
				str);
		exit(1);
	}

	if(WooF_dir[strlen(WooF_dir)-1] == '/') {
		WooF_dir[strlen(WooF_dir)-1] = 0;
	}

	memset(putbuf,0,sizeof(putbuf));
	sprintf(putbuf,"WOOFC_DIR=%s",WooF_dir);
	putenv(putbuf);
#ifdef DEBUG
	fprintf(stdout,"WooFInit: %s\n",putbuf);
	fflush(stdout);
#endif


	err = mkdir(WooF_dir,0600);
	if((err < 0) && (errno != EEXIST)) {
		perror("WooFInit");
		return(-1);
	}

	sprintf(Host_log_name,"cspot-log.%10.10d",host_id);
	memset(log_name,0,sizeof(log_name));
	sprintf(log_name,"%s/%s",WooF_dir,Host_log_name);

	Host_log = LogCreate(log_name,host_id,DEFAULT_WOOF_LOG_SIZE);

	if(Host_log == NULL) {
		fprintf(stderr,"WooFInit: couldn't open log as %s, size %d\n",log_name,DEFAULT_WOOF_LOG_SIZE);
		fflush(stderr);
		exit(1);
	}

	Host_id = host_id;

	err = pthread_create(&tid,NULL,WooFLauncher,NULL);
	if(err < 0) {
		fprintf(stderr,"couldn't start launcher thread\n");
		exit(1);
	}
	pthread_detach(tid);

	signal(SIGHUP, WooFShutdown);
	return(1);
}

void WooFExit()
{       
	WooFDone = 1;
	pthread_exit(NULL);
}
	
/*
 * FIX ME: it would be better if the TRIGGER seq_no gets retired after the launch is successful
 *         Right now, the last_seq_no in the launcher is set before the launch actually happens
 *         which means a failure will nt be retried
 */
void *WooFDockerThread(void *arg)
{
	char *launch_string = (char *)arg;

#ifdef DEBUG
//	fprintf(stdout,"LAUNCH: %s\n",launch_string);
//	fflush(stdout);
#endif
	system(launch_string);
#ifdef DEBUG
//	fprintf(stdout,"DONE: %s\n",launch_string);
//	fflush(stdout);
#endif

	free(launch_string);

	return(NULL);
}

void *WooFLauncher(void *arg)
{
	unsigned long last_seq_no = 0;
	unsigned long first;
	LOG *log_tail;
	EVENT *ev;
	char *launch_string;
	char *pathp;
	WOOF *wf;
	char woof_shepherd_dir[2048];
	int err;
	pthread_t tid;
	int none;

	/*
	 * wait for things to show up in the log
	 */
#ifdef DEBUG
		fprintf(stdout,"WooFLauncher started\n");
		fflush(stdout);
#endif

	while(WooFDone == 0) {
		P(&Host_log->tail_wait);
#ifdef DEBUG
		fprintf(stdout,"WooFLauncher awake\n");
		fflush(stdout);
#endif

		if(WooFDone == 1) {
			break;
		}

		/*
		 * must lock to extract tail
		 */
		P(&Host_log->mutex);
		log_tail = LogTail(Host_log,last_seq_no,Host_log->size);
		V(&Host_log->mutex);
		if(log_tail == NULL) {
#ifdef DEBUG
		fprintf(stdout,"WooFLauncher no tail, continuing\n");
		fflush(stdout);
#endif
			LogFree(log_tail);
			continue;
		}
		if(log_tail->head == log_tail->tail) {
#ifdef DEBUG
		fprintf(stdout,"WooFLauncher log tail empty, continuing\n");
		fflush(stdout);
#endif
			LogFree(log_tail);
			continue;
		}

		ev = (EVENT *)(MIOAddr(log_tail->m_buf) + sizeof(LOG));
#ifdef DEBUG
	first = log_tail->head;
	while(first != log_tail->tail) {
	printf("WooFLauncher: log tail %lu, seq_no: %lu, last %lu\n",first, ev[first].seq_no, last_seq_no);
	fflush(stdout);
		if(ev[first].type == TRIGGER) {
			break;
		}
		first = (first-1);
		if(first >= log_tail->size) {
			first = log_tail->size - 1;
		}
	}
#endif

		/*
		 * find the first TRIGGER we havn't seen yet
		 */
		none = 0;
		first = log_tail->head;
		while((ev[first].type != TRIGGER) ||
		      (ev[first].seq_no <= last_seq_no)) {
			first = (first - 1);
			if(first >= log_tail->size) {
				first=log_tail->size - 1;
			}
			if(first == log_tail->tail) {
				none = 1;
				break;
			}
		}  

		/*
		 * if no TRIGGERS found
		 */
		if(none == 1) {
#ifdef DEBUG
		fprintf(stdout,"WooFLauncher log tail empty, continuing\n");
		fflush(stdout);
#endif
			LogFree(log_tail);
			continue;
		}
		/*
		 * otherwise, fire this event
		 */

#ifdef DEBUG
		fprintf(stdout,"WooFLauncher: firing woof: %s handler: %s woof_seq_no: %lu log_seq_no: %lu\n",
			ev[first].woofc_name,
			ev[first].woofc_handler,
			ev[first].woofc_seq_no,
			ev[first].seq_no);
		fflush(stdout);
#endif


		wf = WooFOpen(ev[first].woofc_name);

		if(wf == NULL) {
			fprintf(stderr,"WooFLauncher: open failed for WooF at %s, %lu %lu\n",
				ev[first].woofc_name,
				ev[first].woofc_element_size,
				ev[first].woofc_history_size);
			fflush(stderr);
			exit(1);
		}

		/*
		 * find the last directory in the path
		 */
		pathp = strrchr(WooF_dir,'/');
		if(pathp == NULL) {
			fprintf(stderr,"couldn't find leaf dir in %s\n",
				WooF_dir);
			exit(1);
		}

		strncpy(woof_shepherd_dir,pathp,sizeof(woof_shepherd_dir));

		launch_string = (char *)malloc(2048);
		if(launch_string == NULL) {
			exit(1);
		}

		memset(launch_string,0,2048);

		sprintf(launch_string, "docker run -it\
			 -e WOOFC_DIR=%s\
			 -e WOOF_SHEPHERD_NAME=%s\
			 -e WOOF_SHEPHERD_NDX=%lu\
			 -e WOOF_SHEPHERD_SEQNO=%lu\
			 -e WOOF_HOST_ID=%lu\
			 -e WOOF_HOST_LOG_NAME=%s\
			 -e WOOF_HOST_LOG_SIZE=%lu\
			 -e WOOF_HOST_LOG_SEQNO=%lu\
			 -v %s:%s\
			 centos:7\
			 %s/%s",
				woof_shepherd_dir,
				wf->shared->filename,
				ev[first].woofc_ndx,
				ev[first].woofc_seq_no,
				Host_id,
				Host_log_name,
				Host_log->size,
				ev[first].seq_no,
				WooF_dir,pathp,
				woof_shepherd_dir,ev[first].woofc_handler);

		/*
		 * remember its sequence number for next time
		 *
		 * needs +1 because LogTail returns the earliest inclusively
		 */
		last_seq_no = ev[first].seq_no; 		/* log seq_no */
#ifdef DEBUG
		fprintf(stdout,"WooFLauncher: seq_no: %lu, handler: %s\n",
			ev[first].seq_no, ev[first].woofc_handler);
		fflush(stdout);
#endif
		LogFree(log_tail); 
		WooFFree(wf);

		err = pthread_create(&tid,NULL,WooFDockerThread,(void *)launch_string);
		if(err < 0) {
			/* LOGGING
			 * log thread create failure here
			 */
			exit(1);
		}
		pthread_detach(tid);
	}

#ifdef DEBUG
		fprintf(stdout,"WooFLauncher exiting\n");
		fflush(stdout);
#endif
	pthread_exit(NULL);
}

	


	
