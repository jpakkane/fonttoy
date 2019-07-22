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
    virtual void append_free_variables_to(std::vector<double> &variables) const = 0;
    virtual int put_free_variables_in(std::vector<double> &variables, const int offset) const = 0;
    virtual int get_free_variables_from(const std::vector<double> &variables, const int offset) = 0;
    virtual void update_model(std::vector<Point> &points) const = 0;
    virtual std::vector<int> determines_points() const = 0;
    virtual std::vector<VariableLimits> get_limits() const = 0;
};

class FixedConstraint final : public Constraint {

public:
    FixedConstraint(int point_index, Point p) : point_index(point_index), p(p) {}

    double calculate_error(const std::vector<Point> &points) const override;
    int num_free_variables() const override;
    void append_free_variables_to(std::vector<double> &variables) const override;
    int put_free_variables_in(std::vector<double> &points, const int offset) const override;
    int get_free_variables_from(const std::vector<double> &points, const int offset) override;
    void update_model(std::vector<Point> &points) const override;
    std::vector<int> determines_points() const override;
    std::vector<VariableLimits> get_limits() const override;

private:
    int point_index;
    Point p;
};

class DirectionConstraint final : public Constraint {

public:
    DirectionConstraint(int from_point_index, int to_point_index, double angle);

    double calculate_error(const std::vector<Point> &points) const override;
    int num_free_variables() const override;
    void append_free_variables_to(std::vector<double> &variables) const override;
    int put_free_variables_in(std::vector<double> &points, const int offset) const override;
    int get_free_variables_from(const std::vector<double> &points, const int offset) override;
    void update_model(std::vector<Point> &points) const override;
    std::vector<int> determines_points() const override;
    std::vector<VariableLimits> get_limits() const override;

private:
    int from_point_index;
    int to_point_index;
    double angle;
    double distance;
};

class MirrorConstraint final : public Constraint {

public:
    MirrorConstraint(int point_index, int from_point_index, int mirror_point_index);

    double calculate_error(const std::vector<Point> &points) const override;
    int num_free_variables() const override;
    void append_free_variables_to(std::vector<double> &variables) const override;
    int put_free_variables_in(std::vector<double> &points, const int offset) const override;
    int get_free_variables_from(const std::vector<double> &points, const int offset) override;
    void update_model(std::vector<Point> &points) const override;
    std::vector<int> determines_points() const override;
    std::vector<VariableLimits> get_limits() const override;

private:
    int point_index;
    int from_point_index;
    int mirror_point_index;
};

class SmoothConstraint final : public Constraint {

public:
    SmoothConstraint(int this_control_index, int other_control_index, int curve_point_index);

    double calculate_error(const std::vector<Point> &points) const override;
    int num_free_variables() const override;
    void append_free_variables_to(std::vector<double> &variables) const override;
    int put_free_variables_in(std::vector<double> &points, const int offset) const override;
    int get_free_variables_from(const std::vector<double> &points, const int offset) override;
    void update_model(std::vector<Point> &points) const override;
    std::vector<int> determines_points() const override;
    std::vector<VariableLimits> get_limits() const override;

private:
    int this_control_index, other_control_index, curve_point_index;
    double alpha;
};


class AngleConstraint final : public Constraint {

public:
    AngleConstraint(int point_index, int from_point_index, double min_angle, double max_angle);

    double calculate_error(const std::vector<Point> &points) const override;
    int num_free_variables() const override;
    void append_free_variables_to(std::vector<double> &variables) const override;
    int put_free_variables_in(std::vector<double> &points, const int offset) const override;
    int get_free_variables_from(const std::vector<double> &points, const int offset) override;
    void update_model(std::vector<Point> &points) const override;
    std::vector<int> determines_points() const override;
    std::vector<VariableLimits> get_limits() const override;

private:
    int point_index;
    int from_point_index;
    double min_angle;
    double max_angle;
    double angle;
    double distance;
};
