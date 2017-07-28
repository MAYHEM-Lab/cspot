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
char Host_dir[2048];
char Namelog_name[2048];
unsigned long Name_id;
LOG *Name_log;

#define DEBUG

static int WooFDone;

struct cont_arg_stc
{
	int container_count;
};

typedef struct cont_arg_stc CA;

/*
 * from https://en.wikipedia.org/wiki/Universal_hashing
 */
unsigned long WooFNameHash(char *namespace)
{
	unsigned long h = 5381;
	unsigned long a = 33;
	unsigned long i;

	for(i=0; i < strlen(namespace); i++) {
		h = ((h*a) + namespace[i]); /* no mod p due to wrap */ 
	}

	return(h);
}
void WooFShutdown(int sig)
{
	int val;

	WooFDone = 1;
	while(sem_getvalue(&Name_log->tail_wait,&val) >= 0) {
		if(val > 0) {
			break;
		}
		V(&Name_log->tail_wait);
	}

	return;
}


int WooFInit()
{
	struct timeval tm;
	int err;
	char log_name[2048];
	char log_path[2048];
	char putbuf[25];
	char *str;
	MIO *lmio;
	unsigned long name_id;

	gettimeofday(&tm,NULL);
	srand48(tm.tv_sec+tm.tv_usec);

	str = getenv("WOOFC_DIR");
	if(str == NULL) {
		getcwd(WooF_dir,sizeof(WooF_dir));
	} else {
		strncpy(WooF_dir,str,sizeof(WooF_dir));
	}

	if(strcmp(WooF_dir,"/") == 0) {
		fprintf(stderr,"WooFInit: WOOFC_DIR can't be %s\n",
				WooF_dir);
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

	name_id = WooFNameHash(WooF_dir);

	sprintf(Namelog_name,"cspot-log.%20.20lu",name_id);
	memset(log_name,0,sizeof(log_name));
	sprintf(log_name,"%s/%s",WooF_dir,Namelog_name);

#ifdef DEBUG
	printf("WooFInit: Name log at %s with name %s\n",log_name,Namelog_name);
	fflush(stdout);
#endif

#ifndef IS_PLATFORM
	lmio = MIOReOpen(log_name);
	if(lmio == NULL) {
		fprintf(stderr,"WooFInit: name %s (%s) log not initialized.\n",
			log_name,WooF_dir);
		fflush(stderr);
		exit(1);
	}
	Name_log = MIOAddr(lmio);


#endif
	Name_id = name_id;

	return(1);
}

#ifdef IS_PLATFORM
void *WooFLauncher(void *arg);
void *WooFContainerLauncher(void *arg);

static int WooFHostInit(int min_containers, int max_containers)
{
	char log_name[2048];
	CA *ca;
	int err;
	pthread_t tid;

	WooFInit();

	memset(log_name,0,sizeof(log_name));
	sprintf(log_name,"%s/%s",WooF_dir,Namelog_name);

	Name_log = LogCreate(log_name,Name_id,DEFAULT_WOOF_LOG_SIZE);
	if(Name_log == NULL) {
		fprintf(stderr,"WooFInit: couldn't create name log as %s, size %d\n",log_name,DEFAULT_WOOF_LOG_SIZE);
		fflush(stderr);
		exit(1);
	}

	ca = malloc(sizeof(CA));
	if(ca == NULL) {
		exit(1);
	}

	ca->container_count = min_containers;

	err = pthread_create(&tid,NULL,WooFContainerLauncher,(void *)ca);
	if(err < 0) {
		fprintf(stderr,"couldn't start launcher thread\n");
		exit(1);
	}

	signal(SIGHUP, WooFShutdown);
	pthread_join(tid,NULL);
	return(1);
}

	

void WooFExit()
{       
	WooFDone = 1;
	pthread_exit(NULL);
}

/*
 * FIX ME: it would be better if the TRIGGER seq_no gets 
 * retired after the launch is successful  Right now, the last_seq_no 
 * in the launcher is set before the launch actually happens 
 * which means a failure will not be retried
 */
void *WooFDockerThread(void *arg)
{
	char *launch_string = (char *)arg;

#ifdef DEBUG
	fprintf(stdout,"LAUNCH: %s\n",launch_string);
	fflush(stdout);
#endif
	system(launch_string);
#ifdef DEBUG
	fprintf(stdout,"DONE: %s\n",launch_string);
	fflush(stdout);
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
		P(&Name_log->tail_wait);
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
		P(&Name_log->mutex);
		log_tail = LogTail(Name_log,last_seq_no,Name_log->size);
		V(&Name_log->mutex);
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

		sprintf(launch_string, "docker run -i\
			 -e WOOFC_DIR=%s\
			 -e WOOF_SHEPHERD_NAME=%s\
			 -e WOOF_SHEPHERD_NDX=%lu\
			 -e WOOF_SHEPHERD_SEQNO=%lu\
			 -e WOOF_NAME_ID=%lu\
			 -e WOOF_NAMELOG_NAME=%s\
			 -e WOOF_NAMELOG_SIZE=%lu\
			 -e WOOF_NAMELOG_SEQNO=%lu\
			 -v %s:%s\
			 centos:7\
			 %s/%s",
				woof_shepherd_dir,
				wf->shared->filename,
				ev[first].woofc_ndx,
				ev[first].woofc_seq_no,
				Name_id,
				Namelog_name,
				Name_log->size,
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

void *WooFContainerLauncher(void *arg)
{
	unsigned long last_seq_no = 0;
	unsigned long first;
	LOG *log_tail;
	EVENT *ev;
	char *launch_string;
	char *pathp;
	WOOF *wf;
	int err;
	pthread_t tid;
	int none;
	int count;
	int container_count;
	CA *ca = (CA *)arg;
	pthread_t *tids;

	container_count = ca->container_count;
	free(ca);

#ifdef DEBUG
	fprintf(stdout,"WooFContainerLauncher started\n");
	fflush(stdout);
#endif

	tids = (pthread_t *)malloc(container_count*sizeof(pthread_t));
	if(tids == NULL) {
		fprintf(stderr,"WooFContainerLauncher no space\n");
		exit(1);
	}

	for(count = 0; count < container_count; count++) {
		/*
		 * yield in case other threads need to complete
		 */
#ifdef DEBUG
		fprintf(stdout,"WooFContainerLauncher: launch %d\n", count+1);
		fflush(stdout);
#endif

		/*
		 * find the last directory in the path
		 */
		pathp = strrchr(WooF_dir,'/');
		if(pathp == NULL) {
			fprintf(stderr,"couldn't find leaf dir in %s\n",
				WooF_dir);
			exit(1);
		}

		launch_string = (char *)malloc(2048);
		if(launch_string == NULL) {
			exit(1);
		}

		memset(launch_string,0,2048);

		sprintf(launch_string, "docker run -i\
			 -e WOOFC_DIR=%s\
			 -e WOOF_NAME_ID=%lu\
			 -e WOOF_NAMELOG_NAME=%s\
			 -v %s:%s\
			 centos:7\
			 %s/%s",
				pathp,
				Name_id,
				Namelog_name,
				WooF_dir,pathp,
				pathp,"woofc-container");

		err = pthread_create(&tid,NULL,WooFDockerThread,(void *)launch_string);
		if(err < 0) {
			/* LOGGING
			 * log thread create failure here
			 */
			exit(1);
		}
//		pthread_detach(tid);
		tids[count] = tid;
	}

	for(count=0; count < container_count; count++) {
		pthread_join(tids[count],NULL);
	}

#ifdef DEBUG
		fprintf(stdout,"WooFContainerLauncher exiting\n");
		fflush(stdout);
#endif
	free(tids);
	pthread_exit(NULL);
}
	


#define ARGS "m:M:d:"
char *Usage = "woofc-name-platform -d application woof directory\n\
\t-m min container count\n\
-t-M max container count\n";

int main(int argc, char **argv)
{
	int c;
	int min_containers;
	int max_containers;
	char app_dir[2048];
	char putbuf[3000];

	min_containers = 1;
	max_containers = 1;

	memset(app_dir,0,sizeof(app_dir));
	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'm':
				min_containers = atoi(optarg);
				break;
			case 'M':
				max_containers = atoi(optarg);
				break;
			case 'd':
				strncpy(app_dir,optarg,sizeof(app_dir));
				break;
			default:
				fprintf(stderr,
					"unrecognized command %c\n",
						(char)c);
				fprintf(stderr,"%s",Usage);
				exit(1);
		}
	}

	if(min_containers < 0)  {
		fprintf(stderr,
			"must specify valid number of min containers\n");
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		exit(1);
	}

	if(min_containers > max_containers) {
		fprintf(stderr,"min must be <= max containers\n");
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		exit(1);
	}


	if(app_dir[0] == 0) {
		getcwd(app_dir,sizeof(app_dir));
	}

	memset(putbuf,0,sizeof(putbuf));
	sprintf(putbuf,"WOOFC_DIR=%s",app_dir);
	putenv(putbuf);

	fclose(stdin);
		

	WooFHostInit(min_containers, max_containers);

	pthread_exit(NULL);

	return(0);

}

#endif
