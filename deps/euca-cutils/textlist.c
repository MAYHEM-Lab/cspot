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

#include "hval.h"
#include "redblack.h"
#include "dlist.h"
#include "textlist.h"

TXL *InitTXL()
{
	TXL *tl;

	tl = (TXL *)malloc(sizeof(TXL));
	if(tl == NULL) {
		return(NULL);
	}
	memset(tl,0,sizeof(TXL));

	tl->list = DlistInit();
	if(tl->list == NULL) {
		free(tl);
		return(NULL);
	}

	tl->index = RBInitS();
	if(tl->index == NULL) {
		DlistRemove(tl->list);
		free(tl);
		return(NULL);
	}

	return(tl);
}

void DestroyTXL(TXL *tl)
{
	DlistNode *dl;
	char *s;

	if(tl->list != NULL) {
		DLIST_FORWARD(tl->list,dl)
		{
			s = dl->value.s;
			if(s != NULL) {
				free(s);
			}
		}
		DlistRemove(tl->list);
	}
	if(tl->index != NULL) {
		RBDestroyS(tl->index);
	}
	free(tl);
	return;
}

int AppendWordTXL(TXL *tl, char *word)
{
	char *local_word;
	DlistNode *dn;
	RB *rb;

	if(word == NULL) {
		return(1);
	}

	local_word = (char *)malloc(strlen(word) + 1);
	if(local_word == NULL) {
		return(0);
	}
	memset(local_word,0,strlen(word)+1);
	strcpy(local_word,word);

	dn = DlistAppend(tl->list,(Hval)local_word);
	if(dn == NULL) {
		free(local_word);
		return(0);
	}

	/*
	 * index the first occurance
	 */
	rb = RBFindS(tl->index,word);
	if(rb == NULL) {
		RBInsertS(tl->index,local_word,(Hval)(void *)dn);
	}

	tl->count++;

	return(1);
}

DlistNode *FindWord(TXL *tl, char *word)
{
	RB *rb;

	rb = RBFindS(tl->index,word);
	if(rb != NULL) {
		return((DlistNode *)(rb->value.v));
	} else {
		return(NULL);
	}
}

/*
 * return a dlist of pointers to words in the TXL that contain the
 * substring
 */
Dlist *FindSubStr(TXL *tl, char *str)
{
	Dlist *sslist;
	DlistNode *curr;
	char *found;

	sslist = DlistInit();
	if(sslist == NULL) {
		return(NULL);
	}

	DLIST_FORWARD(tl->list,curr)
	{
		/*
		 * note that search is case sensitive
		 */
		found = strstr(curr->value.s,str);
		if(found != NULL) {
			DlistAppend(sslist,(Hval)(void *)curr);
		}
		
	}

	return(sslist);
}

int IsInStr(char *str, char c)
{
	int i;
	for(i=0; i < strlen(str); i++)
	{
		if(str[i] == c) {
			return(1);
		}
	}

	return(0);
}

char *GetNextWord(char *line, char *separators, char **out_word)
{
	char *word;
	int len;
	int i;
	int j;
	int k;
	int l;
	int size;
	char *curr;
	char *where;

	*out_word = NULL;
	if(line == NULL) {
		return(NULL);
	}

	len = strlen(line);
	if(len == 0) {
		return(NULL);
	}
	size = 0;
	i = 0;
	while(IsInStr(separators,line[i]))
	{
		i++;
		/*
		 * return NULL if we see nothing but separator
		 * chars
		 */
		if(line[i] == 0) {
			return(NULL);
		}
	}

	/*
	 * i now the first non separator
	 */
	j = i;
	while(!IsInStr(separators,line[j]))
	{
		j++;
		if(line[j] == 0) {
			break;
		}
	}

	/*
	 * j now next non separator or \0 at end of line
	 */
	word = (char *)malloc((j - i) + 1);
	if(word == NULL) {
		return(NULL);
	}
	memset(word,0,(j-i)+1);
	l = 0;
	for(k=i,l=0; k < j; k++,l++)
	{
		word[l] = line[k];
	}
	*out_word = word;

	/*
	 * if we are at the end of the string, return NULL
	 */
	if(line[j] != 0) {
		where = &(line[j]);
	} else {
		where = NULL;
	}

	return(where);
}


TXL *ParseLine(char *line, char *separators)
{
	char *curr;
	char *next;
	char *word;
	TXL *tl;
	int err;

	tl = InitTXL();
	if(tl == NULL) {
		return(NULL);
	}

	curr = line;
	next = GetNextWord(curr,separators,&word);
	while(word != NULL)
	{
		if(word != NULL) {
			err = AppendWordTXL(tl,word);
		}
		if(err <= 0) {
			fprintf(stderr,
		"failed to append %s to TXL\n",word);
			fflush(stderr);
			DestroyTXL(tl);
			return(NULL);
		}
		if(word != NULL) {
			free(word);
			word = NULL;
		}
		curr = next;
		next = GetNextWord(curr,separators,&word);
	}
	return(tl);
}

void PrintTXL(TXL *tl)
{
	DlistNode *dn;
	int count;

	count = 0;
	DLIST_FORWARD(tl->list,dn)
	{
		fprintf(stdout,"%s",dn->value.s);
		count++;
		if(count != tl->count) {
			fprintf(stdout," ");
		} else {
			fprintf(stdout,"\n");
		}
	}

	return;
}

#ifdef TEST
int main(int argc, char *argv[])
{
	Dlist *dl;
	DlistNode *dln;
	DlistNode *dln2;

	char *line = "The quick brown fox jumped over the lazy dog's back.";
	char *separators = " ";
	TXL *tl;

	tl = ParseLine(line,separators);
	if(tl == NULL) {
		fprintf(stderr,"couldn't parse line\n");
		exit(1);
	}

	PrintTXL(tl);

	/*
	 * test the find sub string function
	 */
	printf("looking for words with the substring 'he'\n");
	dl = FindSubStr(tl,"he");
	if(dl != NULL) {
		DLIST_FORWARD(dl,dln)
		{
			dln2 = (DlistNode *)dln->value.v;
			printf("%s ",dln2->value.s);
		}
		printf("\n");
	} else {
		printf("ERROR: no substring found\n");
	}
		

	DestroyTXL(tl);

	return(0);

}

#endif
