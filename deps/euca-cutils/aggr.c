/*
# Copyright 2003-2018 University of California
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
/*
 * avergaes by count or time stamp
 * 
 * can do either non-overlap or overlap
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include <ctype.h>

#include "simple_input.h"

#define DEFAULT_COUNT (6)

extern char *optarg;
double Time;	/* time stamp range over which to aggregate */
int Tau;	/* number of samples over which to aggregate */
int Overlap;	/* should window slide or exclude? */
int Agg;	/* Aggregate instead of average */



#define PRED_ARGS "Af:c:t:oP:"
char *Usage = "aggr -f filename\n\
\t-A <produce a sum instead of an average>\n\
\t-c count (cannot be used with -t)\n\
\t-t time (cannot be used with -c)\n\
\t-o <make the windows overlap>\n";


int
AvgByTime(double ts,
	  double input_val,
	  void *state_set,
	  double time_delta,
	  double *out_ts,
	  double *out_val)
{
	int ierr;
	double curr_ts;
	double curr_val;
	double count;
	double acc;
	
	Rewind(state_set);
	
	acc = input_val;
	*out_ts = ts;
	count = 1.0;
	while(ReadEntry(state_set,&curr_ts,&curr_val) != 0)
	{
		/*
		 * if we are within a delta of the ts, count it
		 */
		if(curr_ts >= (ts - time_delta))
		{
			acc += curr_val;
			count += 1;
		}
	}
	
	*out_val = acc / count;
	
	Rewind(state_set);
	return(1);
}

int
SumByTime(double ts,
	  double input_val,
	  void *state_set,
	  double time_delta,
	  double *out_ts,
	  double *out_val)
{
	int ierr;
	double curr_ts;
	double curr_val;
	double count;
	double acc;
	
	Rewind(state_set);
	
	acc = input_val;
	*out_ts = ts;
	count = 1.0;
	while(ReadEntry(state_set,&curr_ts,&curr_val) != 0)
	{
		/*
		 * if we are within a delta of the ts, count it
		 */
		if(curr_ts >= (ts - time_delta))
		{
			acc += curr_val;
			count += 1;
		}
	}
	
	*out_val = acc;
	
	Rewind(state_set);
	return(1);
}

int
ElideByTime(double ts,
	  double input_val,
	  void *state_set,
	  double time_delta,
	  double *out_ts,
	  double *out_val)
{
	int ierr;
	double curr_ts;
	double curr_val;
	double count;
	double acc;
	
	Rewind(state_set);
	
	acc = input_val;
	*out_ts = ts;
	count = 1.0;
	while(ReadEntry(state_set,&curr_ts,&curr_val) != 0)
	{
		/*
		 * if we are within a delta of the ts, count it
		 */
		if(curr_ts >= (ts - time_delta))
		{
			*out_val = curr_val;
			*out_ts = curr_ts;
			return(1);
		}
	}
	
	
	Rewind(state_set);
	return(0);
}

int
AvgByCount(double ts,
	  double input_val,
	  void *state_set,
	  int tau,
	  double *out_ts,
	  double *out_val)
{
	int ierr;
	double curr_ts;
	double curr_val;
	double count;
	double acc;
	int size;
	int read;
	
	Rewind(state_set);
	
	size = SizeOf(state_set);
	
	acc = input_val;
	*out_ts = ts;
	count = 1.0;
	read = 0;
	while((ReadEntry(state_set,&curr_ts,&curr_val) != 0) &&
	      (count < tau))
	{
		read += 1;
		
		/*
		 * -1 because input_val counts
		 */
		if((size - read) < (tau-1))
		{
			acc += curr_val;
/*
printf("%f ",curr_val);
*/
			count += 1;
		}
	}
	
/*
printf("%f ",input_val);
printf(" | ");
*/

	*out_val = acc / count;
	
	Rewind(state_set);
	return(1);
}

