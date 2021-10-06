#ifndef RED_BLACK_H
#define RED_BLACK_H

#include "hval.h"

struct rb_stc
{
	unsigned char color;
	struct rb_stc *left;
	struct rb_stc *right;
	struct rb_stc *parent;
	struct rb_stc *prev;
	struct rb_stc *next;
	double key;
	Hval value;
};

typedef struct rb_stc RB;

#define RB_GREEN (0)
#define RB_RED (1)
#define RB_BLACK (2)

RB *RBTreeInit();
void RBDeleteTree(RB *tree);
void RBDelete(RB *tree, RB *node);
RB *RBFind(RB *tree, double key);
void RBInsert(RB *tree, double key, Hval value);

#define RB_FIRST(tree) ((tree)->prev)
#define RB_LAST(tree) ((tree)->next)

#define RB_FORWARD(tree,curr)\
  for((curr)=RB_FIRST(tree); (curr) != NULL; (curr) = (curr)->next)

#define RB_BACKWARD(tree,curr)\
  for((curr)=RB_LAST(tree); (curr) != NULL; (curr) = (curr)->prev)


#endif

