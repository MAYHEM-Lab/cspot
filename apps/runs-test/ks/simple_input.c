#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "simple_input.h"

static double Seq_no = 0;

struct data_set_stc
{
	double **data;
	int current;
	int data_c;
	int space_size;
	int block_size;
	int fields;
};

typedef struct data_set_stc DataSet;

int
InitDataSet(void **cookie, int fields)
{
	DataSet *ds;
	int i;
	
	ds = (DataSet *)malloc(sizeof(DataSet));
	if(ds == NULL)
	{
		fprintf(stderr,
		"InitDataSet: couldn't malloc space for data_set on open\n");
		fflush(stderr);
		exit(1);
	}

	ds->block_size = DEFAULT_BLOCKSIZE;
	
	
	ds->fields = fields;
	ds->data =(double **)malloc(fields*sizeof(double *));

	if(ds->data == NULL)
	{
		fprintf(stderr,
	"InitDataSet: can't malloc %d elements on init\n",DEFAULT_BLOCKSIZE);
		fflush(stderr);
		exit(1);
	}

	for(i=0; i < fields; i++)
	{
		ds->data[i] = (double *)malloc(ds->block_size*sizeof(double));
		if(ds->data[i] == NULL)
		{
			exit(1);
		}
	}

	ds->data_c = 0;
	ds->current = -1;
	ds->space_size = ds->block_size;

	*cookie = (void *)ds;
	
	return(1);
}

void SetBlockSize(void *cookie, int size)
{
	DataSet *ds = (DataSet *)cookie;

	ds->block_size = size;
	return;
}

void ExpandData(char *icookie)
{
	DataSet *ds = (DataSet *)icookie;
	int fields = ds->fields;
	int i;
	int j;
	double *new_data;
	int new_size;

	new_size = ds->space_size + ds->block_size;

	for(i=0; i < fields; i++)
	{
		new_data = (double *)malloc(new_size*sizeof(double));
		if(new_data == NULL)
		{
			exit(1);
		}
		for(j=0; j < ds->data_c; j++)
		{
			new_data[j] = ds->data[i][j];
		}
		free(ds->data[i]);
		ds->data[i] = new_data;
	}

	ds->space_size = new_size;
	
	return;
}

void FreeDataSet(void *cookie)
{
	DataSet *ds = (DataSet *)cookie;
	int i;

	for(i=0; i < ds->fields; i++)
	{
		free(ds->data[i]);
	}
	free(ds->data);
	free(ds);
}

double *GetFieldValues(void *cookie, int field)
{
	DataSet *ds = (DataSet *)cookie;

	if(field >= ds->fields)
	{
		return(NULL);
	}

	return(ds->data[field]);

}

int SizeOf(void *cookie)
{
	DataSet *ds = (DataSet *)cookie;

	return(ds->data_c);
}

void Rewind(void *cookie)
{
	DataSet *ds = (DataSet *)cookie;

	ds->current = 0;

	return;
}

void Seek(void *cookie, int position)
{
	DataSet *ds = (DataSet *)cookie;

	ds->current = position;

	return;
}

char *GetNextStr(char *s, char **next_s)
{
	int i;
	char *ret_s;
	char *ls;
	int size;

	/*
	 * CAUTION -- this routine returns a string that it mallocs
	 * expecting caller to call free()
	 */

	i=0;

	while(isspace(s[i]) && (s[i] != 0))
	{
		i++;
	}
	if(s[i] == 0)
	{
		return(NULL);
	}

	size = 0;
	ls = &(s[i]);
	while(!isspace(ls[size]) && (ls[size] != 0))
	{
		size++;
	}
	if(ls[size] == 0)
	{
		return(NULL);
	}

	size++;
	ret_s = (char *)malloc(size*sizeof(char));
	if(ret_s == NULL)
	{
		exit(1);
	}
	memset(ret_s,0,size);
	strncpy(ret_s,ls,size);

	/*
	 * skip over the non spaces for next
	 */
	while(!isspace(s[i]))
	{
		i++;
	}

	*next_s = &(s[i]);

	return(ret_s);
}
	
