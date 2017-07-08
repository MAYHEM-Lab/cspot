#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>

#include "log.h"
#include "woofc.h"

static char *WooF_dir;
static char Host_log_name[2048];
static unsigned long Host_id;
static LOG *Host_log;

static int WooFDone;

void *WooFLauncher(void *arg);

int WooFInit(unsigned long host_id)
{
	struct timeval tm;
	int err;
	char log_name[2048];
	char log_path[2048];
	pthread_t tid;

	gettimeofday(&tm,NULL);
	srand48(tm.tv_sec+tm.tv_usec);

	WooF_dir = getenv("WOOFC_DIR");
	if(WooF_dir == NULL) {
		WooF_dir = DEFAULT_WOOF_DIR;
	}

	if(strcmp(WooF_dir,"/") == 0) {
		fprintf(stderr,"WooFInit: WOOFC_DIR can't be %s\n",
				WooF_dir);
		exit(1);
	}

	err = mkdir(WooF_dir,0600);
	if((err < 0) && (errno != EEXIST)) {
		perror("WooFInit");
		return(-1);
	}

	sprintf(Host_log_name,"cspot-log.%10.0d",host_id);
	memset(log_name,0,sizeof(log_name));
	sprintf(log_name,"%s%s",WooF_dir,Host_log_name);

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

	system(launch_string);
	free(launch_string);

	return(NULL);
}

void *WooFLauncher(void *arg)
{
	unsigned long last_seq_no = 1;
	unsigned long first;
	LOG *log_tail;
	EVENT *ev;
	char *launch_string;
	char *pathp;
	WOOF *wf;
	char woof_shepherd_dir[2048];
	int err;
	pthread_t tid;

	/*
	 * wait for things to show up in the log
	 */

	while(WooFDone != 0) {
		P(&Host_log->tail_wait);

		/*
		 * must lock to extract tail
		 */
		P(&Host_log->mutex);
		log_tail = LogTail(Host_log,last_seq_no,Host_log->size);
		V(&Host_log->mutex);
		if(log_tail == NULL) {
			LogFree(log_tail);
			continue;
		}
		if(log_tail->head == log_tail->tail) {
			LogFree(log_tail);
			continue;
		}

		ev = (EVENT *)(MIOAddr(log_tail->m_buf) + sizeof(LOG));

		/*
		 * find the first TRIGGER we havn't seen yet
		 */
		first = log_tail->head;
		while(ev[first].type != TRIGGER) {
			first++;
			if(first == log_tail->tail) {
				break;
			}
		}  

		/*
		 * if no TRIGGERS found
		 */
		if(log_tail->tail == first) {
			LogFree(log_tail);
			continue;
		}
		/*
		 * otherwise, fire this event
		 */


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
			 -e WOOF_SHEPHERD_DIR=%s\
			 -e WOOF_SHEPHERD_NAME=%s\
			 -e WOOF_SHEPHERD_NDX=%lu\
			 -e WOOF_SHEPHERD_SEQNO=%lu\
			 -e WOOF_HOST_ID=%lu\
			 -e WOOF_HOST_LOG_NAME=%s\
			 -e WOOF_HOST_LOG_SIZE=%lu\
			 -e WOOF_HOST_LOG_SEQNO=%lu\
			 -v %s:%s\
			 centos:7\
			 %s",
				woof_shepherd_dir,
				wf->filename,
				ev[first].woofc_ndx,
				ev[first].woofc_seq_no,
				Host_id,
				Host_log->filename,
				Host_log->size,
				ev[first].seq_no,
				WooF_dir,pathp,
				wf->handler_name);

		/*
		 * remember its sequence number for next time
		 */
		last_seq_no = ev[first].seq_no; 		/* log seq_no */
		LogFree(log_tail);
		err = pthread_create(&tid,NULL,WooFDockerThread,(void *)launch_string);
		if(err < 0) {
			/* LOGGING
			 * log thread create failure here
			 */
			exit(1);
		}
		pthread_detach(tid);
		WoofClose(wf);	/* shepherd will repopen */
	}

	pthread_exit(NULL);
}

	


	
