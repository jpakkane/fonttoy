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

typedef double (*fptr)(const double *, const int);

void estimate_derivative(fptr f, const double *x, double *g, double f0, double *h, double N) {
    std::vector<double> x0(N);
    int i;
    for(int i = 0; i < N; i++) {
        x0[i] = x[i];
    }
    for(i = 0; i < N; i++) {
        double old_v = x0[i];
        x0[i] += h[i];
        double dx = h[i];
        double df = f(&x0[0], N) - f0;
        g[i] = df / dx;
        x0[i] = old_v;
    }
}

double maxd(const double d1, const double d2) { return d1 > d2 ? d1 : d2; }

void compute_absolute_step(double rel_step, const double *x, double *h, const int N) {
    for(int i = 0; i < N; ++i) {
        double sign_x = x[i] >= 0 ? 1.0 : -1.0;
        h[i] = rel_step * sign_x * maxd(1.0, fabs(x[i]));
    }
}

double fn(const double *x, const int n) {
    double result = 0.0;
    for(int i = 0; i < n; i++) {
        const double curx = x[i];
        result += curx * curx - curx;
    }
    return result;
}

static lbfgsfloatval_t evaluate(void *instance,
                                const lbfgsfloatval_t *x,
                                lbfgsfloatval_t *g,
                                const int n,
                                const lbfgsfloatval_t step) {
    (void)instance;
    (void)step;
    const double rel_step = 0.000000001;
    double fx = 0.0;
    std::vector<double> h(n);
    fx = fn(x, n);
    compute_absolute_step(rel_step, x, &h[0], n);
    estimate_derivative(fn, x, g, fx, &h[0], n);
    return fx;
}

static int progress(void *,
                    const lbfgsfloatval_t *,
                    const lbfgsfloatval_t *,
                    const lbfgsfloatval_t,
                    const lbfgsfloatval_t,
                    const lbfgsfloatval_t,
                    const lbfgsfloatval_t,
                    int,
                    int,
                    int) {
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
    for(const auto &p : params) {
        printf(" %f\n", p);
    }
}

int main2(int, char **) {
    SvgExporter e;
    std::vector<Vector> v;
    v.emplace_back(1.0, 2.0);
    v.push_back({1.1, 2.2});
    printf("Val: %f.\n", v.back().x());
    v.pop_back();
    printf("Val: %f.\n", v.back().x());
    calculate_with_lbfgs();
    e.write_svg("test.svg");
    return 0;
}


struct OptimizationData {
    Stroke *s;
    std::vector<double> variables;
};

std::vector<double> compute_absolute_step(double rel_step, const std::vector<double> &x) {
    std::vector<double> h;
    h.reserve(x.size());
    for(size_t i = 0; i < x.size(); ++i) {
        double sign_x = x[i] >= 0 ? 1.0 : -1.0;
        h.push_back(rel_step * sign_x * maxd(1.0, fabs(x[i])));
    }
    return h;
}

std::vector<double> estimate_derivative(Stroke *s, const std::vector<double> &x, double f0, const std::vector<double> &h) {
    std::vector<double> g(x.size());
    std::vector<double> x0 = x;
    for(size_t i = 0; i < x.size(); i++) {
        double old_v = x0[i];
        x0[i] += h[i];
        double dx = h[i];
        double df = s->calculate_value_for(x0) - f0;
        g[i] = df / dx;
        x0[i] = old_v;
    }
    return g;
}


static lbfgsfloatval_t evaluate_model(void *instance,
                                const lbfgsfloatval_t *x,
                                lbfgsfloatval_t *g,
                                const int n,
                                const lbfgsfloatval_t step) {
    auto s = reinterpret_cast<Stroke*>(instance);
    (void)step;
    const double rel_step = 0.000000001;
    double fx = 0.0;
    std::vector<double> curx(x, x+n);
    fx = s->calculate_value_for(curx);
    auto curh = compute_absolute_step(rel_step, curx);
    auto g_est = estimate_derivative(s, curx, fx, curh);
    for(int i=0; i<n; i++) {
        g[i] = g_est[i];
    }
    return fx;
}

void optimize(Stroke *s) {
    double final_result = 1e8;
    auto variables = s->get_free_variables();
    assert(variables.size() == 7);

    lbfgs_parameter_t param;
    lbfgs_parameter_init(&param);
    int ret = lbfgs(variables.size(), &variables[0], &final_result, evaluate_model, nullptr, &s, &param);
    printf("Exit value: %d\n", ret);
}

int main(int, char **) {
    SvgExporter e;
    Stroke s(6);
    const double h = 1.0;
    const double w = 0.7;
    const double e1 = 0.2;
    const double e2 = h - e1;
    const double e3 = 0.27;
    const double e4 = h - e3;
    const double r = 0.05;

    // All points first
    s.add_constraint(std::make_unique<FixedConstraint>(0, Point(r, e1)));
    s.add_constraint(std::make_unique<FixedConstraint>(3, Point(w / 2, r)));
    s.add_constraint(std::make_unique<FixedConstraint>(6, Point(w - r, e3)));
    s.add_constraint(std::make_unique<FixedConstraint>(9, Point(w / 2, h / 2)));
    s.add_constraint(std::make_unique<FixedConstraint>(12, Point(r, e4)));
    s.add_constraint(std::make_unique<FixedConstraint>(15, Point(w / 2, h - r)));
    s.add_constraint(std::make_unique<FixedConstraint>(18, Point(w - r, e2)));

    // Then control points.
    s.add_constraint(std::make_unique<DirectionConstraint>(3, 2, M_PI));
    s.add_constraint(std::make_unique<MirrorConstraint>(4, 2, 3));
    s.add_constraint(std::make_unique<DirectionConstraint>(6, 5, 3.0 * M_PI / 2.0));
    s.add_constraint(std::make_unique<SmoothConstraint>(7, 5, 6));
    s.add_constraint(std::make_unique<AngleConstraint>(
        8, 9, (360.0 - 15.0) / 360.0 * 2.0 * M_PI, (360.0 - 1.0) / 360.0 * 2.0 * M_PI));
    s.add_constraint(std::make_unique<MirrorConstraint>(10, 8, 9));
    s.add_constraint(std::make_unique<DirectionConstraint>(12, 11, 3.0 * M_PI / 2.0));
    s.add_constraint(std::make_unique<SmoothConstraint>(13, 11, 12));
    s.add_constraint(std::make_unique<SameOffsetConstraint>(14, 15, 2, 3));
    s.add_constraint(std::make_unique<MirrorConstraint>(16, 14, 15));
    s.add_constraint(std::make_unique<SameOffsetConstraint>(17, 18, 0, 1));

    optimize(&s);

    // e.write_svg("ess.svg");
    printf("All done, bye-bye.\n");
    return 0;
}