int GetFieldCount(char *fname)
{
	int i;
	FILE *fd;
	char *s;
	char *str;
	char *next_str;
	char line_buff[255];
	int err;
	char *ferr;
	int fcount;

	fd = fopen(fname,"r");
	if(fd == NULL)
	{
		fprintf(stderr,"GetFieldCount could open %s for reading\n",
		                fname);
		fflush(stderr);
		return(0);
	}

	while(!feof(fd))
	{
		ferr = fgets(line_buff,sizeof(line_buff),fd);
		if(ferr == NULL)
		{
			break;
		}
		if(line_buff[0] == '#')
		{
			continue;
		}
		if(line_buff[0] == '\n')
		{
			continue;
		}
		fcount = 0;
		str = line_buff;
		while(GetNextStr(str,&next_str))
		{
			fcount++;
			str = next_str;
		}
		break;
		
	}

	fclose(fd);

	return(fcount);
}
int LoadDataSet(char *fname, void *cookie)
{
	DataSet *ds = (DataSet *)cookie;
	int i;
	FILE *fd;
	char *s;
	char *next_s;
	int count;
	char line_buff[255];
	int err;
	char *ferr;

	if(cookie == NULL)
	{
		return(0);
	}
	
	fd = fopen(fname,"r");
	if(fd == NULL)
	{
		fprintf(stderr,"LoadDataSet could open %s for reading\n",
		                fname);
		fflush(stderr);
		return(0);
	}

	count = 0;
	while(!feof(fd))
	{
		ferr = fgets(line_buff,sizeof(line_buff),fd);
		if(ferr == NULL)
		{
			break;
		}
		if(line_buff[0] == '#')
		{
			continue;
		}
		if(line_buff[0] == '\n')
		{
			continue;
		}
		count++;
	}
	rewind(fd);

	while(count > ds->space_size)
	{
		ExpandData((void *)ds);
	}

	ds->current = 0;
	while(!feof(fd))
	{
		memset(line_buff,0,sizeof(line_buff));
		ferr = fgets(line_buff,sizeof(line_buff),fd);
		if(ferr == NULL)
		{
			break;
		}
		if(line_buff[0] == '#')
		{
			continue;
		}
		if(line_buff[0] == '\n')
		{
			continue;
		}
		count++;
		for(i=0; i < ds->fields; i++)
		{
			ds->data[i][ds->current] = 0.0;
		}
		s = line_buff;
		for(i=0; i < ds->fields; i++)
		{
			s = GetNextStr(s,&next_s);
			if(s == NULL)
			{
				break;
			}
			ds->data[i][ds->current] = strtod(s,NULL);
			free(s);
			s = next_s;
		}
		ds->current++;
		ds->data_c++;
	}

	/*
	 * reset the current pointer
	 */
	ds->current = 0;
	fclose(fd);

	return(1);
}

int WriteData(void *cookie, int fields, double *values)
{
	DataSet *ds = (DataSet *)cookie;
	int lfields;
	int i;

	if((ds->current + 1) >= ds->space_size)
	{
		ExpandData((void *)ds);
	}

	/*
	 * EOF is when current == data_c
	 */
	if(ds->current < ds->data_c)
	{
		ds->current++;
	}

	if(fields > ds->fields)
	{
		lfields = ds->fields;
	}
	else
	{
		lfields = fields;
	}

	for(i=0; i < lfields; i++)
	{
		ds->data[i][ds->current] = values[i];
	}

	if(ds->current >= ds->data_c)
	{
		ds->data_c++;
	}
	
	return(1);
}

int WriteEntry(void *cookie, double ts, double value)
{
	DataSet *ds = (DataSet *)cookie;
	int i;

	if((ds->current + 1) >= ds->space_size)
	{
		ExpandData((void *)ds);
	}

	/*
	 * EOF is when current == data_c
	 */
	if(ds->current < ds->data_c)
	{
		ds->current++;
	}

	ds->data[0][ds->current] = ts;
	ds->data[1][ds->current] = value;

	if(ds->current >= ds->data_c)
	{
		ds->data_c++;
	}
	
	return(1);
}

int WriteEntry4(void *cookie, double a, double b, double c, double d)
{
	DataSet *ds = (DataSet *)cookie;
	int i;

	if((ds->current + 1) >= ds->space_size)
	{
		ExpandData((void *)ds);
	}

	if(ds->current < ds->data_c)
	{
		ds->current++;
	}

	ds->data[0][ds->current] = a;
	ds->data[1][ds->current] = b;
	ds->data[2][ds->current] = c;
	ds->data[3][ds->current] = d;


	if(ds->current >= ds->data_c)
	{
		ds->data_c++;
	}
	
	return(1);
}

int WriteEntry6(void *cookie, 
	        double a, 
		double b, 
		double c, 
		double d,
		double e,
		double f)
{
	DataSet *ds = (DataSet *)cookie;
	int i;

	if((ds->current + 1) >= ds->space_size)
	{
		ExpandData((void *)ds);
	}

	if(ds->current < ds->data_c)
	{
		ds->current++;
	}

	ds->data[0][ds->current] = a;
	ds->data[1][ds->current] = b;
	ds->data[2][ds->current] = c;
	ds->data[3][ds->current] = d;
	ds->data[4][ds->current] = e;
	ds->data[5][ds->current] = f;


	if(ds->current >= ds->data_c)
	{
		ds->data_c++;
	}
	
	return(1);
}

