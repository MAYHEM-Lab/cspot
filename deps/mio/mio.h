#ifndef MIO_H
#define MIO_H

#include <stdio.h>
#include "mymalloc.h"

struct mio_stc
{
	unsigned char type;
	char *fname;
	char mode[10];	
	FILE *bf;
	int fd;
	void *addr;
	unsigned long int size;
	unsigned int long recs;
	int fields;
	struct mio_stc *text_index;
};

typedef struct mio_stc MIO;

#define MIOTEXT (1)
#define MIODOUBLE (2)

#define MIOLINESIZE (1024*1024) /* max size of input line */
#define MIOSEPARATORS " \n"	/* separator chars for text parsing */

MIO *MIOOpen(char *filename, char *mode, unsigned long int size);
MIO *MIOReOpen(char *filename);
MIO *MIOMalloc(unsigned long int size);
void MIOClose(MIO *mio);
unsigned long int MIOSize(char *file);
unsigned long int MIOFileSize(char *file);
void *MIOAddr(MIO *mio);
int MIOTextFields(MIO *mio);
unsigned long int MIOTextRecords(MIO *mio);

void MIOPrintText(MIO *t_mio);
MIO *MIOOpenText(char *filename, char *mode, unsigned long int size);

MIO *MIODoubleFromText(MIO *t_mio, char *dfname);
MIO *MIOOpenDouble(char *filename, char *mode, unsigned int long size);

int MIOIndexText(MIO *mio);
char *MIOGetText(MIO *mio, int rec, int field);
void MIOSync(MIO *mio);

#endif

