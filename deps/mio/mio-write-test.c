#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "mio.h"

#define ARGS "w:"
char *Usage = "usage: mio-write-test -w writefile\n";
char Wfile[4096];

int main(int argc, char **argv)
{
	int c;
	MIO *mio;
	unsigned long int size;
	int err;
	char *string;


	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch (c) {
			case 'w':
				strncpy(Wfile,optarg,sizeof(Wfile));
				break;
			default:
				fprintf(stderr,
			"mio-test unrecognized command: %c\n",
					(char)c);
				fprintf(stderr,"%s",Usage);
				exit(1);
		}
	}

	if(Wfile[0] == 0) {
		fprintf(stderr,"need a file\n");
		fprintf(stderr,"%s",Usage);
		exit(1);
	} 

	size = 8196;

	printf("attempting open for %s for write with size %ld\n",
			Wfile,
			size);
	mio = MIOOpenText(Wfile,"a+",size);
	if(mio == NULL) {
		fprintf(stderr,"open text failed\n");
		exit(1);
	}
	printf("attempting open for %s complete\n",Wfile);

	string = (char *)MIOAddr(mio);
	strcpy(string,"Hellow world\n");

	printf("attempting close for %s for write\n",
			Wfile);
	MIOClose(mio);

	return(0);
}

