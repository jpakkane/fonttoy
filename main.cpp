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

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif

#include <cstdio>
#include <fonttoy.hpp>
#include <constraints.hpp>
#include <svgexporter.hpp>
#include <parser.hpp>
#include <vector>
#include <lbfgs.h>
#include <cassert>
#include <cmath>

static bool debug_svgs = false;

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

std::vector<double> compute_absolute_step(double rel_step, const std::vector<double> &x) {
    std::vector<double> h;
    h.reserve(x.size());
    for(size_t i = 0; i < x.size(); ++i) {
        double sign_x = x[i] >= 0 ? 1.0 : -1.0;
        h.push_back(rel_step * sign_x * maxd(1.0, fabs(x[i])));
    }
    return h;
}

std::vector<double> estimate_derivative(Stroke *s,
                                        const std::vector<double> &x,
                                        double f0,
                                        const std::vector<double> &h) {
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

void put_beziers_in(Stroke &s, SvgExporter &svg) {
    for(const auto b : s.build_beziers()) {
        svg.draw_bezier(b.p1(), b.c1(), b.c2(), b.p2(), true);
    }
}

void write_svg(Stroke &s, const char *fname) {
    SvgExporter svg;
    put_beziers_in(s, svg);
    svg.write_svg(fname);
}

static lbfgsfloatval_t evaluate_model(void *instance,
                                      const lbfgsfloatval_t *x,
                                      lbfgsfloatval_t *g,
                                      const int n,
                                      const lbfgsfloatval_t step) {
    static int num = 0;
    auto s = reinterpret_cast<Stroke *>(instance);
    (void)step;
    const double rel_step = 0.000000001;
    double fx = 0.0;
    std::vector<double> curx(x, x + n);
    fx = s->calculate_value_for(curx);
    if(debug_svgs) {
        char buf[256];
        sprintf(buf, "eval%03d.svg", num++);
        write_svg(*s, buf);
    }
    auto curh = compute_absolute_step(rel_step, curx);
    auto g_est = estimate_derivative(s, curx, fx, curh);
    for(int i = 0; i < n; i++) {
        g[i] = g_est[i];
    }
    printf("Evaluation: %f\n", fx);
    return fx;
}

int model_progress(void *instance,
                   const lbfgsfloatval_t *,
                   const lbfgsfloatval_t *,
                   const lbfgsfloatval_t,
                   const lbfgsfloatval_t,
                   const lbfgsfloatval_t,
                   const lbfgsfloatval_t,
                   int,
                   int k,
                   int) {
    printf("Iteration %d\n", k);
    Stroke *s = reinterpret_cast<Stroke *>(instance);
    if(debug_svgs) {
        char buf[128];
        sprintf(buf, "step%d.svg", k);
        write_svg(*s, buf);
    }
    return 0;
}

void optimize(Stroke *s) {
    double final_result = 1e8;
    auto variables = s->get_free_variables();
    assert(variables.size() == 7);
    s->freeze();
    variables = s->get_free_variables();
    assert(variables.size() == 9);

    s->calculate_value_for(variables);
    if(debug_svgs) {
        write_svg(*s, "initial.svg");
    }

    lbfgs_parameter_t param;
    lbfgs_parameter_init(&param);
    int ret = lbfgs(
        variables.size(), &variables[0], &final_result, evaluate_model, model_progress, s, &param);
    printf("Exit value: %d\n", ret);
    // insert final values back in the stroke here.
    s->calculate_value_for(variables);
}

Stroke calculate_sample() {
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
    return s;
}

class Bridge : public ExternalFuncall {
public:
    funcall_result funcall(const std::string &funname, const std::vector<double> &args) override {
        if(funname == "Stroke") {
            if(s) {
                return "Second call to stroke.";
            }
            if(args.size() != 1) {
                return "Wrong number of arguments.";
            }
            s.reset(new Stroke(args[0]));
            return 0.0;
        } else if(funname == "FixedConstraint") {
            if(!s) {
                return "Stroke not set.";
            }
            if(args.size() != 3) {
                return "Wrong number of arguments.";
            }
            s->add_constraint(std::make_unique<FixedConstraint>(args[0], Point(args[1], args[2])));
            return 0.0;
        } else if(funname == "DirectionConstraint") {
            if(!s) {
                return "Stroke not set.";
            }
            if(args.size() != 3) {
                return "Wrong number of arguments.";
            }
            s->add_constraint(std::make_unique<DirectionConstraint>(args[0], args[1], args[2]));
            return 0.0;
        } else if(funname == "MirrorConstraint") {
            if(!s) {
                return "Stroke not set.";
            }
            if(args.size() != 3) {
                return "Wrong number of arguments.";
            }
            s->add_constraint(std::make_unique<MirrorConstraint>(args[0], args[1], args[2]));
            return 0.0;
        } else if(funname == "SmoothConstraint") {
            if(!s) {
                return "Stroke not set.";
            }
            if(args.size() != 3) {
                return "Wrong number of arguments.";
            }
            s->add_constraint(std::make_unique<SmoothConstraint>(args[0], args[1], args[2]));
            return 0.0;
        } else if(funname == "AngleConstraint") {
            if(!s) {
                return "Stroke not set.";
            }
            if(args.size() != 4) {
                return "Wrong number of arguments.";
            }
            s->add_constraint(
                std::make_unique<AngleConstraint>(args[0], args[1], args[2], args[3]));
            return 0.0;
        } else if(funname == "SameOffsetConstraint") {
            if(!s) {
                return "Stroke not set.";
            }
            if(args.size() != 4) {
                return "Wrong number of arguments.";
            }
            s->add_constraint(
                std::make_unique<SameOffsetConstraint>(args[0], args[1], args[2], args[3]));
            return 0.0;
        } else {
            return "Unknown function.";
        }
        return 0;
    }

    Stroke &get_stroke() {
        assert(s);
        return *s.get();
    }

private:
    std::unique_ptr<Stroke> s;
};

std::string read_file(const char *fname) {
    FILE *f = fopen(fname, "r");
    assert(f);
    const int bufsize = 1000000;
    // Must be heap allocated so no std::array.
    std::unique_ptr<char[]> buf(new(char[bufsize]));
    auto num_read = fread(buf.get(), 1, bufsize, f);
    fclose(f);
    return std::string(buf.get(), buf.get() + num_read);
}

Stroke calculate_sample_dynamically(const char *fname) {
    std::string program = read_file(fname);
    Bridge b;
    Lexer l(program);
    Parser p(l);
    Interpreter i(p, &b);

    if(!p.parse()) {
        printf("Parser fail: %s\n", p.get_error().c_str());
        assert(0);
    }
    if(!i.execute_program()) {
        printf("Interpreter fail: %s\n", i.get_error().c_str());
        assert(0);
    }
    optimize(&b.get_stroke());
    return std::move(b.get_stroke());
}

#if defined(WASM)

extern "C" {

// If this function is not referenced from main() below, emcc will just
// remove it. That was a "fun" debugging experience.
int wasm_entrypoint(char *buf) {
    SvgExporter e;
    Stroke s = calculate_sample();
    SvgExporter svg;
    put_beziers_in(s, svg);
    std::string result = svg.to_string();
    strcpy(buf, result.c_str());
    printf("Something.\n");
    return 42;
}
}

int main(int argc, char **) {
    printf("Initialized.\n");
    if(argc == 1234567) {
        wasm_entrypoint(nullptr);
    }
    return 0;
}

#else

int main(int argc, char **argv) {
    debug_svgs = true;
    SvgExporter e;
    if(argc != 2) {
        printf("%s <input file>\n", argv[0]);
        return 1;
    }
    Stroke s = calculate_sample_dynamically(argv[1]);
    write_svg(s, "output.svg");

    printf("All done, bye-bye.\n");
    return 0;
}
#endif
