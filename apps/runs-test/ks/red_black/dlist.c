#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "dlist.h"

DlistNode *DlistNodeInit()
{
	DlistNode *dn;

	dn = (DlistNode *)malloc(sizeof(DlistNode));
	if(dn == NULL)
	{
		return(NULL);
	}

	memset(dn,0,sizeof(DlistNode));

	return(dn);
}

void DlistNodeFree(DlistNode *dn)
{
	free(dn);
}

Dlist *DlistInit()
{
	Dlist *dl;

	dl = (Dlist *)malloc(sizeof(Dlist));
	if(dl == NULL)
	{
		return(NULL);
	}
	memset(dl,0,sizeof(Dlist));

	return(dl);
}

void DlistRemove(Dlist *dl)
{
	DlistNode *dn;
	DlistNode *curr;

	dn = dl->first;

	if(dn != NULL)
	{
		curr = dn->next;
		free(dn);
		while(curr != NULL)
		{
			dn = curr;
			curr = curr->next;
			free(dn);
		}
	}

	free(dl);
}

DlistNode *DlistAppend(Dlist *dl, Hval value)
{
	DlistNode *dn;

	dn = DlistNodeInit();
	if(dn == NULL)
	{
		return(NULL);
	}
	dn->value = value;
	if(dl->count == 0)
	{
		dn->next = NULL;
		dn->prev = NULL;
		dl->first = dn;
		dl->last = dn;
	}
	else
	{
		dn->prev = dl->last;
		dn->next = NULL;
		dl->last->next = dn;
		dl->last = dn;
	}
	
	dl->count++;
	return(dn);
}
		
DlistNode *DlistPrepend(Dlist *dl, Hval value)
{
	DlistNode *dn;

	dn = DlistNodeInit();
	if(dn == NULL)
	{
		return(NULL);
	}
	dn->value = value;
	if(dl->count == 0)
	{
		dn->next = NULL;
		dn->prev = NULL;
		dl->first = dn;
		dl->last = dn;
	}
	else
	{
		dn->next = dl->first;
		dn->prev = NULL;
		dl->first->prev = dn;
		dl->first = dn;
	}
	
	dl->count++;
	return(dn);
}	

void DlistDelete(Dlist *dl, DlistNode *dn)
{
	if(dl->count == 0)
	{
		return;
	}

	if(dl->first == dn)
	{
		dl->first = dl->first->next;
		if(dl->first)
		{
			dl->first->prev = NULL;
		}
	} 
	else if(dl->last == dn)
	{
		dl->last = dl->last->prev;
		if(dl->last)
		{
			dl->last->next = NULL;
		}
	}
	else
	{
		dn->next->prev = dn->prev;
		dn->prev->next = dn->next;
	}

	dl->count--;
	return;
}
		
#ifdef TEST

int main(int argc, char *argv[])
{
	Dlist *dl;
	DlistNode *dn;
	int i;

	dl = DlistInit();
	if(dl == NULL)
	{
		fprintf(stderr,"DlistInit fails\n");
		exit(1);
	}

	printf("appending: ");
	for(i=0; i < 10; i++)
	{
		printf("%d ",i);
		dn = DlistAppend(dl,(Hval)i);
	}
	printf("\nReading\n");
	dn = dl->first;
	while(dn != NULL)
	{
		i = dn->value.i;
		printf("%d ",i);
		dn = dn->next;
	}
	printf("\nremoving Dlist\n");
	DlistRemove(dl);

	dl = DlistInit();
	if(dl == NULL)
	{
		fprintf(stderr,"DlistInit fails\n");
		exit(1);
	}

	printf("prepending ");
	for(i=0; i < 10; i++)
	{
		printf("%d ",i);
		dn = DlistPrepend(dl,(Hval)i);
	}
	printf("\nReading\n");
	dn = dl->first;
	while(dn != NULL)
	{
		i = dn->value.i;
		printf("%d ",i);
		dn = dn->next;
	}
	printf("\nremoving Dlist\n");
	DlistRemove(dl);

	dl = DlistInit();
	if(dl == NULL)
	{
		fprintf(stderr,"DlistInit fails\n");
		exit(1);
	}

	printf("prepending ");
	for(i=0; i < 10; i++)
	{
		printf("%d ",i);
		dn = DlistPrepend(dl,(Hval)i);
	}

	dn = dl->first;
	for(i=0; i < 5; i++)
	{
		dn = dn->next;
	}
	printf("\ndeleting %d\n",dn->value.i);
	DlistDelete(dl,dn);
	dn = dl->first;
	while(dn != NULL)
	{
		i = dn->value.i;
		printf("%d ",i);
		dn = dn->next;
	}
	printf("\nremoving Dlist\n");
	DlistRemove(dl);

	return(0);
}
#endif
