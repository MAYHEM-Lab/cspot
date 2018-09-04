#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <sys/time.h>

#include "redblack.h"
#include "dlist.h"

#include "mioarray.h"
#include "mioregress.h"

#define RAND() (drand48())

#define BAD_VAL (-1000000.0)

double Corr(Array1D *x, Array1D *y);
int JenksSplitEigen(Array1D *ev);

struct object_stc
{
	int id;
	Array1D *series;
	int L;
	int K;
	char *str;
};

typedef struct object_stc Object;

struct bin_stc
{
	Array1D *centroid;
	Dlist *list;
	int count;
};

typedef struct bin_stc Bin;


Array2D *UnitizeArray2D(Array2D *u)
{
	Array2D *on_u;

	on_u = NormalizeColsArray2D(u);
	return(on_u);
}


Array2D *TrajectoryMatrix(Array1D *x, int start, int m, int k)
{
	Array2D *tr;
	int i;
	int j;
	int t_i;
	int t_j;

	tr = MakeArray2D(m,k);
	if(tr == NULL) {
		return(NULL);
	}

	
	i = start; // start at start
	for(t_i=0; t_i < m; t_i++) {
		j = i; // j walks down the row from current i
		for(t_j=0; t_j < k; t_j++) {
			tr->data[(t_i*tr->xdim)+t_j] = x->data[j];
			j++;
		}
		i++; // there are m different starting points, one for each row
	}

	return(tr);
}

int SortEigenVectors(Array1D *ev, Array2D *ea)
{
	RB *map;
	RB *rb;
	int i;
	int j;
	Array2D *sea;
	Array1D *sev;
	int s_j;

	map = RBInitD();
	if(map == NULL) {
		return(-1);
	}

	sea = MakeArray2D(ea->ydim,ea->xdim);
	if(sea == NULL) {
		RBDestroyD(map);
		return(-1);
	}
//	sea = NormalizeColsArray2D(ea);

	sev = MakeArray1D(ev->ydim);
	if(sev == NULL) {
		RBDestroyD(map);
		FreeArray2D(sea);
		return(-1);
	}


	for(i=0; i < ev->ydim; i++) {
		RBInsertD(map,ev->data[i],(Hval)i);
	}

	s_j = 0;
	/*
	 * move the columns and create sorted ev
	 */
	RB_BACKWARD(map,rb) {
		j = rb->value.i;
		for(i=0; i < ea->ydim; i++) {
			sea->data[i * sea->xdim + s_j] =
				ea->data[i * ea->xdim + j];
		}
		sev->data[s_j] = ev->data[j];
		s_j += 1;
	}

	/*
	 * now overwrite ev with sorted data
	 */
	for(j=0; j < ev->ydim; j++) {
		ev->data[j] = sev->data[j];
	}

	/*
	 * and overwrite ea with sorted ea
	 */
	for(i=0; i < ea->ydim; i++) {
		for(j=0; j < ea->xdim; j++) {
			ea->data[i*ea->xdim+j] = 
				sea->data[i*sea->xdim+j];
		}
	}

	RBDestroyD(map);
	FreeArray2D(sea);
	FreeArray1D(sev);

	return(1);
}



Array2D *DiagonalAverage(Array2D *X)
{
	Array2D *Y;
	int i;
	int j;
	int k;
	int N;
	double acc;
	double tot;

	/*
	 * first row and last column
	 */
	N = X->xdim + (X->ydim - 1);
	Y = MakeArray1D(N);
	if(Y == NULL) {
		exit(1);
	}

	for(k = 0; k < N ; k++) {
		acc = 0;
		tot = 0;
		for(i=0; i < X->ydim; i++) {
			for(j=0; j < X->xdim; j++) {
				if((i + j) == k) {
					acc += X->data[i*X->xdim+j];
					tot++;
				}
			}
		}
		Y->data[k] = acc / tot;
	}

	return(Y);
}


