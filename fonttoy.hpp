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

struct Vector;

// Points and vectors are immutable.

class Point final {
public:
    Point(double x, double y) : x_(x), y_(y) {}
    Point(const Point &p) : x_(p.x_), y_(p.y_) {}
    Point(Point &&p) : x_(p.x_), y_(p.y_) {}

    Point &operator=(const Point &o) = delete;
    Point &operator=(Point &&o) = delete;

    Point &operator=(Point &o) = delete;

    Vector operator-(const Point &other) const;
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

    Vector &operator=(const Vector &o) = delete;
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

    bool is_numerically_zero() const;

private:
    double x_ = 0.0;
    double y_ = 0.0;
};

Vector operator*(const double d, const Vector &v);

class Bezier final {
public:
    Bezier(Point p1, Point c1, Point c2, Point p2) : p1(p1), c1(c1), c2(c2), p2(p2) {}

    Point evaluate(const double t) const;
    Point evaluate_d1(const double t) const;
    Point evaluate_d2(const double t) const;

private:
    Point p1, c1, c2, p2;
};