int WriteEntry7(void *cookie, 
	        double a, 
		double b, 
		double c, 
		double d,
		double e,
		double f,
		double g)
{
	DataSet *ds = (DataSet *)cookie;
	int i;

	if((ds->current + 1) >= ds->space_size)
	{
		ExpandData((void *)ds);
	}

	if(ds->current < ds->data_c)
	{
		ds->current++;
	}

	ds->data[0][ds->current] = a;
	ds->data[1][ds->current] = b;
	ds->data[2][ds->current] = c;
	ds->data[3][ds->current] = d;
	ds->data[4][ds->current] = e;
	ds->data[5][ds->current] = f;
	ds->data[6][ds->current] = g;


	if(ds->current >= ds->data_c)
	{
		ds->data_c++;
	}
	
	return(1);
}
int WriteEntry10(void *cookie, 
	        double a, 
		double b, 
		double c, 
		double d,
		double e,
		double f,
		double g,
		double h,
		double i,
		double j)
{
	DataSet *ds = (DataSet *)cookie;

	if((ds->current + 1) >= ds->space_size)
	{
		ExpandData((void *)ds);
	}

	if(ds->current < ds->data_c)
	{
		ds->current++;
	}

	ds->data[0][ds->current] = a;
	ds->data[1][ds->current] = b;
	ds->data[2][ds->current] = c;
	ds->data[3][ds->current] = d;
	ds->data[4][ds->current] = e;
	ds->data[5][ds->current] = f;
	ds->data[6][ds->current] = g;
	ds->data[7][ds->current] = h;
	ds->data[8][ds->current] = i;
	ds->data[9][ds->current] = j;


	if(ds->current >= ds->data_c)
	{
		ds->data_c++;
	}
	
	return(1);
}

int ReadData(void *cookie, int fields, double *values)
{
	DataSet *ds = (DataSet *)cookie;
	int lfields;
	int i;

	if(ds->current == -1)
	{
		return(0);
	}

	if(ds->current >= ds->data_c)
	{
		return(0);
	}

	if(fields > ds->fields)
	{
		lfields = ds->fields;
	}
	else
	{
		lfields = fields;
	}

	for(i=0; i < lfields; i++)
	{
		values[i] = ds->data[i][ds->current];
	}

	ds->current++;

	return(1);
}

int ReadEntry(void *cookie, double *ts, double *value)
{
	DataSet *ds = (DataSet *)cookie;

	if(ds->current == -1)
	{
		return(0);
	}

	if(ds->current >= ds->data_c)
	{
		return(0);
	}

	*ts = ds->data[0][ds->current];
	*value = ds->data[1][ds->current];

	ds->current++;

	return(1);
}

int ReadEntry4(void *cookie, double *a, double *b, double *c, double *d)
{
	DataSet *ds = (DataSet *)cookie;

	if(ds->current == -1)
	{
		return(0);
	}

	if(ds->current >= ds->data_c)
	{
		return(0);
	}

	if(ds->fields < 4)
	{
		return(0);
	}

	*a = ds->data[0][ds->current];
	*b = ds->data[1][ds->current];
	*c = ds->data[2][ds->current];
	*d = ds->data[3][ds->current];

	ds->current++;

	return(1);
}

int ReadEntry5(void *cookie, double *a, double *b, double *c, 
		double *d, double *e)
{
	DataSet *ds = (DataSet *)cookie;

	if(ds->current == -1)
	{
		return(0);
	}

	if(ds->current >= ds->data_c)
	{
		return(0);
	}

	if(ds->fields < 4)
	{
		return(0);
	}

	*a = ds->data[0][ds->current];
	*b = ds->data[1][ds->current];
	*c = ds->data[2][ds->current];
	*d = ds->data[3][ds->current];
	*e = ds->data[4][ds->current];

	ds->current++;

	return(1);
}

int ReadEntry6(void *cookie, 
	       double *a, 
	       double *b, 
	       double *c, 
	       double *d,
	       double *e,
	       double *f )
{
	DataSet *ds = (DataSet *)cookie;

	if(ds->current == -1)
	{
		return(0);
	}

	if(ds->current >= ds->data_c)
	{
		return(0);
	}

	if(ds->fields < 4)
	{
		return(0);
	}

	*a = ds->data[0][ds->current];
	*b = ds->data[1][ds->current];
	*c = ds->data[2][ds->current];
	*d = ds->data[3][ds->current];
	*e = ds->data[4][ds->current];
	*f = ds->data[5][ds->current];

	ds->current++;

	return(1);
}

