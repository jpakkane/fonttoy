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

#include <maths.hpp>
#include <constraints.hpp>

#include <vector>
#include <memory>
#include <optional>

class Stroke final {
public:
    explicit Stroke(const int num_beziers);

    std::vector<double> get_free_variables() const;
    void set_free_variables(const std::vector<double> &v);
    std::optional<std::string> add_constraint(std::unique_ptr<Constraint> c);

    double calculate_value_for(const std::vector<double> &vars);
    std::vector<Bezier> build_beziers() const;
    Bezier build_bezier(int i) const;

    void freeze();

    const std::vector<Point> &get_points() const { return points; }
    Point evaluate(const double t) const;

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

struct Shape {
    Stroke skeleton;
    Stroke left;
    Stroke right;

    Shape(int i) : skeleton(i), left(i), right(i) {}
};