Array1D *SSADecomposition(Array1D *series, int L, int start, int end)
{
	Array1D *ev;
	Array2D *ea;
	Array2D *U;
	Array2D *V;
	Array2D *S;
	Array2D *tr;
	Array2D *D;
	Array2D *D2;
	Array2D *Dm2;
	Array2D *X;
	Array2D *X_t;
	Array2D *V_t;
	Array2D *rank_1;
	Array2D *t_u;
	Array2D *t_x;
	Array2D *t_y;
	Array2D *diff;
	Array2D *sum;
	int K;
	int N;
	Array1D *U_col;
	Array1D *V_row;
	int d;
	int i;
	int j;
	int k;
	Array1D **dcomp;
	Array1D *Y;
	int y_i;
	Array2D *I;
	double acc;
	int min_split;


	N = series->ydim;
	K = N - L + 1;


	X = TrajectoryMatrix(series,0,L,K);
	if(X == NULL) {
		fprintf(stderr, 
			"couldn't create trajctory matrix for %d lags\n",
				L);
		exit(1);
	}
	X_t = TransposeArray2D(X);
	if(X_t == NULL) {
		fprintf(stderr,"couldn't transpose array X\n");
		exit(1);
	}

	S = MultiplyArray2D(X_t,X);
	if(S == NULL) {
		fprintf(stderr,
			"couldn't compute S\n");
		exit(1);
	}
	FreeArray2D(X_t);

	ev = EigenValueArray2D(S);
	if(ev == NULL) {
		fprintf(stderr,"couldn't get eigen values\n");
		exit(1);
	}

	ea = EigenVectorArray2D(S);
	if(ea == NULL) {
		fprintf(stderr,"couldn't get eigen vectors\n");
		exit(1);
	}
	FreeArray2D(S);

	V = UnitizeArray2D(ea);
	if(V == NULL) {
		fprintf(stderr,"couldn't unitize eigen vectors\n");
		exit(1);
	}
	FreeArray2D(ea);
	SortEigenVectors(ev,V);

	min_split = JenksSplitEigen(ev);
//printf("min_split: %d\n",min_split);
//fflush(stdout);



	t_u = MultiplyArray2D(X,V);
	if(t_u == NULL) {
		exit(1);
	}

	/*
	 * need number of cols from V_t
	 */
	Dm2 = MakeArray2D(t_u->xdim,V->xdim);
	if(Dm2 == NULL) {
		exit(1);
	}

	D2 = MakeArray2D(t_u->xdim,V->xdim);
	if(D2 == NULL) {
		exit(1);
	}

	D = MakeArray2D(t_u->xdim,V->xdim);
	if(D == NULL) {
		exit(1);
	}

	for(i=0; i < Dm2->ydim; i++) {
		for(j=0; j < Dm2->xdim; j++) {
			Dm2->data[i*D->xdim+j] = 0;
			D2->data[i*D->xdim+j] = 0;
			D->data[i*D->xdim+j] = 0;
		}
	}

	/*
	 * diagonal is 1/sqrt(eigenvalue) for non-zero entries
	 *
	 * if L < K, leave others zero
	 */
	for(i=0; i < L; i++) {
		if(i < Dm2->xdim) {
			Dm2->data[i*Dm2->xdim+i] = 1.0/sqrt(ev->data[i]);
			D2->data[i*D2->xdim+i] = sqrt(ev->data[i]);
			D->data[i*D->xdim+i] = ev->data[i];
		}
	}

	U = MultiplyArray2D(t_u,Dm2);
	if(U == NULL) {
		fprintf(stderr,"can't get U\n");
		exit(1);
	}
	FreeArray2D(t_u);

	V_t = TransposeArray2D(V);
//	V_t = InvertArray2D(V);
	if(V_t == NULL) {
		fprintf(stderr,"couldn't get V_t\n");
		exit(1);
	}
	FreeArray2D(V);
	FreeArray2D(X);


	U_col = MakeArray1D(U->ydim);
	if(U_col == NULL) {
		exit(1);
	}

	V_row = MakeArray2D(1,V_t->xdim);
	if(V_row == NULL) {
		exit(1);
	
	}

#if 0
	sum = MakeArray2D(U_col->ydim,V_row->xdim);
#endif
	sum = MakeArray1D(N);
	if(sum == NULL) {
		exit(1);
	}

	for(i=0; i < sum->ydim; i++) {
		for(j=0; j < sum->xdim; j++) {
			sum->data[i*sum->xdim+j] = 0;
		}
	}

	for(j = 0; j < min_split; j++) {
		for(i=0; i < U_col->ydim; i++) {
			U_col->data[i] = U->data[i*U->xdim+j] * sqrt(ev->data[j]);
		}
		for(i=0; i < V_row->xdim; i++) {
			V_row->data[0*V_row->xdim + i] =
				V_t->data[j*V_t->xdim+i];
		}

		rank_1 = MultiplyArray2D(U_col,V_row);
		if(rank_1 == NULL) {
			exit(1);
		}

#if 0
		for(i=0; i < sum->ydim; i++) {
			for(k=0; k < sum->xdim; k++) {
				sum->data[i*sum->xdim+k] =
					sum->data[i*sum->xdim+k] +
				        rank_1->data[i*rank_1->xdim+k];
			}
		}
#endif
		Y = DiagonalAverage(rank_1);
		for(i=0; i < sum->ydim; i++) {
			for(k=0; k < sum->xdim; k++) {
				sum->data[i*sum->xdim+k] =
					sum->data[i*sum->xdim+k] +
				        Y->data[i*Y->xdim+k];
			}
		}
		FreeArray2D(rank_1);
		FreeArray2D(Y);

	}

#if 0
	Y = DiagonalAverage(sum);
	FreeArray2D(sum);
#endif

	FreeArray1D(U_col);
	FreeArray2D(V_row);
	FreeArray2D(U);
	FreeArray2D(V_t);
	FreeArray1D(ev);
	FreeArray2D(D);
	FreeArray2D(D2);
	FreeArray2D(Dm2);

#if 0
	return(Y);
#endif
	return(sum);
		
}


