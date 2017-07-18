
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <sys/time.h>

#define RAND() (drand48())
#define SEED(s) (srand48(s))


double Mu;
double Sigma;
int Samples;
double Low;
double High;
int Values;
int CDF;


#define PI (3.14159265)

#define Usage "normal -m mu -s sigma -c sample_count -l low -h high [-VC]\n"

#define ARGS "m:s:c:h:l:VC"

/*
 * http://mathworld.wolfram.com/NormalDistribution.html
 */
double NormalCDF(double x, double mu, double sigma)
{

	return(0.5 * (1.0 + erf((x - mu) / (sqrt(2.0)*sigma))));
}

	

double
Normal(double y, double mu, double sigma)
{
        double temp;


        temp = exp(-0.5 * ((y - mu)/sigma) * ((y - mu)/sigma));
        temp = temp / (sqrt(2.0 * PI) * sigma);

        return(temp);

}

/*
 * Lower tail quantile for standard normal distribution function.
 *
 * This function returns an approximation of the inverse cumulative
 * standard normal distribution function.  I.e., given P, it returns
 * an approximation to the X satisfying P = Pr{Z <= X} where Z is a
 * random variable from the standard normal distribution.
 *
 * The algorithm uses a minimax approximation by rational functions
 * and the result has a relative error whose absolute value is less
 * than 1.15e-9.
 *
 * Author:      Peter J. Acklam
 * Time-stamp:  2002-06-09 18:45:44 +0200
 * E-mail:      jacklam@math.uio.no
 * WWW URL:     http://www.math.uio.no/~jacklam
 *
 * C implementation adapted from Peter's Perl version
 */


/* Coefficients in rational approximations. */
static const double a[] =
{
	-3.969683028665376e+01,
	 2.209460984245205e+02,
	-2.759285104469687e+02,
	 1.383577518672690e+02,
	-3.066479806614716e+01,
	 2.506628277459239e+00
};

static const double b[] =
{
	-5.447609879822406e+01,
	 1.615858368580409e+02,
	-1.556989798598866e+02,
	 6.680131188771972e+01,
	-1.328068155288572e+01
};

static const double c[] =
{
	-7.784894002430293e-03,
	-3.223964580411365e-01,
	-2.400758277161838e+00,
	-2.549732539343734e+00,
	 4.374664141464968e+00,
	 2.938163982698783e+00
};

static const double d[] =
{
	7.784695709041462e-03,
	3.224671290700398e-01,
	2.445134137142996e+00,
	3.754408661907416e+00
};

#define LOW 0.02425
#define HIGH 0.97575

/*
 * http://home.online.no/~pjacklam/notes/invnorm/impl/sprouse/ltqnorm.c
 */
double
InvNormal(double p, double mu, double sigma)
{
	double q, r;
	double value;
	double x;
	double u;
	double e;

	errno = 0;

	if (p < 0 || p > 1)
	{
		errno = EDOM;
		return 0.0;
	}
	else if (p == 0)
	{
		errno = ERANGE;
		return -HUGE_VAL /* minus "infinity" */;
	}
	else if (p == 1)
	{
		errno = ERANGE;
		return HUGE_VAL /* "infinity" */;
	}
	else if (p < LOW)
	{
		/* Rational approximation for lower region */
		q = sqrt(-2*log(p));
		value = (((((c[0]*q+c[1])*q+c[2])*q+c[3])*q+c[4])*q+c[5]) /
			((((d[0]*q+d[1])*q+d[2])*q+d[3])*q+1);
		x = value;
	}
	else if (p > HIGH)
	{
		/* Rational approximation for upper region */
		q  = sqrt(-2*log(1-p));
		value =  -(((((c[0]*q+c[1])*q+c[2])*q+c[3])*q+c[4])*q+c[5]) /
			((((d[0]*q+d[1])*q+d[2])*q+d[3])*q+1);
		x = value;
	}
	else
	{
		/* Rational approximation for central region */
    		q = p - 0.5;
    		r = q*q;
		value =  (((((a[0]*r+a[1])*r+a[2])*r+a[3])*r+a[4])*r+a[5])*q /
			(((((b[0]*r+b[1])*r+b[2])*r+b[3])*r+b[4])*r+1);
		x = value;
	}

	if(( 0 < p)&&(p < 1))
	{
		e = 0.5 * erfc(-x/sqrt(2)) - p;
		u = e * sqrt(2*M_PI) * exp(x*x/2);
		x = x - u/(1 + x*u/2);
	}

	return(x*sigma+mu);

}

double InvNormalCondBetween(double r, 
		double a, double b, double mu, double sigma)
{
	double lower;
	double upper;
	double ret;

	lower = NormalCDF(a,mu,sigma);
	upper = NormalCDF(b,mu,sigma);

	ret = InvNormal(r*upper+(1.0-r)*lower,mu,sigma);

	return(ret);
}

double InvNormalCondLess(double r, double b, double mu, double sigma)
{
	double lower;
	double upper;
	double ret;

	lower = 0.0;
	upper = NormalCDF(b,mu,sigma);

	ret = InvNormal(r*upper+(1.0-r)*lower,mu,sigma);

	return(ret);
}

double InvNormalCondGreater(double r, double a, double mu, double sigma)
{
	double lower;
	double upper;
	double ret;

	lower = NormalCDF(a,mu,sigma);;
	upper = 1.0;

	ret = InvNormal(r*upper+(1.0-r)*lower,mu,sigma);

	return(ret);
}

#ifdef STANDALONE
main(int argc, char *argv[])
{
	int c;
	int i;
	int j;
	double r;
	double y;
	double value;
	double acc;
	double incr;
	double curr;
	struct timeval tm;

	gettimeofday(&tm,NULL);
	SEED(tm.tv_sec+tm.tv_usec);

	Mu = 0.0;
	Sigma = 1.0;
	Samples = -1;
	Low = -1.0;
	High = 1.0;
	Values = 0;
	CDF = 0;

	while((c = getopt(argc,argv,ARGS)) != EOF)
	{
		switch(c)
		{
			case 'm':
				Mu = atof(optarg);
				break;
			case 's':
				Sigma = atof(optarg);
				break;
			case 'c':
				Samples = atoi(optarg);
				break;
			case 'C':
				CDF = 1;
				break;
			case 'l':
				Low = atoi(optarg);
				break;
			case 'h':
				High = atoi(optarg);
				break;
			case 'V':
				Values = 1;
				break;
			default:
				fprintf(stderr,"unrecognize arg: %c\n",c);
				fprintf(stderr,"usage: %s\n",Usage);
				fflush(stderr);
				exit(1);
		}
	}

	if(Samples < 1)
	{
		fprintf(stderr,"usage: %s\n",Usage);
		fflush(stderr);
		exit(1);
	}

	if(CDF == 0)
	{
		for(i=0; i < Samples; i++)
		{
			r = RAND();
			
			if(Values == 0)
			{
				value = Low + (High - Low)*r;
				y = Normal(value,Mu,Sigma);
				fprintf(stdout,"%3.4f %3.4f\n",value,y);
			}
			else
			{
				value = InvNormal(r,Mu,Sigma);
				fprintf(stdout,"%f\n",value);
			}
					
		}

		fflush(stdout);
	}
	else
	{
		incr = (High-Low)/(double)Samples;
		curr = Low;
		acc = 0;
		for(j = 0; j < Samples; j++)
		{
			y = Normal(curr,Mu,Sigma);
			acc += (y*incr);
			fprintf(stdout,"%3.4f %3.4f\n",
					curr,acc);
			curr += incr;
		}
		fflush(stdout);
	}


	exit(0);
}


#endif
