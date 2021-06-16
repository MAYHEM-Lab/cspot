/*
# Copyright 2009-2012 Eucalyptus Systems, Inc.
#
# Redistribution and use of this software in source and binary forms,
# with or without modification, are permitted provided that the following
# conditions are met:
#
#   Redistributions of source code must retain the above copyright notice,
#   this list of conditions and the following disclaimer.
#
#   Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef SIMPLE_INPUT_H
#define SIMPLE_INPUT_H

#include <stdio.h>

#define DEFAULT_BLOCKSIZE (50000)
int InitDataSet(void **cookie, int fields);
void FreeDataSet(void *cookie);
void SetBlockSize(void *cookie, int size);
int LoadDataSet(char *fname, void *cookie);
void Rewind(void *cookie);
int SizeOf(void *cookie);
int WriteData(void *cookie, int fields, double *values);
/*
 * legacy
 */
int WriteEntry(void *cookie, double ts, double value);
int WriteEntry4(void *cookie, double a, double b, double c, double d);
int WriteEntry6(void *cookie, double a, double b, double c, 
			      double d, double e, double f);
int WriteEntry7(void *cookie, double a, double b, double c, 
			      double d, double e, double f, double g);
int WriteEntry10(void *cookie, double a, double b, double c, 
			      double d, double e, double f, double g,
			      double h, double i, double j);
int ReadData(void *cookie, int fields, double *values);
int ReadEntry(void *cookie, double *ts, double *value);
/*
 * legacy
 */
int ReadEntry4(void *cookie, double *a, double *b, double *c, double *d);
int ReadEntry5(void *cookie, double *a, double *b, double *c, double *d,
		double *e);
int ReadEntry7(void *cookie, double *a, double *b, double *c, double *d,
		double *e, double *f, double *g);
int ReadEntry10(void *cookie, double *a, double *b, double *c, 
			     double *d, double *e, double *f,
			     double *g, double *h, double *i, double *j);
int CopyDataSet(void *s, void *d, int count);

int GetFieldCount(char *fname);
double *GetFieldValues(void *cookie, int field);
int Current(void *cookie);
void PrintDataSetStats(FILE *fd, void *cookie);

#endif
