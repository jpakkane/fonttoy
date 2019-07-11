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

    Vector operator-(const Point &other) const;

    const double &x() const { return x_; }
    const double &y() const { return y_; }

private:
    double x_ = 0.0;
    double y_ = 0.0;
};

class Vector final {
public:
    Vector(double x, double y) : x_(x), y_(y) {}

    double length() const;

    const double &x() const { return x_; }
    const double &y() const { return y_; }

private:
    double x_ = 0.0;
    double y_ = 0.0;
};

class Bezier final {
public:
    Bezier(Point p1, Point c1, Point c2, Point p2) : p1(p1), c1(c1), c2(c2), p2(p2) {}

    Point evaluate(const double t) const;
    Point evaluate_d1(const double t) const;
    Point evaluate_d2(const double t) const;

private:
    Point p1, c1, c2, p2;
};
