#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <spawn.h>

#include "debug.h"

#define SPLAY (0)


int main(int argc,char **argv, char **env)
{
	int err;
	char hbuff[255];
	int i;
	int j;
	int k;
	char *fargv[3]; // in case we are timing
	pid_t pid;
	pid_t npid;
	char *menv[12];
	char args[12*255];
	char *str;
	char c;
	int status;
	int splay_count = 0;
#ifdef TIMING
	double start;
	double end;
	double end1;
#endif
#ifdef TRACK
	int hid;
	char tbuff[255];
#endif

	signal(SIGPIPE, SIG_IGN);

#ifdef DEBUG
	fprintf(stdout,"woofc-forker-helper: running\n");
	fflush(stdout);
#endif

	while(1) {
		/*
		 * we need 11 env variables and a handler
		 */
		memset(args,0,sizeof(args));
		err = read(0,args,sizeof(args));
		if(err <= 0) {
			fprintf(stdout,"woofc-forker-helper read error %d\n",err);
			fflush(stdout);
			exit(0);
		}
		j = 0;
		k = 0;
		for(i=0; i < 11; i++) {
			while((args[j] != 0) && (j < sizeof(args))) { // look for NULL terminator
				j++;
			}
			if(j >= sizeof(args)) {
				fprintf(stdout,"woofc-forker-helper corrupt launch %s\n",args);
				fflush(stdout);
				exit(0);
			}
			menv[i] = &args[k];
#ifdef DEBUG
			fprintf(stdout,"woofc-forker-helper: received %s\n",menv[i]);
			fflush(stdout);
#endif
			j++;
			k = j;
		}
		menv[11] = NULL;

		fargv[0] = &args[k]; // handler is last
#ifdef TIMING
		/*
		 * if we are timing, there is one more
		 */
		j++;
		while((args[j] != 0) && (j < sizeof(args))) { // look for NULL terminator
				j++;
		}
		j++;
		fargv[1] = &args[j];
		fargv[2] = NULL;
#else
		fargv[1] = NULL;
#endif
#ifdef DEBUG
		fprintf(stdout,"woofc-forker-helper: received handler: %s\n",fargv[0]);
		fflush(stdout);
#endif

#ifdef TRACK
		/*
		 * read the hid
		 */
		err = read(0,tbuff,sizeof(tbuff));
		if(err <= 0) {
			fprintf(stderr,"woofc-forker-helper read %d for handler\n",err);
			exit(0);
		}
		hid = atoi(tbuff);
		/*
		 * read the woof name
		 */
		err = read(0,tbuff,sizeof(tbuff));
		if(err <= 0) {
			fprintf(stderr,"woofc-forker-helper read %d for handler\n",err);
			exit(0);
		}

		printf("%s %d RECVD\n",tbuff,hid);
		fflush(stdout);
#endif
		STOPCLOCK(&end1);
		err = posix_spawn(&pid,fargv[0],NULL,NULL,fargv,menv);
		if(err < 0) {
			printf("woof-forker-helper: spawn of %s failed\n",fargv[0]);
		}
		STOPCLOCK(&end);
#ifdef TIMING
		start = strtod(fargv[1],NULL);
#endif
		TIMING_PRINT("PRESPWN %lf [%d]\n",
			DURATION(start,end1)*1000,pid);
		/*
		 * must be printf since stderr is in use
		 */
		TIMING_PRINT("SPAWNED %lf [%d]\n",
			DURATION(start,end)*1000,pid);
		/*
		 * send WooFForker completion signal
		 */
		write(2,&c,1);

		/*
		 * if SPLAY > 0, clean up as best we can when we have over
		 * threshold zombies
		 */
		if(SPLAY != 0) {
			splay_count++;
			if(splay_count >= SPLAY) {
	#ifdef DEBUG
				fprintf(stdout,"woofc-forker-helper: about to wait for %s\n",fargv[0]);
				fflush(stdout);
	#endif
				/*
				 * reap the zombie handlers
				 */
				while((pid = waitpid(-1,&status,WNOHANG)) > 0) {
					splay_count--;
	#ifdef DEBUG
					fprintf(stdout,"woofc-forker-helper: completed wait for %s as proc %d\n",fargv[0],pid);
					fflush(stdout);
	#endif
				}
			}
		}
		/*
		 * if SPLAY is zero, wait
		 */
		if(SPLAY == 0) {
//printf("Helper[%d]: calling wait for pid: %d\n",getpid(),pid);
//fflush(stdout);
			npid = waitpid(pid,&status,0);
			if(npid < 0) {
//printf("Helper[%d]: pid: %d exited with error %d\n",getpid(),npid,errno);
//fflush(stdout);
			}
			if(WIFEXITED(status)) {
//printf("Helper[%d]: pid: %d exited with status %d\n",getpid(),npid,WEXITSTATUS(status));
//fflush(stdout);
			} else if(WIFSIGNALED(status)) {
				if(WTERMSIG(status)) {
//printf("Helper[%d]: pid: %d exited with signal\n",getpid(),npid);
//fflush(stdout);
				} else {
//printf("Helper[%d]: pid: %d exited with signal\n",getpid(),npid);
//fflush(stdout);
				}
			}
		}
			
#ifdef DEBUG
		fprintf(stdout,"woofc-forker-helper: signaled parent for %s after proc %d reaped\n",fargv[0],pid);
		fflush(stdout);
#endif
		fflush(stdout);
	}

	fprintf(stdout,"woofc-forker-helper exiting\n");	
	fflush(stdout);
	exit(0);
}

		
