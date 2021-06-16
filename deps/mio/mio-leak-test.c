#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "mio.h"

#define ITERATIONS (70000)

#define ARGS "r:"
char *Usage = "usage: mio-leak-test -r readfile\n";
char Rfile[4096];

int yan()
{
	return(1);
}

int main(int argc, char **argv)
{
	int c;
	double *values;
	int i;
	int j;
	MIO *mio;
	MIO *d_mio;
	unsigned long int size;
	int fields;
	unsigned long int recs;
	double *darray;


	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch (c) {
			case 'r':
				strncpy(Rfile,optarg,sizeof(Rfile));
				break;
			default:
				fprintf(stderr,
			"mio-test unrecognized command: %c\n",
					(char)c);
				fprintf(stderr,"%s",Usage);
				exit(1);
		}
	}

	if(Rfile[0] == 0) {
		fprintf(stderr,"need a file\n");
		fprintf(stderr,"%s",Usage);
		exit(1);
	} 

	size = MIOSize(Rfile);
	if((long int)size < 0) {
		fprintf(stderr,"MIOFileSize failed for file %s\n",
			Rfile);
		exit(1);
	}

	printf("attempting %d iterations of text read and convert\n",
		ITERATIONS);
	for(i=0; i < ITERATIONS; i++) {
		if(i == 125) {
			yan();
		}
		mio = MIOOpenText(Rfile,"r",size);
		if(mio == NULL) {
			fprintf(stderr,
			"open failed on iter %d\n",i);
			exit(1);
		}
		fields = MIOTextFields(mio);
		recs = MIOTextRecords(mio);
		d_mio = MIODoubleFromText(mio,NULL);
		if(d_mio == NULL) {
			fprintf(stderr,
			"convert failed on iter %d\n",i);
			exit(1);
		}
		MIOClose(mio);
		MIOClose(d_mio);
	}
	printf("done\n");

	return(0);
}


	

	
	

