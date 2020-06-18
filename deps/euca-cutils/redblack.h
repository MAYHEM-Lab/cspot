/*
# Software License Agreement (BSD License)
#
# Copyright (c) 2009-2012, Eucalyptus Systems, Inc.
# All rights reserved.
#
# Redistribution and use of this software in source and binary forms, with or
# without modification, are permitted provided that the following conditions
# are met:
#
#   Redistributions of source code must retain the above
#   copyright notice, this list of conditions and the
#   following disclaimer.
#
#   Redistributions in binary form must reproduce the above
#   copyright notice, this list of conditions and the
#   following disclaimer in the documentation and/or other
#   materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef REDBLACK_H
#define REDBLACK_H

#include "hval.h"

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

RB *RBInitI64();
void RBDestroyI64(RB *rb);
void RBDeleteI64(RB *rb, RB *node);
RB *RBFindI64(RB *rb, int64_t ikey);
void RBInsertI64(RB *rb, int64_t ikey, Hval value);
#define K_I64(k) ((k).key.i64)

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

