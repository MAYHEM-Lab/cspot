#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "mio.h"

#define ARGS "r:"
char *Usage = "usage: mio-test -r readfile\n";
char Rfile[4096];

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
	long rec;
	int field;
	char **index;
	char *text_field;
	int err;


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

	printf("attempting open for %s for read with size %ld\n",
			Rfile,
			size);
	mio = MIOOpenText(Rfile,"r",size);
	printf("attempting open for %s complete\n",Rfile);

	printf("attempting text field count\n");
	fields = MIOTextFields(mio);
	printf("file %s has %d text fields\n",Rfile,fields);

	printf("attempting text rec count\n");
	recs = MIOTextRecords(mio);
	printf("file %s has %ld text fields\n",Rfile,recs);


	MIOPrintText(mio);

	printf("attempting conversion to doubles\n");
	d_mio = MIODoubleFromText(mio,NULL);
	printf("conversion to doubles complete\n");
	darray = (double *)MIOAddr(d_mio);
	if(darray == NULL) {
		printf("double conversion has no array\n");
		exit(1);
	}
	for(i=0; i < recs; i++) {
		for(j=0; j < fields; j++) {
			printf("%f ",darray[i*fields+j]);
		}
		printf("\n");
	}
	

	printf("attempting close for %s for read\n",
			Rfile);
	MIOClose(mio);

	printf("attempting close for for double\n");
	MIOClose(d_mio);

	printf("attempting text open again\n");
	mio = MIOOpenText(Rfile,"r",size);
	if(mio == NULL) {
		printf("second open failed\n");
		exit(1);
	}
	err = MIOIndexText(mio);
	if(err < 0) {
		printf("MIOIndexText failed\n");
		exit(1);
	}
	index = (char **)MIOAddr(mio->text_index);
	printf("printing text fields\n");
	for(rec=0; rec < mio->recs; rec++) {
		for(field=0; field < mio->fields; field++) {
			text_field = MIOGetText(mio,rec,field);
			if(text_field == NULL) {
			printf("no text field for rec: %ld, field: %d\n",
				rec,field);
				exit(1);
			}
			if(field < (mio->fields-1)) {
				printf("%s ",text_field);
			} else {
				printf("%s",text_field);
			}
			Free(text_field);
		}
		printf("\n");
	}
	printf("closing text mio\n");
	MIOClose(mio);
		

	return(0);
}

