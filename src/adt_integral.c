#include "adt_integral.h"



double quadratic_test(double x){
    return x*x;
}

double riemann_sum(fun f, double a, double b, int n) {
    double h = (b - a) / n;
    double sum = 0.0;
    for (int i = 0; i < n; i++) {
        sum += f(a + i * h) * h;
    }
    return sum;
};

double mid_point(fun f, double a, double b, int n) {
    double h = (b - a) / n;
    double sum = 0.0;
    for (int i = 0; i < n; i++) {
        double x_mid = a + (i + 0.5) * h;
        sum += f(x_mid) * h;
    }
    return sum;
};

double trapez(fun f, double a, double b, int n) {
    double h = (b - a) / n;
    double sum = 0.5 * (f(a) + f(b));
    for (int i = 1; i < n; i++) {
        sum += f(a + i * h);
    }
    return sum * h;
};

double simpson(fun f, double a, double b, int n) {
    if (n % 2 != 0) n++; 
    double h = (b - a) / n;
    double sum = f(a) + f(b);
    for (int i = 1; i < n; i++) {
        if (i % 2 == 0)
            sum += 2 * f(a + i * h);
        else
            sum += 4 * f(a + i * h);
    }
    return sum * h / 3.0;
};

double quad_gauss(fun f, double a, double b){
    static const double nodes[4] = {
        -0.8611363115940526,
        -0.3399810435848563,
        0.3399810435848563,
        0.8611363115940526
    };

    static const double weights[4] = {
        0.3478548451374538,
        0.6521451548625461,
        0.6521451548625461,
        0.3478548451374538
    };
    double half_length = (b-a) /2.0;
    double midpoint = (a+b) /2.0;

    double integral = 0.0;

    for(int i = 0 ; i < 4 ; i++){
        double x = midpoint +half_length * nodes[i];
        integral += weights[i] * f(x);
    }

    integral *= half_length;

    return integral;

}
