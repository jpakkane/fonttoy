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

using namespace tinyxml2;

void write_svg(const char *ofname) {
    XMLDocument doc;
    auto root = doc.NewElement("svg");
    root->SetAttribute("xmlns","http://www.w3.org/2000/svg");
    root->SetAttribute("width", "1200px");
    root->SetAttribute("height", "1400px");
    doc.InsertFirstChild(root);
    doc.SaveFile(ofname);
}
