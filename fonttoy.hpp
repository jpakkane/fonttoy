#pragma once

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

#include <vector>
#include <memory>
#include <optional>

class Vector;
class Constraint;

struct VariableLimits {
    std::optional<double> min_value;
    std::optional<double> max_value;
};

struct WhichCoordinate {
    bool x = false;
    bool y = false;

    WhichCoordinate(bool x, bool y) : x(x), y(y) {}

    bool defines_same(const WhichCoordinate &o) const {
        return (x&&o.x) || (y&&o.y);
    }

    bool try_union(const WhichCoordinate &o) {
        if(defines_same(o)) {
            return false;
        }
        x |= o.x;
        y |= o.y;
        return true;
    }

    bool fully_constrained() const {
        return x && y;
    }
};

struct CoordinateDefinition {
    int index;
    WhichCoordinate w;

    CoordinateDefinition(int index, bool defines_x, bool defines_y) : index(index), w(defines_x, defines_y) {
    }
};

// Points and vectors are immutable but assignable

class Point final {
public:
    Point() : x_(0.0), y_(0.0) {}
    Point(double x, double y) : x_(x), y_(y) {}
    Point(const Point &p) : x_(p.x_), y_(p.y_) {}
    Point(Point &&p) : x_(p.x_), y_(p.y_) {}

    const Point &operator=(const Point &o) {
        x_ = o.x_;
        y_ = o.y_;
        return *this;
    }

    const Point &operator=(Point &&o) {
        x_ = o.x_;
        y_ = o.y_;
        return *this;
    }

    Vector operator-(const Point &other) const;
    Point operator-(const Vector &v) const;
    Point operator+(const Vector &v) const;

    const double &x() const { return x_; }
    const double &y() const { return y_; }

private:
    double x_ = 0.0;
    double y_ = 0.0;
};

class Vector final {
public:
    Vector(double x, double y) : x_(x), y_(y) {}
    explicit Vector(const Point &p) : x_(p.x()), y_(p.y()) {}
    Vector(const Vector &o) : x_(o.x_), y_(o.y_) {}
    Vector(Vector &&o) : x_(o.x_), y_(o.y_) {}

    const Vector &operator=(const Vector &o) {
        x_ = o.x_;
        y_ = o.y_;
        return *this;
    }

    Vector &operator=(Vector &&o) = delete;

    double length() const;

    const double &x() const { return x_; }
    const double &y() const { return y_; }

    double distance(const Point &p) const;
    double angle() const;
    double dot(const Vector &other) const;
    Vector operator*(const double r) const;
    Vector normalized() const;
    Vector projected_to(const Vector &target) const;

    Point operator+(const Point &o) const { return o + *this; }
    Point operator+(Point &&o) const { return o + *this; }

    Vector operator-(const Vector &o) const { return Vector{x_ - o.x_, y_ - o.y_}; }

    bool is_numerically_zero() const;

private:
    double x_ = 0.0;
    double y_ = 0.0;
};

Vector operator*(const double d, const Vector &v);
Vector operator*(const double d, Vector &&v);

class Bezier final {
public:
    Bezier(Point p1, Point c1, Point c2, Point p2) : p1_(p1), c1_(c1), c2_(c2), p2_(p2) {}

    Point evaluate(const double t) const;
    Vector evaluate_d1(const double t) const;
    Vector evaluate_d2(const double t) const;
    Vector evaluate_left_normal(const double t) const;

    const Point &p1() const { return p1_; }
    const Point &c1() const { return c1_; }
    const Point &c2() const { return c2_; }
    const Point &p2() const { return p2_; }

private:
    Point p1_, c1_, c2_, p2_;
};

class Stroke final {
public:
    explicit Stroke(const int num_beziers);

    std::vector<double> get_free_variables() const;
    void set_free_variables(const std::vector<double> &v);
    std::optional<std::string> add_constraint(std::unique_ptr<Constraint> c);

    double calculate_value_for(const std::vector<double> &vars);
    std::vector<Bezier> build_beziers() const;

    void freeze();

    const std::vector<Point>& get_points() const { return points; }

private:
    void update_model();
    double calculate_2nd_der() const;
    double calculate_limit_errors(const std::vector<double> &vars) const;

    int num_beziers;
    std::vector<Point> points;
    std::vector<WhichCoordinate> coord_specifications;
    std::vector<std::unique_ptr<Constraint>> constraints;
    std::vector<VariableLimits> limits;
    bool is_frozen = false; // No more constraints.
};
