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

#include <svgexporter.hpp>
#include <tinyxml2.h>
#include <cstdio>

using namespace tinyxml2;

SvgExporter::SvgExporter() {
    setup_canvas();
    // draw_example();
}

void SvgExporter::setup_canvas() {
    root = doc.NewElement("svg");
    root->SetAttribute("xmlns", "http://www.w3.org/2000/svg");
    root->SetAttribute("width", "600px");
    root->SetAttribute("height", "600px");
    doc.InsertFirstChild(root);
    auto bg = doc.NewElement("rect");
    bg->SetAttribute("width", "600px");
    bg->SetAttribute("height", "700px");
    bg->SetAttribute("fill", "white");
    root->InsertFirstChild(bg);
    canvas = root;

    draw_line(-20, 0, 20, 0, "black", 0.002, nullptr);
    draw_line(0, -20, 0, 20, "black", 0.002, nullptr);
    draw_line(-20, 1, 20, 1, "black", 0.002, nullptr);
    draw_line(1, -20, 1, 20, "black", 0.002, nullptr);
    draw_line(0.7, -20, 0.7, 20, "black", 0.001, nullptr);
    draw_text(-0.06, -0.02, 0.02, "(0, 0)");
    draw_text(1.01, 1.01, 0.02, "(1, 1)");
    draw_text(0.71, 1.01, 0.02, "(w, 1)");

    draw_horizontal_guide(0.6, "X-height");
    draw_horizontal_guide(0.62, "X-overshoot");
    draw_horizontal_guide(0.92, "Cap overshoot");
    draw_horizontal_guide(-0.02, "Undershoot");
    draw_horizontal_guide(0.9, "Cap height");
    draw_horizontal_guide(-0.22, "Descender height");
    auto c = doc.NewComment("Character splines go here");
    canvas->InsertEndChild(c);
}

void SvgExporter::draw_line(double x1,
                            double y1,
                            double x2,
                            double y2,
                            const char *stroke,
                            double stroke_width,
                            const char *dash) {
    auto l = doc.NewElement("line");
    l->SetAttribute("x1", x_to_canvas_x(x1));
    l->SetAttribute("y1", y_to_canvas_y(y1));
    l->SetAttribute("x2", x_to_canvas_x(x2));
    l->SetAttribute("y2", y_to_canvas_y(y2));
    if(stroke) {
        l->SetAttribute("stroke", stroke);
        l->SetAttribute("stroke-width", scale * stroke_width);
    }
    if(dash) {
        l->SetAttribute("stroke-dasharray", dash);
    }
    canvas->InsertEndChild(l);
}

void SvgExporter::draw_example() {
    Point p1(0, 0);
    Point c1(0.2, 0.8);
    Point c2(0.9, -0.2);
    Point p2(1, 0);
    draw_bezier(p1, c1, c2, p2, true);
}

void SvgExporter::draw_bezier(
    const Point &p1, const Point &c1, const Point &c2, const Point &p2, bool draw_controls) {
    const int buf_size = 1024;
    char buf[buf_size];
    double stroke_width = 0.002;
    const char *stroke = "black";
    const char *fill = "none";
    snprintf(buf,
             buf_size,
             "M%f %f C %f %f %f %f %f %f",
             x_to_canvas_x(p1.x()),
             y_to_canvas_y(p1.y()),
             x_to_canvas_x(c1.x()),
             y_to_canvas_y(c1.y()),
             x_to_canvas_x(c2.x()),
             y_to_canvas_y(c2.y()),
             x_to_canvas_x(p2.x()),
             y_to_canvas_y(p2.y()));
    auto b = doc.NewElement("path");
    b->SetAttribute("d", buf);
    b->SetAttribute("stroke-width", stroke_width * scale);
    b->SetAttribute("stroke", stroke);
    b->SetAttribute("fill", fill);
    canvas->InsertEndChild(b);

    if(draw_controls) {
        draw_line(p1.x(), p1.y(), c1.x(), c1.y(), "black", 0.001, "1.0,1.0");
        draw_line(p2.x(), p2.y(), c2.x(), c2.y(), "black", 0.001, "1.0,1.0");
        draw_circle(p1.x(), p1.y(), 0.01);
        draw_circle(p2.x(), p2.y(), 0.01);
        draw_cross(c1.x(), c1.y());
        draw_cross(c2.x(), c2.y());
    }
}

