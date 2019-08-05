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
#include <emscripten.h>
#endif

static bool debug_svgs = false;

static_assert(sizeof(lbfgsfloatval_t) == sizeof(double));

typedef double (*fptr)(const double *, const int);

double maxd(const double d1, const double d2) { return d1 > d2 ? d1 : d2; }

enum class WhichStroke : char { skeleton, left, right };

double distance_error(Shape *s, const std::vector<double> &x, WhichStroke which) {
    assert(which == WhichStroke::left || which == WhichStroke::right);
    double total_error = 0;
    auto skel_beziers = s->skeleton.build_beziers();
    Stroke *side = which == WhichStroke::left ? &s->left : &s->right;
    side->set_free_variables(x);
    auto side_beziers = side->build_beziers();
    assert(skel_beziers.size() == side_beziers.size());
    for(int bez_index = 0; bez_index < (int)skel_beziers.size(); ++bez_index) {
        for(int i = 1; i < 4; ++i) {
            double t = i / 4.0;
            double target_distance = 0.05; // FIXME, calculate from pen shape.
            auto skel_point = skel_beziers[bez_index].evaluate(t);
            auto side_point = side_beziers[bez_index].evaluate(t);
            auto offset = skel_point - side_point;
            auto distance = offset.length();
            auto diff = distance - target_distance;
            total_error += diff * diff;
        }
    }
    return total_error;
}

struct OptimizerArguments {
    Shape *s;
    WhichStroke which;

