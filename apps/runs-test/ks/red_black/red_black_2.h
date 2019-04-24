#ifndef RED_BLACK_2_H
#define RED_BLACK_2_H

#include "hval_2.h"

struct key_type_stc
{
        unsigned char type;
        Hval key;
};

typedef struct key_type_stc KEY_t;

#define K_INT (1)
#define K_DOUBLE (2)
#define K_STRING (3)
#define K_INT64 (4)

struct rb_stc
{
	unsigned char color;
	struct rb_stc *left;
	struct rb_stc *right;
	struct rb_stc *parent;
	struct rb_stc *prev;
	struct rb_stc *next;
	KEY_t key;
	Hval value;
};

typedef struct rb_stc RB;

#define RB_GREEN (0)
#define RB_RED (1)
#define RB_BLACK (2)

#if 0
RB *RBTreeInit(unsigned char type);
void RBDeleteTree(RB *tree);
void RBDelete(RB *tree, RB *node);
RB *RBFind(RB *tree, KEY_t key);
void RBInsert(RB *tree, KEY_t key, Hval value);
#endif

RB *RBInitD();
void RBDestroyD(RB *rb);
void RBDeleteD(RB *rb, RB *node);
RB *RBFindD(RB *rb, double dkey);
void RBInsertD(RB *rb, double dkey, Hval value);
#define K_D(k) ((k).key.d)

RB *RBInitI();
void RBDestroyI(RB *rb);
void RBDeleteI(RB *rb, RB *node);
RB *RBFindI(RB *rb, int ikey);
void RBInsertI(RB *rb, int ikey, Hval value);
#define K_I(k) ((k).key.i)

RB *RBInitS();
void RBDestroyS(RB *rb);
void RBDeleteS(RB *rb, RB *node);
RB *RBFindS(RB *rb, char *skey);
void RBInsertS(RB *rb, char *skey, Hval value);
#define K_S(k) ((k).key.s)

#define RB_FIRST(tree) ((tree)->prev)
#define RB_LAST(tree) ((tree)->next)

#define RB_FORWARD(tree,curr)\
  for((curr)=RB_FIRST(tree); (curr) != NULL; (curr) = (curr)->next)

#define RB_BACKWARD(tree,curr)\
  for((curr)=RB_LAST(tree); (curr) != NULL; (curr) = (curr)->prev)


#endif

