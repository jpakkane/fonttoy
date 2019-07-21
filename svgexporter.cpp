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

SvgExporter::SvgExporter() { setup_canvas(); }

void SvgExporter::setup_canvas() {
    root = doc.NewElement("svg");
    root->SetAttribute("xmlns", "http://www.w3.org/2000/svg");
    root->SetAttribute("width", "1200px");
    root->SetAttribute("height", "1400px");
    doc.InsertFirstChild(root);
    auto bg = doc.NewElement("rect");
    bg->SetAttribute("width", "1200px");
    bg->SetAttribute("height", "1400px");
    bg->SetAttribute("fill", "white");
    root->InsertFirstChild(bg);
    auto t = doc.NewElement("g");
    t->SetAttribute("transform", "translate(100, 1100)");
    root->InsertEndChild(t);
    auto t2 = doc.NewElement("g");
    t2->SetAttribute("transform", "matrix(1, 0, 0, -1, 0, 0)");
    t->InsertEndChild(t2);
    canvas = doc.NewElement("g");
    canvas->SetAttribute("transform", "scale(1000)");
    t2->InsertEndChild(canvas);

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
    l->SetAttribute("x1", x1);
    l->SetAttribute("y1", y1);
    l->SetAttribute("x2", x2);
    l->SetAttribute("y2", y2);
    if(stroke) {
        l->SetAttribute("stroke", stroke);
        l->SetAttribute("stroke-width", stroke_width);
    }
    if(dash) {
        l->SetAttribute("stroke-dasharray", dash);
    }
    canvas->InsertEndChild(l);
}

void SvgExporter::draw_text(double x, double y, double size, const char *msg) {
    auto g1 = doc.NewElement("g");
    const int buf_size = 1024;
    char buf[buf_size];

    snprintf(buf, buf_size, "translate(%f, %f)", x, y);

    g1->SetAttribute("transform", buf);
    canvas->InsertEndChild(g1);
    auto g2 = doc.NewElement("g");
    g2->SetAttribute("transform", "matrix(1, 0, 0, -1, 0, 0)");
    g1->InsertEndChild(g2);
    auto text = doc.NewElement("text");
    text->SetAttribute("x", 0);
    text->SetAttribute("y", 0);
    text->SetAttribute("font-size", size);
    text->SetAttribute("fill", "black");
    text->SetText(msg);
    g2->InsertEndChild(text);
}

void SvgExporter::draw_horizontal_guide(double y, const char *txt) {
    draw_line(-20, y, 20, y, "black", 0.002, "0.01,0.01");
    draw_text(0.82, y + 0.002, 0.02, txt);
}

void SvgExporter::write_svg(const char *ofname) { doc.SaveFile(ofname); }