void SvgExporter::draw_cross(double x, double y) {
    const int buf_size = 1024;
    char buf[buf_size];
    const double cross_size = 0.01;
    snprintf(buf,
             buf_size,
             "M %f %f L %f %f M %f %f L %f %f",
             x_to_canvas_x(x - cross_size),
             y_to_canvas_y(y - cross_size),
             x_to_canvas_x(x + cross_size),
             y_to_canvas_y(y + cross_size),
             x_to_canvas_x(x - cross_size),
             y_to_canvas_y(y + cross_size),
             x_to_canvas_x(x + cross_size),
             y_to_canvas_y(y - cross_size));
    auto c = doc.NewElement("path");
    c->SetAttribute("d", buf);
    c->SetAttribute("stroke-width", 0.002 * scale);
    c->SetAttribute("stroke", "black");
    canvas->InsertEndChild(c);
}

void SvgExporter::draw_circle(double x, double y, double radius) {
    auto c = doc.NewElement("circle");
    c->SetAttribute("cx", x_to_canvas_x(x));
    c->SetAttribute("cy", y_to_canvas_y(y));
    c->SetAttribute("r", radius * scale);
    canvas->InsertEndChild(c);
}

void SvgExporter::draw_text(double x, double y, double size, const char *msg) {
    auto text = doc.NewElement("text");
    text->SetAttribute("x", x_to_canvas_x(x));
    text->SetAttribute("y", y_to_canvas_y(y));
    text->SetAttribute("font-size", size * scale);
    text->SetAttribute("fill", "black");
    text->SetText(msg);
    canvas->InsertEndChild(text);
}

void SvgExporter::draw_shape(const std::vector<Bezier> &left_beziers, const std::vector<Bezier> &right_beziers) {
    std::string cmds;
    cmds.reserve(2048);
    const int bufsize = 1024;
    char buf[bufsize];
    snprintf(buf, bufsize, "M %f %f ", x_to_canvas_x(left_beziers[0].p1().x()), y_to_canvas_y(left_beziers[0].p1().y()));
    cmds += buf;
    for(const auto b: left_beziers) {
        snprintf(buf, bufsize, "C %f %f %f %f %f %f ",
                x_to_canvas_x(b.c1().x()), y_to_canvas_y(b.c1().y()),
                x_to_canvas_x(b.c2().x()), y_to_canvas_y(b.c2().y()),
                x_to_canvas_x(b.p2().x()), y_to_canvas_y(b.p2().y()));
        cmds += buf;
    }

    snprintf(buf, bufsize, "L %f %f ", x_to_canvas_x(right_beziers.back().p2().x()), y_to_canvas_y(right_beziers.back().p2().y()));
    cmds += buf;
    for(int i=right_beziers.size()-1; i>=0; --i) {
        auto &b = right_beziers[i];
        snprintf(buf, bufsize, "C %f %f %f %f %f %f ",
                x_to_canvas_x(b.c2().x()), y_to_canvas_y(b.c2().y()),
                x_to_canvas_x(b.c1().x()), y_to_canvas_y(b.c1().y()),
                x_to_canvas_x(b.p1().x()), y_to_canvas_y(b.p1().y()));
        cmds += buf;
    }
    cmds += " Z";

    auto shape = doc.NewElement("path");
    shape->SetAttribute("d", cmds.c_str());
    shape->SetAttribute("fill", "gray");
    shape->SetAttribute("stroke", "none");
    canvas->InsertEndChild(shape);
}


void SvgExporter::draw_horizontal_guide(double y, const char *txt) {
    draw_line(-20, y, 20, y, "black", 0.002, "1.0,1.0");
    draw_text(0.82, y + 0.002, 0.02, txt);
}

void SvgExporter::write_svg(const char *ofname) { doc.SaveFile(ofname); }

std::string SvgExporter::to_string() const {
    XMLPrinter printer;
    doc.Print(&printer);
    return std::string(printer.CStr());
}

double SvgExporter::x_to_canvas_x(double x) const { return scale * x + 100; }

double SvgExporter::y_to_canvas_y(double y) const { return scale * (-y) + 450; }
