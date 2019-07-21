/*
  Copyright (C) 2019 Jussi Pakkanen

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, see <http://www.gnu.org/licenses/>.
*/

#include <cstdio>
#include <fonttoy.hpp>
#include <constraints.hpp>
#include <svgexporter.hpp>
#include <vector>
#include <lbfgs.h>
#include <cassert>
#include <cmath>

static_assert(sizeof(lbfgsfloatval_t) == sizeof(double));

typedef double (*fptr)(const double*, const int);

void estimate_derivative(fptr f, const double *x, double *g, double f0, double *h, double N) {
    std::vector<double> x0(N);
    int i;
    for(int i=0; i<N; i++) {
        x0[i] = x[i];
    }
    for(i=0; i<N; i++) {
        double old_v = x0[i];
        x0[i] += h[i];
        double dx = h[i];
        double df = f(&x0[0], N) - f0;
        g[i] = df / dx;
        x0[i] = old_v;
    }
}

double maxd(const double d1, const double d2) {
    return d1 > d2 ? d1 : d2;
}

void compute_absolute_step(double rel_step, const double *x, double *h, const int N) {
    for(int i=0; i<N; ++i) {
        double sign_x = x[i] >= 0 ? 1.0 : -1.0;
        h[i] = rel_step * sign_x * maxd(1.0, fabs(x[i]));
    }
}

double fn(const double *x, const int n) {
    double result = 0.0;
    for(int i=0; i<n; i++) {
        const double curx = x[i];
        result += curx*curx - curx;
    }
    return result;
}


static lbfgsfloatval_t evaluate(void *instance,
                                const lbfgsfloatval_t *x,
                                lbfgsfloatval_t *g,
                                const int n,
                                const lbfgsfloatval_t step) {
    const double rel_step = 0.000000001;
    double fx = 0.0;
    std::vector<double> h(n);
    fx = fn(x, n);
    compute_absolute_step(rel_step, x, &h[0], n);
    estimate_derivative(fn, x, g, fx, &h[0], n);
    return fx;
}

static int progress(
    void *instance,
    const lbfgsfloatval_t *x,
    const lbfgsfloatval_t *g,
    const lbfgsfloatval_t fx,
    const lbfgsfloatval_t xnorm,
    const lbfgsfloatval_t gnorm,
    const lbfgsfloatval_t step,
    int n,
    int k,
    int ls
    )
{
    return 0;
}

void calculate_with_lbfgs() {
    const int N = 10;
    double fx = 0.0;
    lbfgs_parameter_t param;
    lbfgs_parameter_init(&param);
    std::vector<double> params(N);
    int ret = lbfgs(N, &params[0], &fx, evaluate, progress, NULL, &param);

    printf("L-BFGS optimization terminated with status code = %d\n", ret);
    printf("Value: %f\nParameters:\n", fx);
    for(const auto &p: params) {
        printf(" %f\n", p);
    }
}

int main(int, char **) {
    std::vector<Vector> v;
    v.emplace_back(1.0, 2.0);
    v.push_back({1.1, 2.2});
    printf("Val: %f.\n", v.back().x());
    v.pop_back();
    printf("Val: %f.\n", v.back().x());
    calculate_with_lbfgs();
    write_svg("test.svg");
    return 0;
}
