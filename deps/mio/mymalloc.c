#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

void *Malloc(unsigned int size)
{
	unsigned char *c;

	c = (unsigned char *)malloc(size);
	if(c == NULL) {
		return(NULL);
	}

	memset(c,0,size*sizeof(unsigned char));

	return((void *)c);
}

void Free(void *p)
{
	free(p);
}
