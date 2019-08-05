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

#include <tinyxml2.h>
#include <string>
#include <fonttoy.hpp>

class SvgExporter final {
public:
    SvgExporter();
    void write_svg(const char *ofname);
    std::string to_string() const;

    void draw_line(double x1,
                   double y1,
                   double x2,
                   double y2,
                   const char *stroke,
                   double stroke_width,
                   const char *dash);

    void draw_text(double x, double y, double size, const char *msg);

    void draw_horizontal_guide(double y, const char *txt);

    void draw_bezier(const Point &p1,
                     const Point &c1,
                     const Point &c2,
                     const Point &p2,
                     bool draw_controls = false);

    void draw_circle(double x, double y, double radius);

    void draw_cross(double x, double y);

    void draw_shape(const std::vector<Bezier> &left_beziers, const std::vector<Bezier> &right_beziers);

private:
    void setup_canvas();
    void draw_example();

    double x_to_canvas_x(double x) const;
    double y_to_canvas_y(double y) const;

    const double scale = 400.0;
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLElement *root;
    tinyxml2::XMLElement *canvas;
};