double Omega(int i, int L, int N)
{
        int K;
        int L_star;
        int K_star;
        int omega;

        K = N - L + 1;

        if(L < K) {
                L_star = L;
                K_star = K;
        } else {
                L_star = K;
                K_star = L;
        }

        if((i >=0) && (i <= (L_star - 1))) {
                omega = i+1;
        } else if((i <= L_star) && (i <= K_star)) {
                i = L_star;
        } else {
                i = N - 1;
        }

        return(omega);
}

double InnerProduct(Array1D *f_1, Array1D *f_2, int L, int K)
{
        double sum;
        int i;
        int N;

        N = L + K - 1;

        sum = 0;
        for(i=0; i < N; i++) {
                sum += (Omega(i,L,K) * f_1->data[i] * f_2->data[i]);
        }

        return(sum);

}

double Corr(Array1D *x, Array1D *y)
{
	int i;
	double x_bar;
	double y_bar;
	double x2;
	double y2;
	double count;
	double acc;
	double num;
	double r;

	acc = 0;
	count = 0;
	for(i=0; i < x->ydim; i++) {
		acc += x->data[i];
		count++;
	}
	x_bar = acc / count;

	acc = 0;
	count = 0;
	for(i=0; i < y->ydim; i++) {
		acc += y->data[i];
		count++;
	}
	y_bar = acc / count;

	acc = 0;
	count = 0;
	for(i=0; i < x->ydim; i++) {
		acc += ((x->data[i] - x_bar) * (y->data[i] - y_bar));
		count++;
	}
	num = acc;

	acc = 0;
	count = 0;
	for(i=0; i < x->ydim; i++) {
		acc += ((x->data[i] - x_bar) * (x->data[i] - x_bar));
		count++;
	}
	x2 = acc;

	acc = 0;
	count = 0;
	for(i=0; i < y->ydim; i++) {
		acc += ((y->data[i] - y_bar) * (y->data[i] - y_bar));
		count++;
	}
	y2 = acc;

	r = num / sqrt(x2 * y2);

	return(fabs(r));
} 
		


double WCorr(Array2D *tr_x, Array2D *tr_y, int L, int K) {
        double rho;
        double ip1;
        double ip2;
        double ip3;

        ip1 = InnerProduct(tr_x, tr_y, L, K);
        ip2 = InnerProduct(tr_x, tr_x, L, K);
        ip3 = InnerProduct(tr_y, tr_y, L, K);

        rho = ip1 / ((sqrt(ip2) * sqrt(ip3)));

        return(rho);
}




