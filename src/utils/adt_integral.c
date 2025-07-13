#include "adt_integral.h"





double simpson(fun f, double a, double b, int n, int index, double x[], double u[]) {
    double h = (b - a) / n;
    double sum = f(a, index, x, u) + f(b, index, x, u);
    for (int i = 1; i < n; i += 2) {
        sum += 4 * f(a + i * h, index, x, u);
    }
    for (int i = 2; i < n - 1; i += 2) {
        sum += 2 * f(a + i * h, index, x, u);
    }
    return sum * h / 3.0;
}