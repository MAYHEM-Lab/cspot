#ifndef DLIST_2_H
#define DLIST_2_H

#include "hval_2.h"

struct dlist_node_stc
{
	struct dlist_node_stc *next;
	struct dlist_node_stc *prev;
	Hval value;
};

typedef struct dlist_node_stc DlistNode;

struct dlist_stc
{
	DlistNode *first;
	DlistNode *last;
//	struct dlist_node_stc dummy;	/* JRB compatability */
	int count;
};

typedef struct dlist_stc Dlist;

Dlist *DlistInit();
void DlistRemove(Dlist *dl);
DlistNode *DlistAppend(Dlist *dl, Hval value);
DlistNode *DlistPrepend(Dlist *dl, Hval value);
void DlistDelete(Dlist *dl, DlistNode *dn);

#define DLIST_FORWARD(list,curr)\
for((curr)=(list)->first; (curr)!=NULL; (curr) = (curr)->next)

#define DLIST_BACKWARD(list,curr)\
for((curr)=(list)->last; (curr)!=NULL; (curr) = (curr)->prev)


#endif