/*
 * https://www.ehdp.com/vitalnet/breaks-1.htm
 */
double JenksDevEigen(Array1D *ev, int split)
{
	int i;
	double acc;
	double total;
	double avg;
	double avg1;
	double avg2;
	double v1;
	double v2;

	if(split == 0) {
		acc = 0;
		total = 0;
		for(i=0; i < ev->ydim; i++) {
			acc += ev->data[i];
			total += 1;
		}
		avg = acc /total;
		acc = 0;
		for(i=0; i < ev->ydim; i++) {
			acc += ((ev->data[i] - avg) * (ev->data[i] - avg));
		}
		v1 = acc;
		v2 = 0;
	} else {
		acc = 0;
		total = 0;
		for(i=0; i < split; i++) {
			acc += ev->data[i];
                        total += 1;
		}
		avg1 = acc/total;
		acc = 0;
		for(i=0; i < split; i++) {
			acc += ((ev->data[i] - avg1) * (ev->data[i] - avg1));
		}
		v1 = acc;
		
		acc = 0;
		total = 0;
		for(i=split; i < ev->ydim; i++) {
			acc += ev->data[i];
                        total += 1;
		}
		avg2 = acc/total;
		acc = 0;
		for(i=split; i < ev->ydim; i++) {
			acc += ((ev->data[i] - avg2) * (ev->data[i] - avg2));
		}
		v2 = acc;
	}
	return(v1+v2);
}

int JenksSplitEigen(Array1D *ev)
{
	int i;
	int j;
	int split;
	double global_dev;
	double class_dev;
	double min_dev = 1e100;
	int min_split;
	double gof;

return(1);

	global_dev = JenksDevEigen(ev,0);
	for(split = 1; split < ev->ydim-1; split++) {
		class_dev = JenksDevEigen(ev,split); 
		if(class_dev < min_dev) {
			min_split = split;
			min_dev = class_dev;
		}
		gof = (global_dev - class_dev) / global_dev;
//printf("split: %d, class: %f, min_split: %d gof: %f\n",
//			split,
//			class_dev,
//			min_split,
//			gof);
		fflush(stdout);
	}

	return(min_split);

}

Array2D *SSASmoothSeries(Array2D *time_series, int lags)
{
	Array1D *data_series;
	Array1D *smooth1D;
	Array2D *smooth2D;
	int i;

	data_series = MakeArray1D(time_series->ydim);
	if(data_series == NULL) {
		return(NULL);
	}

	for(i=0; i < time_series->ydim; i++) {
		data_series->data[i] = time_series->data[i*2+1];
	}

	smooth1D = SSADecomposition(data_series,lags,0,time_series->ydim);
	if(smooth1D == NULL) {
		FreeArray1D(data_series);
		return(NULL);
	}

	FreeArray1D(data_series);

	smooth2D = MakeArray2D(time_series->ydim,2);
	if(smooth2D == NULL) {
		FreeArray1D(smooth1D);
		return(NULL);
	}

	for(i=0; i < time_series->ydim; i++) {
		smooth2D->data[i*2+0] = time_series->data[i*2+0];
		smooth2D->data[i*2+1] = smooth1D->data[i];
	}

	FreeArray1D(smooth1D);

	return(smooth2D);
}
		
		

		


#ifdef STANDALONE



int SignalRange(char *range, int *start, int *end)
{
	char buf[4096];
	int s;
	int e;
	char *cursor;
	int i;

	
	memset(buf,0,sizeof(buf));
	cursor = range;
	i = 0;
	while(*cursor != 0) {
		if(isspace(*cursor)) {
			cursor++;
			continue;
		}
		if(*cursor == '-') {
			break;
		}
		if(isdigit(*cursor)) {
			buf[i] = *cursor;
			i++;
			if(i == 4096) {
				break;
			}
		}
		cursor++;
	}

	if(i == 4096) {
		return(-1);
	}

	if(i == 0) {
		s = -1;
	} else {
		s = atoi(buf);
	}

	cursor++;
	memset(buf,0,sizeof(buf));
	i = 0;
	while(*cursor != 0) {
		if(isspace(*cursor)) {
			cursor++;
			continue;
		}
		if(isdigit(*cursor)) {
			buf[i] = *cursor;
			i++;
			if(i == 4096) {
				break;
			}
		}
		cursor++;
	}

	if(i == 4096) {
		return(-1);
	}

	/*
	 * only one value signifies end of range -- not start
	 */
	if(i == 0) {
		e = s;
		s = -1;
	} else {
		e = atoi(buf);
	}

	*start = s;
	*end = e;

	return(1);
}
		
