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
#ifndef DLIST_2_H
#define DLIST_2_H

#include "hval.h"

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

