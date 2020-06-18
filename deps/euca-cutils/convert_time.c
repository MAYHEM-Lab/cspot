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
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

char Line_buff[512];
char Fname[255];
int DST;

/*
 * simple program for generating UTC from text time stamps
 */


int ConvertDay(char *ds)
{
	char day_buff[15];
	char *curr;
	int i;

	memset(day_buff,0,sizeof(day_buff));

	curr = ds;

	i = 0;
	while(isalpha(*curr))
	{
		day_buff[i] = *curr;
		curr++;
		i++;
	}

	if(strncmp(day_buff,"Sun",sizeof(day_buff)) == 0)
	{
		return(0);
	}
	else if(strncmp(day_buff,"Mon",sizeof(day_buff)) == 0)
	{
		return(1);
	}
	else if (strncmp(day_buff,"Tue",sizeof(day_buff)) == 0)
	{
		return(2);
	}
	else if (strncmp(day_buff,"Wed",sizeof(day_buff)) == 0)
	{
		return(3);
	}
	else if (strncmp(day_buff,"Thu",sizeof(day_buff)) == 0)
	{
		return(4);
	} 
	else if (strncmp(day_buff,"Fri",sizeof(day_buff)) == 0)
	{
		return(5);
	}
	else if (strncmp(day_buff,"Sat",sizeof(day_buff)) == 0)
	{
		return(6);
	}
	else
	{
		return(-1);
	}
}

int ConvertMonth(char *ms)
{
	char mon_buff[15];
	char *curr;
	int i;

	memset(mon_buff,0,sizeof(mon_buff));

	curr = ms;

	i = 0;
	while(isalpha(*curr))
	{
		mon_buff[i] = *curr;
		curr++;
		i++;
	}

	if(strncmp(mon_buff,"Jan",sizeof(mon_buff)) == 0)
	{
		return(0);
	}
	else if(strncmp(mon_buff,"Feb",sizeof(mon_buff)) == 0)
	{
		return(1);
	}
	else if (strncmp(mon_buff,"Mar",sizeof(mon_buff)) == 0)
	{
		return(2);
	}
	else if (strncmp(mon_buff,"Apr",sizeof(mon_buff)) == 0)
	{
		return(3);
	}
	else if (strncmp(mon_buff,"May",sizeof(mon_buff)) == 0)
	{
		return(4);
	} 
	else if (strncmp(mon_buff,"Jun",sizeof(mon_buff)) == 0)
	{
		return(5);
	}
	else if (strncmp(mon_buff,"Jul",sizeof(mon_buff)) == 0)
	{
		return(6);
	}
	else if (strncmp(mon_buff,"Aug",sizeof(mon_buff)) == 0)
	{
		return(7);
	}
	else if (strncmp(mon_buff,"Sep",sizeof(mon_buff)) == 0)
	{
		return(8);
	}
	else if (strncmp(mon_buff,"Oct",sizeof(mon_buff)) == 0)
	{
		return(9);
	}
	else if (strncmp(mon_buff,"Nov",sizeof(mon_buff)) == 0)
	{
		return(10);
	}
	else if (strncmp(mon_buff,"Dec",sizeof(mon_buff)) == 0)
	{
		return(11);
	}
	else
	{
		return(-1);
	}
}

int ConvertTime(char *ts, int *hour, int *min, int *sec)
{
	char t_buff[15];
	char *curr;
	int i;
	int j;

	memset(t_buff,0,sizeof(t_buff));

	curr = ts;
	i = 0;
	while(!isspace(*curr) && (i < sizeof(t_buff)))
	{
		t_buff[i] = *curr;
		i++;
		curr++;
		if(*curr == 0)
		{
			return(-1);
		}
		if((*curr == ':') && (i < sizeof(t_buff)))
		{
			t_buff[i] = ' ';
			i++;
			curr++;
			if(*curr == 0)
			{
				return(-1);
			}
		}
	}

	if(i == sizeof(t_buff))
	{
		return(-1);
	}

	curr = t_buff;

	*hour = atoi(curr);
	while(!isspace(*curr))
	{
		curr++;
	}
	curr++;

	*min = atoi(curr);
	while(!isspace(*curr))
	{
		curr++;
	}
	curr++;

	*sec = atoi(curr);

	return(1);
}