#define ARGS "x:l:N:e:p:m:"
char *Usage = "usage: ssa-decomp -x xfile\n\
\t-e range of signal series (start - end || end)\n\
\t-l lags\n\
\t-m means (for k-means)\n\
\t-N number of time series elements in base matrix\n\
\t-p starting index\n";

char Xfile[4096];

int main(int argc, char *argv[])
{
	int c;
	int size;
	MIO *d_mio;
	MIO *xmio;
	Array1D *x;
	Array1D *x_sub;
	int lags;
	int p;
	int i;
	int j;
	double rho;
	int K;
	int N;
	int signal;
	Array1D *series_signal;
	double x_n;
	int err;
	int start;
	int end;
	Bin **bins;
	Array1D **rank_arrays;
	int means;
	int split;
	

	N = 0;
	p = 0;
	start = -1;
	end = -1;
	means = 2;
	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'x':
				strncpy(Xfile,optarg,sizeof(Xfile));
				break;
			case 'e':
				err = SignalRange(optarg,&start,&end);
				break;
			case 'l':
				lags = atoi(optarg);
				break;
			case 'p':
				p = atoi(optarg);
				break;
			case 'N':
				N = atoi(optarg);
				break;
			case 'm':
				means = atoi(optarg);
				break;
			default:
				fprintf(stderr,
			"unrecognized command: %c\n",(char)c);
				fprintf(stderr,"%s",Usage);
				exit(1);
		}
	}

	if(err < 0) {
		fprintf(stderr,"error in -e argument\n");
		fprintf(stderr,"%s",Usage);
		exit(1);
	}

	if(Xfile[0] == 0) {
		fprintf(stderr,"must specify xfile\n");
		fprintf(stderr,"%s",Usage);
		exit(1);
	}

	if(N == 0) {
		fprintf(stderr,"must enter window size\n");
		fprintf(stderr,"%s",Usage);
		exit(1);
	}

	size = MIOSize(Xfile);
	d_mio = MIOOpenText(Xfile,"r",size);
	if(d_mio == NULL) {
		fprintf(stderr,"couldn't open %s\n",Xfile);
		exit(1);
	}
	xmio = MIODoubleFromText(d_mio,NULL);
	if(xmio == NULL) {
		fprintf(stderr,"no valid data in %s\n",Xfile);
		exit(1);
	}

	x = MakeArray2DFromMIO(xmio);

	if(x == NULL) {
		exit(1);
	}

	if(start == -1) {
		start = 0;
#if 0
		rank_arrays = SSARank1(x,lags,0,lags);
		if(rank_arrays == NULL) {
			exit(1);
		}
		K = x->ydim - lags + 1;
		split = JenksSplit(rank_arrays,lags,lags,K);
		printf("best jenks split: %d\n",i);
		bins = KMeans(rank_arrays,lags,K,means);
		for(i=0; i < means; i++) {
			printf("Bin: %d\n",i);
			PrintBin(stdout,bins[i]);
		}
		free(bins);
		free(rank_arrays);
#endif
	} else {
		if(start > 0) {
			start = start - 1;
		}
	}

	if(end == -1) {
		end = lags - 1;
	}


	if(p == 0) { // if p == 0, start from beginning of x
		series_signal = SSADecomposition(x,lags,start,end);
	} else {
		x_sub = MakeArray1D(x->ydim - p);
		if(x_sub == NULL) {
			exit(1);
		}
		for(i=p; i < x->ydim; i++) {
			x_sub->data[i-p] = x->data[i];
		}
		series_signal = SSADecomposition(x_sub,lags,start,end);
		FreeArray1D(x_sub);
	}

	for(i=0; i < series_signal->ydim; i++) {
		printf("%f\n",series_signal->data[i]);
	}


	return(0);
}

#endif
