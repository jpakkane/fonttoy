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

#include <fonttoy.hpp>
#include <vector>
#include <optional>

struct VariableLimits {
    std::optional<double> min_value;
    std::optional<double> max_value;
};

class Constraint {
public:
    virtual ~Constraint() = default;

    virtual double calculate_error(const std::vector<Point> &points) const = 0;
    virtual int num_free_variables() const = 0;
    virtual void put_free_variables_in(std::vector<double> &points, const int offset) const = 0;
    virtual int get_free_variables_from(const std::vector<double> &points, const int offset) = 0;
    virtual void update_model(std::vector<Point> &points) = 0;
    virtual std::vector<int> determines_points() const = 0;
    virtual std::vector<VariableLimits> get_limits() const = 0;
};
