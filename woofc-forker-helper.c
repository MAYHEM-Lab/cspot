#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define DEBUG

int main(int argc,char **argv, char **envp)
{
	int err;
	char hbuff[1024];
	int i;
	char *fargv[2];
	int pid;
	char *env[12];
	char *str;
	char c;
	int status;
	int bad;
	FILE *log;
	char obuf[255];

	memset(obuf,0,sizeof(obuf));
	sprintf(obuf,"/pt/forker.%d.log",getpid());

	log = fopen(obuf,"a+");
	if(log == NULL) {
		bad = open("/tmp/bad.txt",O_CREAT|O_RDWR,0600);
		if(bad >= 0) {
			write(bad,"log failed",strlen("log failed"));
			close(bad);
		}
		exit(1);
	}


#ifdef DEBUG
	fprintf(log,"woofc-forker-helper: running\n");
	fflush(log);
#endif

	while(1) {
		/*
		 * we need 11 env variables
		 */

		for(i=0; i < 11; i++) {
			err = read(0,hbuff,sizeof(hbuff));
			if(err <= 0) {
				fprintf(log,"woofc-forker-helper read %d on %d\n",err,i);
				fflush(log);
				exit(0);
			}
#ifdef DEBUG
			fprintf(log,"woofc-forker-helper: received %s\n",hbuff);
			fflush(log);
#endif
			str = (char *)malloc(strlen(hbuff)+1);
			if(str == NULL) {
				exit(1);
			}
			memset(str,0,strlen(hbuff)+1);
			strncpy(str,hbuff,strlen(hbuff)+1);
			env[i] = str;
		}
		env[11] = NULL;

		/*
		 * read the handler name
		 */
		err = read(0,hbuff,sizeof(hbuff));
		if(err <= 0) {
			fprintf(stderr,"woofc-forker-helper read %d for handler\n",err);
			exit(0);
		}

		fargv[0] = hbuff;
		fargv[1] = NULL;
		pid = vfork();
		if(pid < 0) {
			fprintf(log,"woofc-forker-helper: vfork failed\n");
			fflush(log);
			exit(1);
		} else if(pid == 0) {
			execve(hbuff,fargv,env);
			fprintf(log,"execve of %s failed\n",hbuff);
			fflush(log);
			close(2);
			exit(0);
		}
		/*
		 * free strings in env pointer
		 */
		for(i=0; i < 11; i++) {
			free(env[i]);
		}

		/*
		 * wait for handler to exit
		 */
		err = waitpid(pid,&status,0);
		/*
		 * send WooFForker completion signal
		 */
		write(2,&c,1);
	}

	fprintf(log,"woofc-forker-helper exiting\n");	
	fflush(log);
	fclose(log);
	exit(0);
}

		
