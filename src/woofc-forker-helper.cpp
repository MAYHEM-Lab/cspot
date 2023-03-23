#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>

#include "debug.h"

#define SPLAY (0)


int main(int argc,char **argv, char **env)
{
	int err;
	char hbuff[255];
	int i;
	char *fargv[2];
	pid_t pid;
	pid_t npid;
	char *menv[12];
	char args[12*255];
	char *str;
	char c;
	int status;
	int splay_count = 0;
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
		 * we need 11 env variables
		 */

		memset(args,0,12*255);
		for(i=0; i < 11; i++) {
//			err = read(0,hbuff,sizeof(hbuff));
			err = read(0,&args[i*255],255);
			menv[i] = &args[i*255];
			if(err <= 0) {
				fprintf(stdout,"woofc-forker-helper read %d on %d\n",err,i);
				fflush(stdout);
				exit(0);
			}
#if 0
			str = (char *)malloc(strlen(hbuff)+1);
			if(str == NULL) {
				exit(1);
			}
			memset(str,0,sizeof(hbuff)+1);
			strncpy(str,hbuff,strlen(hbuff));
#endif
#ifdef DEBUG
			fprintf(stdout,"woofc-forker-helper: received %s\n",menv[i]);
			fflush(stdout);
#endif
//			menv[i] = str;
		}
		menv[11] = NULL;

		/*
		 * read the handler name
		 */
		err = read(0,hbuff,sizeof(hbuff));
		if(err <= 0) {
			fprintf(stderr,"woofc-forker-helper read %d for handler\n",err);
			exit(0);
		}
#ifdef DEBUG
		fprintf(stdout,"woofc-forker-helper: received handler: %s\n",hbuff);
		fflush(stdout);
#endif

		fargv[0] = hbuff;
		fargv[1] = NULL;

#ifdef TRACK
		/*
		 * read the hid
		 */
		err = read(0,tbuff,sizeof(hbuff));
		if(err <= 0) {
			fprintf(stderr,"woofc-forker-helper read %d for handler\n",err);
			exit(0);
		}
		hid = atoi(tbuff);
		/*
		 * read the woof name
		 */
		err = read(0,tbuff,sizeof(hbuff));
		if(err <= 0) {
			fprintf(stderr,"woofc-forker-helper read %d for handler\n",err);
			exit(0);
		}

		printf("%s %d RECVD\n",tbuff,hid);
		fflush(stdout);
#endif
		pid = vfork();
//		pid = fork();
		if(pid < 0) {
			fprintf(stdout,"woofc-forker-helper: vfork failed\n");
			fflush(stdout);
			exit(1);
		} else if(pid == 0) {
			/*
		 	 * reset stderr for handler to use
		 	 */
			close(2);
			dup2(1,2);
			/*
			 * close 0 in case of SIGPIPE
			 */
			close(0);
			execve(hbuff,fargv,menv);
			fprintf(stdout,"execve of %s failed\n",hbuff);
			fflush(stdout);
			close(2);
			_exit(0);
		} else {
//printf("Helper[%d]: forked pid %d, %s\n",getpid(),pid,hbuff);
//fflush(stdout);
		}

#ifdef DEBUG
		fprintf(stdout,"woofc-forker-helper: vfork completed for handler %s\n",hbuff);
		fflush(stdout);
#endif
#ifdef DEBUG
		fprintf(stdout,"woofc-forker-helper: freed env\n");
		fflush(stdout);
#endif

		/*
		 * wait for handler to exit
		 */
#if 0
		if(SPLAY != 0) {
			splay_count++;
			if(splay_count >= SPLAY) {
	#ifdef DEBUG
				fprintf(stdout,"woofc-forker-helper: about to wait for %s\n",hbuff);
				fflush(stdout);
	#endif
				/*
				 * reap the zombie handlers
				 */
				while((pid = waitpid(-1,&status,WNOHANG)) > 0) {
					splay_count--;
	#ifdef DEBUG
					fprintf(stdout,"woofc-forker-helper: completed wait for %s as proc %d\n",hbuff,pid);
					fflush(stdout);
	#endif
				}
			}
		}
#endif
		/*
		 * send WooFForker completion signal
		 */
		write(2,&c,1);
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
		fprintf(stdout,"woofc-forker-helper: signaled parent for %s after proc %d reaped\n",hbuff,pid);
		fflush(stdout);
#endif
		fflush(stdout);
	}

	fprintf(stdout,"woofc-forker-helper exiting\n");	
	fflush(stdout);
	exit(0);
}

		
