#ifndef NORM_H
#define NORM_H

double Normal(double y, double mu, double sigma);
double NormalCDF(double x, double mu, double sigma);
double InvNormal(double p, double mu, double sigma);
double NormalCDF(double x, double mu, double sigma);

double InvNormalCondBetween(double r,
                double a, double b, double mu, double sigma);
double InvNormalCondLess(double r, double b, double mu, double sigma);
double InvNormalCondGreater(double r, double a, double mu, double sigma);

#endif

