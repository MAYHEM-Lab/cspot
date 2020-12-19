#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>

int main(int argc, char **argv)
{
	char p1[255];
	char p2[10];
	char *pargv[3];
	int pid;
	int err;
	char c;
	int status;

	err = read(0,p1,sizeof(p1));
	while(err > 0) {
		err = read(0,p2,sizeof(p2));
		if(err <= 0) {
			exit(0);
		}
		/*
	 	 * p1 is the name of the handler
		 * p2 is the argument
	 	 */
		pargv[0] = p1;
		pargv[1] = p2;
		pargv[2] = NULL;
		pid = fork();
		if(pid == 0) {
			execve(p1,pargv,NULL);
			fprintf(stdout,"execve failed for %s %s\n",
				p1,p2);
			fflush(stdout);
			close(2);
			dup2(1,2);
			perror("fork-proc");
			exit(1);
		} else if(pid > 0) {
			waitpid(pid,&status,0);
			/*
			 * write single char to stderr to indicate done
			 */
			err = write(2,&c,1);
			if(err < 0) {
				fprintf(stdout,"write failed\n");
				fflush(stdout);
				exit(1);
			}
		} else {
			fprintf(stdout,"fork failed\n");
			fflush(stdout);
			exit(1);
		}
		err = read(0,p1,sizeof(p1));
	}
	close(2);
	exit(0);
			
}

