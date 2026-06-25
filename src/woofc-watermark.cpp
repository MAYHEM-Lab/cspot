#include <cstdlib>
#include <unistd.h>
#include <string.h>
extern "C" {
#include "log.h"
#include "woofc.h"
}

#include "debug.h"
#include "global.h"
#include "net.h"
#include "woofc-access.h"
#include "woofc-priv.h"

extern void WooFWatermark(char *name);

#define ARGS "W:"
char *Usage = "woofc-watermark -W local-woof-name\n";

char Fname[2032];

int main(int argc, char**argv)
{
	int c;

	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'W':
				strncpy(Fname,optarg,sizeof(Fname));
				break;
			default:
				fprintf(stderr,"unrecognized command %c\n",(char)c);
				fprintf(stderr,"%s",Usage);
				exit(1);
		}
	}
	if(Fname[0] == 0) {
		fprintf(stderr,"must specify local woof name command\n");
		fprintf(stderr,"%s",Usage);
		exit(1);
	}
	WooFInit();
	WooFWatermark(Fname);
	exit(0);
}


