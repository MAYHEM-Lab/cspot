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
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "dlist.h" /* for printing */

#include "redblackdebug.h"

int rbyan()
{
	return(0);
}

#define MAX_DEL_COUNT (40)
#define BIG_COUNT (10000)

int main(int argc, char *argv[])
{

	RB *tree;
	int i;
	double r;
	int status;
	RB *curr;
	RB *next;
	RB *node;
	char **del_key;
	char *s;
	int del_count;
	int count;

	tree = RBInitS();
	if(tree == NULL)
	{
		fprintf(stderr,"failed to make tree\n");
		exit(1);
	}

	del_count = 0;
	del_key = (char **)malloc(MAX_DEL_COUNT*sizeof(char *));

	for(i=0; i < 10; i++)
	{
		s = (char *)malloc(20);
		if(s == NULL)
		{
			exit(1);
		}
		memset(s,0,20);
		r = drand48();
		sprintf(s,"%f",r);
		printf("inserting: %s\n",s);
		RBInsertS(tree,s,(Hval)0);
		RBPrintTree(tree);
		status = RBTestGPs(tree);
		if(status == 0)
		{
			printf("RB tree fails GP test\n");
			break;
		}
		status = RBTestInvariants(tree);
		if(status == 0)
		{
			printf("RB tree fails invarant test\n");
			break;
		}

		if((drand48() < 0.1) && (del_count < MAX_DEL_COUNT))
		{
			del_key[del_count] = s;
			del_count++;
		}
	}

	curr = tree->prev;
	while(curr != NULL)
	{
		printf("sorted: %s\n",K_S(curr->key));
		curr = curr->next;
	}

	for(i=0; i < del_count; i++)
	{
		printf("searching for %s\n",del_key[i]);
		node = RBFindS(tree,del_key[i]);
		if(node != NULL)
		{
			printf("\tfound\n");
		}
		else
		{
			printf("\tnot found\n");
			exit(1);
		}

		printf("deleteing %s\n",del_key[i]);
		free(del_key[i]);
		RBDeleteS(tree,node);
//		RBPrintTree(tree);
		status = RBTestInvariants(tree);
		if(status == 0)
		{
			printf("RB tree fails invarant test\n");
			exit(1);
		}
	}

	/*
	 * let's try deleting the root
	 */
	printf("deleting root: %s\n",K_S(tree->left->key));
	free(K_S(tree->left->key));
	RBDeleteS(tree,tree->left);
	status = RBTestInvariants(tree);
	if(status == 0)
	{
		printf("RB tree fails invarant test after root\n");
		exit(1);
	}
	printf("new root: %s\n",K_S(tree->left->key));
	curr = tree->prev;
	while(curr != NULL)
	{
		printf("sorted: %s\n",K_S(curr->key));
		curr = curr->next;
	}

	printf("freeing strings\n");
	curr = tree->prev;
        while(curr != NULL)
        {
		free(K_S(curr->key));
		curr = curr->next;
        }
	printf("deleteing tree\n");
	RBDestroyS(tree);

	tree = RBInitS();
	if(tree == NULL)
	{
		fprintf(stderr,"couldn't get new tree\n");
		exit(1);
	}

	printf("doing big insert\n");
	for(i=0; i < BIG_COUNT; i++)
	{
		s = (char *)malloc(20);
		if(s == NULL)
		{
			exit(1);
		}
		memset(s,0,20);
		r = drand48();
		sprintf(s,"%f",r);
		RBInsertS(tree,s,(Hval)0);
		status = RBTestInvariants(tree);
		if(status == 0)
		{
			printf("RB tree fails big invarant test\n");
			break;
		}
	}


	count = BIG_COUNT;
	printf("doing big delete test\n");

	while(count > 10)
	{
		r = drand48();
		curr = tree->prev;
		for(i=0; i < (int)(r*count); i++)
		{
			if(CompareKeyType(tree->prev->key,tree->prev->next->key) > 0)
			{
				printf("bad sequence\n");
				exit(1);
			}
			curr = curr->next;
		}
		printf("deleting: %s\n",K_S(curr->key));
		RBDeleteS(tree,curr);
		status = RBTestInvariants(tree);
		if(status == 0)
		{
			printf("RB tree fails big delete %d\n",count);
			exit(1);
		}
		count--;
	}
		

	curr = tree->prev;
	while(curr != NULL)
	{
		printf("sorted: %s\n",K_S(curr->key));
		curr = curr->next;
	}

	printf("deleteing first\n");
	RBDeleteS(tree,RB_FIRST(tree));
	status = RBTestInvariants(tree);
	if(status == 0)
	{
		fprintf(stderr,"first delete fails\n");
		exit(1);
	}
	curr = RB_FIRST(tree);
	while(curr != NULL)
	{
		printf("sorted: %s\n",K_S(curr->key));
		curr = curr->next;
	}

	printf("deleteing last\n");
	RBDeleteS(tree,RB_LAST(tree));
	status = RBTestInvariants(tree);
	if(status == 0)
	{
		fprintf(stderr,"last delete fails\n");
		exit(1);
	}
	curr = RB_LAST(tree);
	while(curr != NULL)
	{
		printf("sorted: %s\n",K_S(curr->key));
		curr = curr->prev;
	}

	curr = RB_FIRST(tree);
	while(curr != NULL)
	{
		next = curr->next;
		printf("deleteing %s\n",K_S(curr->key));
		RBDeleteS(tree,curr);
		status = RBTestInvariants(tree);
		if(status == 0)
		{
			fprintf(stderr,"intermediate delete failed\n");
			exit(1);
		}
		curr = next;
	}
	
        return(0);
}


