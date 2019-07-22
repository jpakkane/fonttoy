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

#include <fonttoy.hpp>
#include <constraints.hpp>
#include <cmath>
#include <cassert>

Vector Point::operator-(const Point &other) const { return Vector(x_ - other.x_, y_ - other.y_); }

Point Point::operator+(const Vector &v) const { return Point(x_ + v.x(), y_ + v.y()); }

Point Point::operator-(const Vector &v) const { return Point(x_ - v.x(), y_ - v.y()); }

double Vector::length() const { return sqrt(x_ * x_ + y_ * y_); }

double Vector::angle() const { return atan2(x_, y_); }

double Vector::dot(const Vector &other) const { return x_ * other.x_ + y_ * other.y_; }

Vector Vector::operator*(const double r) const { return Vector(r * x_, r * y_); }

double Vector::distance(const Point &p) const {
    const double dx = x_ - p.x();
    const double dy = y_ - p.y();
    return sqrt(dx * dx + dy * dy);
}

bool Vector::is_numerically_zero() const {
    if(fabs(x_) < 0.0001 && fabs(y_) < 0.0001) {
        return true;
    }
    return false;
}

Vector Vector::normalized() const {
    if(is_numerically_zero()) {
        return Vector(0.0, 0.0);
    }
    auto d = length();
    return Vector(x_ / d, y_ / d);
}

Vector Vector::projected_to(const Vector &target) const {
    if(target.is_numerically_zero()) {
        return Vector(0.0, 0.0);
    }
    const double numerator = dot(target);
    const double denominator = target.dot(target);
    return (numerator / denominator) * target;
}

Vector operator*(const double d, const Vector &v) { return v * d; }
Vector operator*(const double d, Vector &&v) { return v * d; }

Point Bezier::evaluate(const double t) const {
    double x = pow(1.0 - t, 3.0) * p1.x() + 3.0 * pow(1.0 - t, 2.0) * t * c1.x() +
               3.0 * (1.0 - t) * t * t * c2.x() + pow(t, 3.0) * p2.x();
    double y = pow(1.0 - t, 3.0) * p1.y() + 3.0 * pow(1.0 - t, 2.0) * t * c1.y() +
               3.0 * (1.0 - t) * t * t * c2.y() + pow(t, 3) * p2.y();
    return Point(x, y);
}

Vector Bezier::evaluate_d1(const double t) const {
    double x = 3.0 * pow(1.0 - t, 2) * (c1.x() - p1.x()) + 6.0 * (1.0 - t) * t * (c2.x() - c1.x()) +
               3.0 * t * t * (p2.x() - c2.x());
    double y = 3.0 * pow(1.0 - t, 2) * (c1.y() - p1.y()) + 6.0 * (1.0 - t) * t * (c2.y() - c1.y()) +
               3.0 * t * t * (p2.y() - c2.y());
    return Vector(x, y);
}

Vector Bezier::evaluate_d2(const double t) const {
    double x = 6.0 * (1.0 - t) * (c2.x() - 2.0 * c1.x() + p1.x()) +
               6.0 * t * (p2.x() - 2.0 * c2.x() + p1.x());
    double y = 6.0 * (1.0 - t) * (c2.y() - 2.0 * c1.y() + p1.y()) +
               6.0 * t * (p2.y() - 2.0 * c2.y() + p1.y());
    return Vector(x, y);
}

Vector Bezier::evaluate_left_normal(const double t) const {
    auto d1 = evaluate_d1(t);
    Vector dn(-d1.y(), d1.x());
    return dn.normalized();
}

Stroke::Stroke(const int num_beziers) : num_beziers(num_beziers) {
    const int num_points = num_beziers * 3 + 1;
    points.reserve(num_points);
    for(int i = 0; i < num_points; ++i) {
        points.push_back(Point());
    }
}

void Stroke::add_constraint(std::unique_ptr<Constraint> c) {
    constraints.push_back(std::move(c));
    for(auto &&l : constraints.back()->get_limits()) {
        limits.emplace_back(l);
    }
}

std::vector<double> Stroke::get_free_variables() const {
    std::vector<double> free_variables;
    for(const auto &c : constraints) {
        c->append_free_variables_to(free_variables);
    }
    return free_variables;
}

void Stroke::set_free_variables(const std::vector<double> &v) {
    int offset = 0;
    for(const auto &c : constraints) {
        offset += c->get_free_variables_from(v, offset);
    }
}

double Stroke::calculate_value_for(const std::vector<double> &vars) {
    set_free_variables(vars);
    update_model();
    return calculate_2nd_der() + calculate_limit_errors(vars);
}

void Stroke::update_model() {
    // FIXME: add topological sorting here.
    for(auto &c : constraints) {
        c->update_model(points);
    }
}

double Stroke::calculate_2nd_der() const {
    auto beziers = build_beziers();
    double i = 0.0;
    double result = 0.0;
    const double delta = 0.01;
    const double cutoff = beziers.size();
    while(i <= cutoff) {
        const int bezier_ind = int(i);
        const double bezier_i = fmod(i, 1.0);
        const auto &cur_b = beziers[bezier_ind];
        const Vector h = cur_b.evaluate_d2(bezier_i);
        const Vector left_n = cur_b.evaluate_left_normal(i);
        double left_n_length = left_n.length(); // Should be one, but zero in the degenerate case.
        if(left_n_length != 0) {
            assert(fabs(left_n_length - 1.0) < 0.0001);
            double projected = h.dot(left_n) / left_n.length();
            result = std::max(fabs(projected), result);
        }
        i += delta;
    }
    return result;
}

double Stroke::calculate_limit_errors(const std::vector<double> &vars) const {
    assert(limits.size() == vars.size());
    double error = 0.0;
    auto err_func = [](const double a, const double b) {
        const double delta = fabs(a - b);
        return 10000.0 * delta * delta;
    };
    for(size_t i = 0; i < limits.size(); i++) {
        const auto &v = vars[i];
        const auto &l = limits[i];
        if(l.min_value && v < *l.min_value) {
            error += err_func(v, *l.min_value);
        }
        if(l.max_value && v > *l.max_value) {
            error += err_func(v, *l.max_value);
        }
    }
    return error;
}

std::vector<Bezier> Stroke::build_beziers() const {
    std::vector<Bezier> b;
    b.reserve(num_beziers);
    size_t i = 3;
    while(i < points.size()) {
        b.emplace_back(points[i - 3], points[i - 2], points[i - 1], points[i]);
        i += 3;
    }
    return b;
}
