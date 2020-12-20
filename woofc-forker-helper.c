#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#define SPLAY (5)

int main(int argc,char **argv, char **env)
{
	int err;
	char hbuff[255];
	int i;
	char *fargv[2];
	int pid;
	char *menv[12];
	char *str;
	char c;
	int status;
	int splay_count = 0;

	signal(SIGPIPE, SIG_IGN);


#ifdef DEBUG
	fprintf(stdout,"woofc-forker-helper: running\n");
	fflush(stdout);
#endif

	while(1) {
		/*
		 * we need 11 env variables
		 */

		for(i=0; i < 11; i++) {
			err = read(0,hbuff,sizeof(hbuff));
			if(err <= 0) {
				fprintf(stdout,"woofc-forker-helper read %d on %d\n",err,i);
				fflush(stdout);
				exit(0);
			}
#ifdef DEBUG
			fprintf(stdout,"woofc-forker-helper: received %s\n",hbuff);
			fflush(stdout);
#endif
			str = (char *)malloc(strlen(hbuff)+1);
			if(str == NULL) {
				exit(1);
			}
			memset(str,0,strlen(hbuff)+1);
			strncpy(str,hbuff,strlen(hbuff)+1);
			menv[i] = str;
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
		pid = vfork();
		if(pid < 0) {
			fprintf(stdout,"woofc-forker-helper: vfork failed\n");
			fflush(stdout);
			exit(1);
		} else if(pid == 0) {
			execve(hbuff,fargv,menv);
			fprintf(stdout,"execve of %s failed\n",hbuff);
			fflush(stdout);
			close(2);
			exit(0);
		}
#ifdef DEBUG
		fprintf(stdout,"woofc-forker-helper: vfork completed for handler %s\n",hbuff);
		fflush(stdout);
#endif
		/*
		 * free strings in menv pointer
		 */
		for(i=0; i < 11; i++) {
			free(menv[i]);
		}

		/*
		 * wait for handler to exit
		 */
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
		/*
		 * send WooFForker completion signal
		 */
		write(2,&c,1);
#ifdef DEBUG
		fprintf(stdout,"woofc-forker-helper: signaled parent for %s after proc %d reaped\n",hbuff,pid);
		fflush(stdout);
#endif
	}

	fprintf(stdout,"woofc-forker-helper exiting\n");	
	fflush(stdout);
	exit(0);
}

		