int
SumByCount(double ts,
	  double input_val,
	  void *state_set,
	  int tau,
	  double *out_ts,
	  double *out_val)
{
	int ierr;
	double curr_ts;
	double curr_val;
	double count;
	double acc;
	int size;
	int read;
	
	Rewind(state_set);
	
	size = SizeOf(state_set);
	
	acc = input_val;
	*out_ts = ts;
	count = 1.0;
	read = 0;
	while((ReadEntry(state_set,&curr_ts,&curr_val) != 0) &&
	      (count < tau))
	{
		read += 1;
		
		/*
		 * -1 because input_val counts
		 */
		if((size - read) < (tau-1))
		{
			acc += curr_val;
			count += 1;
		}
	}
	
	*out_val = acc;
	
	Rewind(state_set);
	return(1);
}

int
main(argc,argv)
int argc;
char *argv[];
{

	char fname[255];
	int c;
	int ierr;
	void *input_data;
	void *agg_data;
	void *agg_data_O;
	void *new_data;
	double out_ts;
	double out_val;
	double value;
	double ts;
	int count_since_last;
	double last_ts;
	int fcount;
	double *values;
	double *Ovalues;
	int total;

	if(argc < 2)
	{
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		exit(1);
	}

	fname[0] = 0;

	Time = -1;
	Tau = DEFAULT_COUNT;
	Overlap = 0;
	Agg = 0;
	while((c = getopt(argc,argv,PRED_ARGS)) != EOF)
	{
		switch(c)
		{
			case 'A':
				Agg = 1;
				break;
			case 'f':
				strncpy(fname,optarg,sizeof(fname));
				break;
			case 'c':
				Tau = atoi(optarg);
				break;
			case 't':
				Time = atof(optarg);
				break;
			case 'o':
				Overlap = 1;
				break;
			case 'P':
				Tau = atoi(optarg);
				break;
			default:
				fprintf(stderr,"unrecognized argument %c\n",
						c);
				fflush(stderr);
				break;
		}
	}
	
	if(Time != -1)
	{
		Tau = -1;
	}

	if(fname[0] == 0)
	{
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		exit(1);
	}
	
	fcount = GetFieldCount(fname);
	if(fcount <= 0) {
		fprintf(stderr,"couldn't get field count for %s\n",
			fname);
		exit(1);
	}

	ierr = InitDataSet(&input_data,fcount);
	if(ierr <= 0)
	{
		fprintf(stderr,"couldn't init data set for %s\n",
			fname);
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		exit(1);
	}

	ierr = LoadDataSet(fname,input_data);
	if(ierr <= 0) {
		fprintf(stderr,"failed to load data from %s\n",
			fname);
		exit(1);
	}

	ierr = InitDataSet(&agg_data,2);
	if(ierr <= 0)
	{
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		FreeDataSet(input_data);
		exit(1);
	}
	ierr = InitDataSet(&agg_data_O,2);
	if(ierr <= 0)
	{
		fprintf(stderr,"%s",Usage);
		fflush(stderr);
		FreeDataSet(input_data);
		exit(1);
	}

	count_since_last = 0;
	last_ts = 0.0;

	values = (double *)malloc(fcount * sizeof(double));
	if(values <= 0) {
		exit(1);
	}

	Ovalues = (double *)malloc(2 * sizeof(double));
	if(Ovalues <= 0) {
		exit(1);
	}
	
	total = 0;
	while(ReadData(input_data,fcount,values) != 0)
	{
		total += 1;
		if(fcount < 2) {
			ts = values[0];
			value = values[0];
		} else {
			ts = values[0];
			value = values[1];
		}
		if(Tau != -1)
		{
			if((Overlap == 1) || (count_since_last >= Tau))
			{
				if(Agg == 0)
				{
					if(Overlap == 0) {
					ierr = AvgByCount(ts,
							  value,
							  agg_data,
							  Tau,
							  &out_ts,
							  &out_val);
					} else {
					ierr = AvgByCount(ts,
							  value,
							  agg_data_O,
							  Tau,
							  &out_ts,
							  &out_val);
					}
				}
				else
				{
				ierr = SumByCount(ts,
						  value,
						  agg_data,
						  Tau,
						  &out_ts,
						  &out_val);
				}

				if(ierr == 0)
				{
					fprintf(stderr,
				"aggr: AvgByCount failed\n");
					fflush(stderr);
					FreeDataSet(input_data);
					FreeDataSet(agg_data);
					exit(1);
				}
				count_since_last = 0;
				if(fcount >= 2) {
				fprintf(stdout,"%10.0f %3.4f\n",
					out_ts,
					out_val);
				} else {
				fprintf(stdout,"%3.4f\n",
					out_val);
				}
				fflush(stdout);
			}
			else
			{
				count_since_last += 1;
			}
		}
		else
		{
			if(fcount < 2) {
				fprintf(stderr,
			"data file %s contains one column\n",
				fname);
				fprintf(stderr,"cannot average by time\n");
				exit(1);
			}
			
			if((Overlap == 1) || (last_ts <= (ts - Time)))
			{
				if(Agg == 0)
				{
				ierr = AvgByTime(ts,
						  value,
						  agg_data,
						  Time,
						  &out_ts,
						  &out_val);
				if(ierr == 0)
				{
					fprintf(stderr,
				"aggr: AvgByTime failed\n");
					fflush(stderr);
					FreeDataSet(input_data);
					FreeDataSet(agg_data);
					exit(1);
				}
				}
				else
				{
				ierr = SumByTime(ts,
						  value,
						  agg_data,
						  Time,
						  &out_ts,
						  &out_val);
				if(ierr == 0)
				{
					fprintf(stderr,
				"aggr: SumByTime failed\n");
					fflush(stderr);
					FreeDataSet(input_data);
					FreeDataSet(agg_data);
					exit(1);
				}
				}
				last_ts = out_ts;

				fprintf(stdout, "%10.0f %3.4f\n",
						out_ts,
						out_val);
				fflush(stdout);
			}
		}	

		if(Overlap == 0) {
			ierr = WriteEntry(agg_data,ts,value);
		} else {
			/*
			 * wait until we see enough
			 */
			if(total <= Tau) {
				while(ReadData(agg_data_O,2,Ovalues));
				Ovalues[0] = ts;
				Ovalues[1] = value;
				ierr = WriteData(agg_data_O,2,Ovalues);
				if(ierr <= 0) {
					exit(1);
				}
				continue;
			}
			ierr = InitDataSet(&new_data,2);
			if(ierr <= 0) {
				exit(1);
			}
			/*
			 * read the first one and throw it away
			 */
			Rewind(agg_data_O);
			ierr = ReadData(agg_data_O,2,Ovalues);
			if(ierr <= 0) {
				if(ierr <= 0) {
					exit(1);
				}
			}
			while(ReadData(agg_data_O,2,Ovalues) != 0) {
				ierr = WriteData(new_data,2,Ovalues);
				if(ierr <= 0) {
					exit(1);
				}
			}
			/*
			 * add the current one to the end
			 */
			Ovalues[0] = ts;
			Ovalues[1] = value;
			ierr = WriteData(new_data,2,Ovalues);
			if(ierr <= 0) {
				exit(1);
			}
			FreeDataSet(agg_data_O);
			agg_data_O = new_data;
		}
		if(ierr == 0)
		{
			fprintf(stderr,
			"aggr: WriteEntry failed for ts %10.0f\n",ts);
			fflush(stderr);
			FreeDataSet(input_data);
			FreeDataSet(agg_data);
			exit(1);
		}

	}
	


	free(values);
	FreeDataSet(input_data);
	FreeDataSet(agg_data);
	FreeDataSet(agg_data_O);
	exit(0);
	
}

