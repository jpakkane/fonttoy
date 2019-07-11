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
#include <cmath>

Vector Point::operator-(const Point &other) const { return Vector(x_ - other.x_, y_ - other.y_); }

double Vector::length() const { return sqrt(x_ * x_ + y_ * y_); }

Point Bezier::evaluate(const double t) const {
    double x = pow(1.0 - t, 3.0) * p1.x() + 3.0 * pow(1.0 - t, 2.0) * t * c1.x() +
               3.0 * (1.0 - t) * t * t * c2.x() + pow(t, 3.0) * p2.x();
    double y = pow(1.0 - t, 3.0) * p1.y() + 3.0 * pow(1.0 - t, 2.0) * t * c1.y() +
               3.0 * (1.0 - t) * t * t * c2.y() + pow(t, 3) * p2.y();
    return Point(x, y);
}

Point Bezier::evaluate_d1(const double t) const {
    double x = 3.0 * pow(1.0 - t, 2) * (c1.x() - p1.x()) + 6.0 * (1.0 - t) * t * (c2.x() - c1.x()) +
               3.0 * t * t * (p2.x() - c2.x());
    double y = 3.0 * pow(1.0 - t, 2) * (c1.y() - p1.y()) + 6.0 * (1.0 - t) * t * (c2.y() - c1.y()) +
               3.0 * t * t * (p2.y() - c2.y());
    return Point(x, y);
}

Point Bezier::evaluate_d2(const double t) const {
    double x = 6.0 * (1.0 - t) * (c2.x() - 2.0 * c1.x() + p1.x()) +
               6.0 * t * (p2.x() - 2.0 * c2.x() + p1.x());
    double y = 6.0 * (1.0 - t) * (c2.y() - 2.0 * c1.y() + p1.y()) +
               6.0 * t * (p2.y() - 2.0 * c2.y() + p1.y());
    return Point(x, y);
}
