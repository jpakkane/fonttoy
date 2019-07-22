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

#include <constraints.hpp>
#include <cmath>

double FixedConstraint::calculate_error(const std::vector<Point> &) const { return 0.0; }

int FixedConstraint::num_free_variables() const { return 0; }

void FixedConstraint::append_free_variables_to(std::vector<double> &) const {}

int FixedConstraint::put_free_variables_in(std::vector<double> &, const int) const { return 0; }

int FixedConstraint::get_free_variables_from(const std::vector<double> &, const int) { return 0; }

void FixedConstraint::update_model(std::vector<Point> &points) const { points[point_index] = p; }

std::vector<int> FixedConstraint::determines_points() const {
    std::vector<int> v;
    v.push_back(point_index);
    return v;
}

std::vector<VariableLimits> FixedConstraint::get_limits() const {
    std::vector<VariableLimits> v;
    return v;
}

DirectionConstraint::DirectionConstraint(int from_point_index, int to_point_index, double angle)
    : from_point_index(from_point_index), to_point_index(to_point_index), angle(angle) {
    distance = 0.2;
}

double DirectionConstraint::calculate_error(const std::vector<Point> &) const { return 0.0; }

int DirectionConstraint::num_free_variables() const { return 1; }

void DirectionConstraint::append_free_variables_to(std::vector<double> &variables) const {
    variables.push_back(distance);
}

int DirectionConstraint::put_free_variables_in(std::vector<double> &points,
                                               const int offset) const {
    points[offset] = distance;
    return 1;
}

int DirectionConstraint::get_free_variables_from(const std::vector<double> &points,
                                                 const int offset) {
    distance = points[offset];
    return 1;
}

void DirectionConstraint::update_model(std::vector<Point> &points) const {
    Vector direction_unit_vector(cos(angle), sin(angle));
    points[to_point_index] = points[from_point_index] + distance * direction_unit_vector;
}

std::vector<int> DirectionConstraint::determines_points() const {
    std::vector<int> r;
    r.push_back(to_point_index);
    return r;
}

std::vector<VariableLimits> DirectionConstraint::get_limits() const {
    std::vector<VariableLimits> v;
    VariableLimits l{0.0, {}};
    v.push_back(l);
    return v;
}

MirrorConstraint::MirrorConstraint(int point_index, int from_point_index, int mirror_point_index)
    : point_index(point_index), from_point_index(from_point_index),
      mirror_point_index(mirror_point_index) {}

double MirrorConstraint::calculate_error(const std::vector<Point> &) const { return 0.0; }

int MirrorConstraint::num_free_variables() const { return 0; }

void MirrorConstraint::append_free_variables_to(std::vector<double> &) const {}

int MirrorConstraint::put_free_variables_in(std::vector<double> &, const int) const { return 0; }

int MirrorConstraint::get_free_variables_from(const std::vector<double> &, const int) { return 0; }

void MirrorConstraint::update_model(std::vector<Point> &points) const {
    Vector updated_location(Vector(points[mirror_point_index]) * 2.0 -
                            Vector(points[from_point_index]));
    points[point_index] = Point(updated_location.x(), updated_location.y());
}

std::vector<int> MirrorConstraint::determines_points() const {
    std::vector<int> result;
    result.push_back(point_index);
    return result;
}

std::vector<VariableLimits> MirrorConstraint::get_limits() const {
    std::vector<VariableLimits> l;
    return l;
}

SmoothConstraint::SmoothConstraint(int this_control_index,
                                   int other_control_index,
                                   int curve_point_index)
    : this_control_index(this_control_index), other_control_index(other_control_index),
      curve_point_index(curve_point_index) {
    alpha = 1.0;
}

double SmoothConstraint::calculate_error(const std::vector<Point> &) const { return 0.0; }

int SmoothConstraint::num_free_variables() const { return 1; }

void SmoothConstraint::append_free_variables_to(std::vector<double> &variables) const {
    variables.push_back(alpha);
}

int SmoothConstraint::put_free_variables_in(std::vector<double> &points, const int offset) const {
    points[offset] = alpha;
    return 1;
}

int SmoothConstraint::get_free_variables_from(const std::vector<double> &points, const int offset) {
    alpha = points[offset];
    return 1;
}

void SmoothConstraint::update_model(std::vector<Point> &points) const {
    Vector delta = points[other_control_index] - points[curve_point_index];
    points[this_control_index] = points[curve_point_index] - delta * alpha;
}

std::vector<int> SmoothConstraint::determines_points() const {
    std::vector<int> p;
    p.push_back(this_control_index);
    return p;
}

std::vector<VariableLimits> SmoothConstraint::get_limits() const {
    VariableLimits l{0.01, {}};
    std::vector<VariableLimits> result;
    result.push_back(l);
    return result;
}
