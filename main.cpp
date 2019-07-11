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

#include <cstdio>
#include <fonttoy.hpp>
#include <vector>

int main(int, char **) {
    std::vector<Vector> v;
    v.emplace_back(1.0, 2.0);
    v.push_back({1.1, 2.2});
    printf("Val: %f.\n", v.back().x());
    v.pop_back();
    printf("Val: %f.\n", v.back().x());
    return 0;
}
