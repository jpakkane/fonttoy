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
#if defined(WASM)
#include<emscripten.h>
#endif

static bool debug_svgs = false;

static_assert(sizeof(lbfgsfloatval_t) == sizeof(double));

typedef double (*fptr)(const double *, const int);

double maxd(const double d1, const double d2) { return d1 > d2 ? d1 : d2; }

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

void put_indexes_in(Stroke &s, SvgExporter &svg) {
    char buf[1024];
    auto &points = s.get_points();
    for(int i=0; i<(int)points.size(); i+=3) {
        const auto &p = points[i];
        const double label_x = p.x() - 0.006;
        const double label_y = p.y() + 0.02;
        sprintf(buf, "%d", i);
        svg.draw_text(label_x, label_y, 0.02, buf);
    }
}

void write_svg(Stroke &s, const char *fname) {
    SvgExporter svg;
    put_beziers_in(s, svg);
    put_indexes_in(s, svg);
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
            auto r = s->add_constraint(std::make_unique<FixedConstraint>(args[0], Point(args[1], args[2])));
            if(r) {
                return *r;
            }
            return 0.0;
        } else if(funname == "DirectionConstraint") {
            if(!s) {
                return "Stroke not set.";
            }
            if(args.size() != 3) {
                return "Wrong number of arguments.";
            }
            auto r = s->add_constraint(std::make_unique<DirectionConstraint>(args[0], args[1], args[2]));
            if(r) {
                return *r;
            }
            return 0.0;
        } else if(funname == "MirrorConstraint") {
            if(!s) {
                return "Stroke not set.";
            }
            if(args.size() != 3) {
                return "Wrong number of arguments.";
            }
            auto r = s->add_constraint(std::make_unique<MirrorConstraint>(args[0], args[1], args[2]));
            if(r) {
                return *r;
            }
            return 0.0;
        } else if(funname == "SmoothConstraint") {
            if(!s) {
                return "Stroke not set.";
            }
            if(args.size() != 3) {
                return "Wrong number of arguments.";
            }
            auto r = s->add_constraint(std::make_unique<SmoothConstraint>(args[0], args[1], args[2]));
            if(r) {
                return *r;
            }
            return 0.0;
        } else if(funname == "AngleConstraint") {
            if(!s) {
                return "Stroke not set.";
            }
            if(args.size() != 4) {
                return "Wrong number of arguments.";
            }
            auto r = s->add_constraint(
                std::make_unique<AngleConstraint>(args[0], args[1], args[2], args[3]));
            if(r) {
                return *r;
            }
            return 0.0;
        } else if(funname == "SameOffsetConstraint") {
            if(!s) {
                return "Stroke not set.";
            }
            if(args.size() != 4) {
                return "Wrong number of arguments.";
            }
            auto r = s->add_constraint(
                std::make_unique<SameOffsetConstraint>(args[0], args[1], args[2], args[3]));
            if(r) {
                return *r;
            }
            return 0.0;
        } else {
            return "Unknown function.";
        }
        return 0;
    }

    bool has_stroke() {
        return (bool)s;
    }

    Stroke &get_stroke() {
        assert(s);
        return *s.get();
    }

private:
    std::unique_ptr<Stroke> s;
};

std::variant<Stroke, std::string> calculate_sample_dynamically(const std::string &program) {
    Bridge b;
    Lexer l(program);
    Parser p(l);
    Interpreter i(p, &b);

    if(!p.parse()) {
        std::string err("Parser fail: ");
        err += p.get_error();
        return err;
    }
    if(!i.execute_program()) {
        std::string err("Interpreter fail: ");
        err += i.get_error();
        return err;
    }
    if(!b.has_stroke()) {
        return "Program did not define a bezier stroke.";
    }
    optimize(&b.get_stroke());
    return std::move(b.get_stroke());
}

#if defined(WASM)

extern "C" {

int EMSCRIPTEN_KEEPALIVE wasm_entrypoint(char *buf) {
    SvgExporter e;
    std::string program(buf);
    auto s = calculate_sample_dynamically(program);
    if(std::holds_alternative<std::string>(s)) {
        strcpy(buf, std::get<std::string>(s).c_str());
        return 1;
    }
    SvgExporter svg;
    put_beziers_in(std::get<Stroke>(s), svg);
    std::string result = svg.to_string();
    strcpy(buf, result.c_str());
    return 0;
}

}

int main(int, char **) {
    printf("Fonttoy Wasm initialized.\n");
    return 0;
}

#else

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

int main(int argc, char **argv) {
    debug_svgs = true;
    SvgExporter e;
    if(argc != 2) {
        printf("%s <input file>\n", argv[0]);
        return 1;
    }
    std::string program = read_file(argv[1]);
    auto s = calculate_sample_dynamically(program);
    if(std::holds_alternative<std::string>(s)) {
        printf("%s\n", std::get<std::string>(s).c_str());
    }
    write_svg(std::get<Stroke>(s), "output.svg");

    printf("All done, bye-bye.\n");
    return 0;
}
#endif