    double calculate_value_for(const std::vector<double> &x) const {
        switch(which) {
        case WhichStroke::skeleton:
            return s->skeleton.calculate_value_for(x);
        case WhichStroke::right:
        case WhichStroke::left:
            return distance_error(s, x, which);
        default:
            assert(false);
        }
        return 0.0 / 0.0;
    }
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

std::vector<double> estimate_derivative(OptimizerArguments *args,
                                        const std::vector<double> &x,
                                        double f0,
                                        const std::vector<double> &h) {
    std::vector<double> g(x.size());
    std::vector<double> x0 = x;
    for(size_t i = 0; i < x.size(); i++) {
        double old_v = x0[i];
        x0[i] += h[i];
        double dx = h[i];
        double df = args->calculate_value_for(x0) - f0;
        g[i] = df / dx;
        x0[i] = old_v;
    }
    return g;
}

void put_beziers_in(Stroke &s, SvgExporter &svg, bool draw_controls) {
    for(const auto b : s.build_beziers()) {
        svg.draw_bezier(b.p1(), b.c1(), b.c2(), b.p2(), draw_controls);
    }
}

void put_indexes_in(Stroke &s, SvgExporter &svg) {
    char buf[1024];
    auto &points = s.get_points();
    for(int i = 0; i < (int)points.size(); i += 3) {
        const auto &p = points[i];
        const double label_x = p.x() - 0.006;
        const double label_y = p.y() + 0.02;
        sprintf(buf, "%d", i);
        svg.draw_text(label_x, label_y, 0.02, buf);
    }
}

void build_svg(Shape &s, SvgExporter &svg) {
    put_beziers_in(s.skeleton, svg, true);
    put_indexes_in(s.skeleton, svg);
    put_beziers_in(s.left, svg, false);
    put_beziers_in(s.right, svg, false);
}

void write_svg(Shape &s, const char *fname) {
    SvgExporter svg;
    build_svg(s, svg);
    svg.write_svg(fname);
}

static lbfgsfloatval_t evaluate_model(void *instance,
                                      const lbfgsfloatval_t *x,
                                      lbfgsfloatval_t *g,
                                      const int n,
                                      const lbfgsfloatval_t step) {
    static int num = 0;
    auto args = reinterpret_cast<OptimizerArguments *>(instance);
    (void)step;
    const double rel_step = 0.000000001;
    std::vector<double> curx(x, x + n);
    double fx = args->calculate_value_for(curx);
    if(debug_svgs) {
        char buf[256];
        sprintf(buf, "eval%03d.svg", num++);
        write_svg(*args->s, buf);
    }
    auto curh = compute_absolute_step(rel_step, curx);
    auto g_est = estimate_derivative(args, curx, fx, curh);
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
    auto *args = reinterpret_cast<OptimizerArguments *>(instance);
    if(debug_svgs) {
        char buf[128];
        sprintf(buf, "step%d.svg", k);
        write_svg(*args->s, buf);
    }
    return 0;
}

void optimize_skeleton(Shape *shape) {
    OptimizerArguments args;
    args.which = WhichStroke::skeleton;
    args.s = shape;
    Stroke *s = &shape->skeleton;
    double final_result = 1e8;
    auto variables = s->get_free_variables();
    assert(variables.size() == 7);
    s->freeze();
    variables = s->get_free_variables();
    assert(variables.size() == 9);

    if(debug_svgs) {
        s->calculate_value_for(variables);
        write_svg(*shape, "initial.svg");
    }

    lbfgs_parameter_t param;
    lbfgs_parameter_init(&param);
    int ret = lbfgs(variables.size(),
                    &variables[0],
                    &final_result,
                    evaluate_model,
                    model_progress,
                    &args,
                    &param);
    printf("Skeleton exit value: %d\n", ret);
    // insert final values back in the stroke here.
    s->calculate_value_for(variables);
}

void optimize_side(Shape *shape, const WhichStroke which) {
    OptimizerArguments args;
    Stroke *skel = &shape->skeleton;
    args.which = which;
    double final_result = 1e8;
    const double r = 0.05;
    args.s = shape;
    auto skel_b = skel->build_beziers();
    auto side = which == WhichStroke::left ? &shape->left : &shape->right;
    const auto &skel_points = skel->get_points();
    const auto &side_points = side->get_points();
    assert(skel_points.size() == side_points.size());

    // Each side point is at a fixed location w.r.t. to the skeleton point.
    for(int i = 0; i < (int)skel_points.size(); i += 3) {
        int bezier_index, eval_point;
        if(i == (int)skel_points.size() - 1) {
            bezier_index = (int)skel_b.size() - 1;
            eval_point = 1.0;
        } else {
            bezier_index = i / 3;
            eval_point = 0.0;
        }
        int flipper = which == WhichStroke::left ? 1 : -1;
        Point skel_point = skel_b[bezier_index].evaluate(eval_point);
        Vector side_normal = skel_b[bezier_index].evaluate_left_normal(eval_point);
        Point side_point = skel_point + flipper * r * side_normal;
        auto rc = side->add_constraint(std::make_unique<FixedConstraint>(i, side_point));
        assert(!rc);
    }

    // Each control point _after_ a fixed point defines the direction.
    // Each control point _before_ a fixed point defines smoothness.
    // The very last point is special in each case.
    for(int i = 0; i < (int)skel_b.size(); ++i) {
        auto direction = skel_b[i].evaluate_d1(0.0);
        auto theta = direction.angle();
        auto rc =
            side->add_constraint(std::make_unique<DirectionConstraint>(i * 3, i * 3 + 1, theta));
        assert(!rc);
    }
    auto backwards_angle = skel_b.back().evaluate_d1(1.0).angle() + M_PI;
    auto rc = side->add_constraint(std::make_unique<DirectionConstraint>(
        skel_points.size() - 1, skel_points.size() - 2, backwards_angle));
    assert(!rc);

    for(int i = 1; i < (int)skel_b.size(); ++i) {
        int middle_curve_point = 3 * i;
        int this_control_index = 3 * i - 1;
        int other_control_index = 3 * i + 1;
        auto rc = side->add_constraint(std::make_unique<SmoothConstraint>(
            this_control_index, other_control_index, middle_curve_point));
        assert(!rc);
    }

    side->freeze();
    auto variables = side->get_free_variables();

    lbfgs_parameter_t param;
    lbfgs_parameter_init(&param);
    int ret = lbfgs(variables.size(),
                    &variables[0],
                    &final_result,
                    evaluate_model,
                    model_progress,
                    &args,
                    &param);
    side->calculate_value_for(variables);
    printf("Side exit value: %d\n", ret);
}

void optimize(Shape *shape) {
    optimize_skeleton(shape);
    optimize_side(shape, WhichStroke::left);
    optimize_side(shape, WhichStroke::right);
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
            s.reset(new Shape(args[0]));
            return 0.0;
        } else if(funname == "FixedConstraint") {
            if(!s) {
                return "Stroke not set.";
            }
            if(args.size() != 3) {
                return "Wrong number of arguments.";
            }
            auto r = s->skeleton.add_constraint(
                std::make_unique<FixedConstraint>(args[0], Point(args[1], args[2])));
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
            auto r = s->skeleton.add_constraint(
                std::make_unique<DirectionConstraint>(args[0], args[1], args[2]));
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
            auto r = s->skeleton.add_constraint(
                std::make_unique<MirrorConstraint>(args[0], args[1], args[2]));
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
            auto r = s->skeleton.add_constraint(
                std::make_unique<SmoothConstraint>(args[0], args[1], args[2]));
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
            auto r = s->skeleton.add_constraint(
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
            auto r = s->skeleton.add_constraint(
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

    bool has_shape() { return (bool)s; }

    Shape &get_shape() {
        assert(s);
        return *s.get();
    }

private:
    std::unique_ptr<Shape> s;
};

std::variant<Shape, std::string> calculate_sample_dynamically(const std::string &program) {
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
    if(!b.has_shape()) {
        return "Program did not define a bezier stroke.";
    }
    optimize(&b.get_shape());
    return std::move(b.get_shape());
}

#if defined(WASM)

extern "C" {

int EMSCRIPTEN_KEEPALIVE wasm_entrypoint(char *buf) {
    std::string program(buf);
    auto s = calculate_sample_dynamically(program);
    if(std::holds_alternative<std::string>(s)) {
        strcpy(buf, std::get<std::string>(s).c_str());
        return 1;
    }
    SvgExporter svg;
    build_svg(std::get<Stroke>(s), svg);
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
    } else {
        write_svg(std::get<Shape>(s), "output.svg");
    }

    printf("All done, bye-bye.\n");
    return 0;
}
#endif