int ReadEntry7(void *cookie, 
	       double *a, 
	       double *b, 
	       double *c, 
	       double *d,
	       double *e,
	       double *f,
	       double *g )
{
	DataSet *ds = (DataSet *)cookie;

	if(ds->current == -1)
	{
		return(0);
	}

	if(ds->current >= ds->data_c)
	{
		return(0);
	}

	if(ds->fields < 4)
	{
		return(0);
	}

	*a = ds->data[0][ds->current];
	*b = ds->data[1][ds->current];
	*c = ds->data[2][ds->current];
	*d = ds->data[3][ds->current];
	*e = ds->data[4][ds->current];
	*f = ds->data[5][ds->current];
	*g = ds->data[6][ds->current];

	ds->current++;

	return(1);
}
int ReadEntry10(void *cookie, 
	       double *a, 
	       double *b, 
	       double *c, 
	       double *d,
	       double *e,
	       double *f,
	       double *g,
	       double *h,
	       double *i,
	       double *j)
{
	DataSet *ds = (DataSet *)cookie;

	if(ds->current == -1)
	{
		return(0);
	}

	if(ds->current >= ds->data_c)
	{
		return(0);
	}

	if(ds->fields < 4)
	{
		return(0);
	}

	*a = ds->data[0][ds->current];
	*b = ds->data[1][ds->current];
	*c = ds->data[2][ds->current];
	*d = ds->data[3][ds->current];
	*e = ds->data[4][ds->current];
	*f = ds->data[5][ds->current];
	*g = ds->data[6][ds->current];
	*h = ds->data[7][ds->current];
	*i = ds->data[8][ds->current];
	*j = ds->data[9][ds->current];

	ds->current++;

	return(1);
}

int Current(void * cookie)
{
	DataSet *ds = (DataSet *)cookie;
	return(ds->current);
}

int CopyDataSet(void *s, void *d, int count)
{
	DataSet *src = (DataSet *)s;
	DataSet *dst = (DataSet *)d;
	int i;
	int j;

	if(count > SizeOf(src))
	{
		return(0);
	}

	if(src->fields != dst->fields)
	{
		return(0);
	}

	while(SizeOf(dst) < count)
	{
		ExpandData((void *)dst);
	}

	for(i=0; i < count; i++)
	{
		for(j=0; j < src->fields; j++)
		{
			dst->data[j][i] = src->data[j][i];
		}
	}
	dst->data_c = count;
	Rewind(dst);

	return(1);
}

void PrintDataSetStats(FILE *fd, void *cookie)
{
	DataSet *ds = (DataSet *)cookie;

	fprintf(stdout,"ds current: %d\n",ds->current);
	fprintf(stdout,"ds data_c: %d\n",ds->data_c);
	fprintf(stdout,"ds space_size: %d\n",ds->space_size);
	fprintf(stdout,"ds block_size: %d\n",ds->block_size);
	fprintf(stdout,"ds fields: %d\n",ds->fields);

	return;
}


#ifdef TEST

extern char *optarg;

char *PRED_ARGS = "f:";


int
main(int argc,char *argv[])
{

	double val;
	double ts;
	void *data_set;
	int size;
	int c;
	int curr;
	int ierr;
	char fname[255];

	if(argc < 2)
	{
		fprintf(stderr,"usage: testinput -f filename\n");
		fflush(stderr);
		exit(1);
	}

	fname[0] = 0;
	memset(fname,0,sizeof(fname));

	while((c = getopt(argc,argv,PRED_ARGS)) != EOF)
	{
		switch(c)
		{
			case 'f':
				strncpy(fname,optarg,sizeof(fname));
				break;
			default:
				fprintf(stderr,"unrecognized argument %c\n",
						c);
				fflush(stderr);
				break;
		}
	}

	if(fname[0] == 0)
	{
		fprintf(stderr,"usage: testinput -f fname\n");
		fflush(stderr);
		exit(1);
	}
	
	ierr = InitDataSet(&data_set,2);
	
	if(ierr == 0)
	{
		fprintf(stderr,"testinput error: InitDataSet failed\n");
		exit(1);
	}

	SetBlockSize(data_set,14);

	ierr = LoadDataSet(fname,data_set);
	if(ierr == 0)
	{
		fprintf(stderr,
			"testinput error: LoadDataSet failed for %s\n",
				fname);
		exit(1);
	}

	while(ReadEntry(data_set,&ts,&val))
	{
		fprintf(stdout,"%d %f\n",(int)ts,val);
		fflush(stdout);
	}

	FreeDataSet(data_set);
	
	exit(0);
}



#endif
