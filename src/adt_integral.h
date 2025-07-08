#ifndef _ADT_INTEGRAL_H
#define _ADT_INTEGRAL_H

#include <stdio.h>
#include <math.h>


typedef double (*fun)(double x);

double quadratic_test(double x);

double riemann_sum(fun f, double a , double b, int n);

double mid_point(fun f, double a, double b, int n);

double trapez(fun f, double a, double b, int n);

double simpson(fun f, double a, double b, int n);

double quad_gauss(fun f, double a, double b);


#endif