double ConvertTimeStringOld(char *ts)
{
	int day;
	int mon;
	int date;
	int hour;
	int min;
	int sec;
	int year;
	int err;
	struct tm t;
	time_t utc;
	double ret;

	char *curr;

	curr = ts;

	/*
	 * [Thu Dec 25 21:06:11 2008]
	 * 2013-02-06 21:04:15
	 */

	while(!isalpha(*curr))
	{
		curr++;
		if(*curr == 0)
		{
			break;
		}
	}

	if(*curr == 0)
	{
		return(-1.0);
	}

	day = ConvertDay(curr);
	if(day < 0)
	{
		day = 0;
		/* return(-1.0); */
	}

	while(isalpha(*curr))
	{
		curr++;
		if(*curr == 0)
		{
			return(-1.0);
		}
	}
	while(*curr == ' ')
	{
		curr++;
		if(*curr == 0)
		{
			return(-1.0);
		}
	}

	mon = ConvertMonth(curr);
	if(mon < 0)
	{
		return(-1.0);
	}

	while(*curr != ' ')
	{
		curr++;
		if(*curr == 0)
		{
			return(-1.0);
		}
	}

	while(*curr == ' ')
	{
		curr++;
		if(*curr == 0)
		{
			return(-1.0);
		}
	}

	date = atoi(curr);

	while(*curr != ' ')
	{
		curr++;
		if(*curr == 0)
		{
			return(-1.0);
		}
	}
	while(*curr == ' ')
	{
		curr++;
		if(*curr == 0)
		{
			return(-1.0);
		}
	}

	err = ConvertTime(curr,&hour,&min,&sec);
	if(err < 0)
	{
		return(-1.0);
	}

	while(*curr == ' ')
	{
		curr++;
		if(*curr == 0)
		{
			return(-1.0);
		}
	}

	while(*curr != ' ')
	{
		curr++;
		if(*curr == 0)
		{
			return(-1.0);
		}
	}
	while(*curr == ' ')
	{
		curr++;
		if(*curr == 0)
		{
			return(-1.0);
		}
	}

	year = atoi(curr);

	memset(&t,0,sizeof(t));

	t.tm_sec = sec;
	t.tm_min = min;
	t.tm_hour = hour;
	t.tm_mday = date;
	t.tm_mon = mon;
	t.tm_year = year - 1900;
	t.tm_wday = day;

	utc = mktime(&t);
	ret = (double)utc;

	return(ret);

}

