#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

int main(int argc,char **argv)
{
	int err;
	char hbuff[4096];
	int i;
	char *fargv[2];
	int pid;
	char *env[12];
	char *str;
	char c;
	int status;



	while(1) {
		/*
		 * we need 11 env variables
		 */

		for(i=0; i < 11; i++) {
			err = read(0,hbuff,sizeof(hbuff));
			if(err <= 0) {
				fprintf(stdout,"woofc-forker-helper read %d on %d\n",err,i);
				exit(0);
			}
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
			fprintf(stderr,"woofc-forker-helper: vfork failed\n");
			exit(1);
		} else if(pid == 0) {
			execve(hbuff,fargv,env);
			fprintf(stdout,"execve of %s failed\n",hbuff);
			close(2);
			dup2(1,2);
			perror("woofc-forker-helper");
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

	exit(0);
}

		

	

	

	
