#ifndef _ADT_INTEGRAL_H
#define _ADT_INTEGRAL_H

#include <stdio.h>
#include <math.h>


typedef double (*fun)(double, int, double[], double[]);

double simpson(fun f, double a, double b, int n,int index, double x[],double u[]);


#endif