double ConvertTimeStringNew(char *ts)
{
	int day;
	int mon;
	int date;
	int hour;
	int min;
	int sec;
	int year;
	int err;
	struct tm t;
	struct tm *now;
	time_t utc;
	double ret;
	char *curr;
	time_t tval;	/* for daylight savings time */
	int dst;

	if(DST == 1) {
		tval = time(NULL);
		now = localtime(&tval);
		if(now != NULL) {
			dst = now->tm_isdst;
		} else {
			dst = 0;
		}
	} else {
		dst = 0;
	}
	

	curr = ts;

	/*
	 * 2013-02-06 21:04:15
	 */

	while(iswspace(*curr))
	{
		curr++;
		if(*curr == 0)
		{
			break;
		}
	}

	year = atoi(curr);

	while(isdigit(*curr))
	{
		curr++;
		if(*curr == 0)
		{
			return(-1.0);
		}
	}
	if(*curr != '-') {
		return(-1.0);
	}
	while(*curr == '-')
	{
		curr++;
		if(*curr == 0)
		{
			return(-1.0);
		}
	}

	mon = atoi(curr) - 1;
	while(isdigit(*curr))
	{
		curr++;
		if(*curr == 0)
		{
			return(-1.0);
		}
	}
	if(*curr != '-') {
		return(-1.0);
	}
	while(*curr == '-')
	{
		curr++;
		if(*curr == 0)
		{
			return(-1.0);
		}
	}

	date = atoi(curr);
	while(isdigit(*curr))
	{
		curr++;
		if(*curr == 0)
		{
			return(-1.0);
		}
	}

	if(*curr != ' ') {
		return(-1.0);
	}
	while(iswspace(*curr))
	{
		curr++;
		if(*curr == 0)
		{
			return(-1.0);
		}
	}


	err = ConvertTime(curr,&hour,&min,&sec);
	if(err < 0)
	{
		return(-1.0);
	}

	memset(&t,0,sizeof(t));

	t.tm_sec = sec;
	t.tm_min = min;
	t.tm_hour = hour;
	t.tm_mday = date;
	t.tm_mon = mon;
	t.tm_year = year - 1900;
	t.tm_wday = day;
	t.tm_isdst = dst;

	utc = mktime(&t);
	ret = (double)utc;

	return(ret);

}

#ifdef STANDALONE
#define ARGS "f:Dd"

char *Usage = "convert_time -f filename\n\
\t-d <disable daylight savings correction\n\
\t-D\n";

int Diff;
	
int main(int argc, char *argv[])
{
	int c;
	FILE *fd;
	char *curr;
	char *t;
	int word;
	double dtime;
	double last_time;

	DST=1;

	while((c = getopt(argc,argv,ARGS)) != EOF)
	{
		Diff = 0;
		switch(c)
		{
			case 'f':
				strncpy(Fname,optarg,sizeof(Fname));
				break;
			case 'D':
				Diff = 1;
				break;
			case 'd':
				DST = 0;
				break;
			default:
				fprintf(stderr,"unrecognized command %c\n",
						(char)c);
				fprintf(stderr,"%s",Usage);
				exit(1);
		}
	}

	if(Fname[0] == 0) {
		fd = stdin;
	} else {
		fd = fopen(Fname,"r");
		if(fd == NULL) {
			fprintf(stderr,"couldn't open %s\n",Fname);
			exit(1);
		}
	}

	last_time = -1;
	while(fgets(Line_buff,sizeof(Line_buff),fd))
	{
		/*
		 * assume time is in first column
		 */
		t = Line_buff;

		if((*t == '\n') || (*t == '#'))
		{
			continue;
		}
		/*
		 * skip 5 words
		 */
		curr = Line_buff;
		word = 0;
		while(*curr == ' ')
		{
			curr++;
		}
		if(*curr == '[') { /* old style */
			while(word < 5)
			{
				while(*curr != ' ')
				{
					if(*curr == 0)
					{
						break;
					}
					curr++;
				}
				if(*curr == 0)
				{
					break;
				}
				word++;
				while(*curr == ' ')
				{
					if(*curr == 0)
					{
						break;
					}
					curr++;
				}
			}
		dtime = ConvertTimeStringOld(t);
	} else { /* new style */
		while(word < 2)
		{
			while(*curr != ' ')
			{
				if(*curr == 0)
				{
					break;
				}
				curr++;
			}
			if(*curr == 0)
			{
				break;
			}
			word++;
			while(*curr == ' ')
			{
				if(*curr == 0)
				{
					break;
				}
				curr++;
			}
		}	
		dtime = ConvertTimeStringNew(t);
	}
		if(Diff == 0) {
			printf("%10.0f %s",dtime,curr);
		} else {
			if(last_time != -1) {
				printf("%10.0f %s",dtime - last_time,curr);
			}
			last_time = dtime;
		}
		memset(Line_buff,0,sizeof(Line_buff));
	}



	return(0);
}

#endif
