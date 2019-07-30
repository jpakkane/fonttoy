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

#include "parser.hpp"

int main(int, char **) {
    std::string input("y=2\nx = 3*cos(0-y*pi)/1\nhello()\n");
    // std::string input("x = (1 + 2)*3");
    Lexer tokenizer(input);
    Parser p(tokenizer);
    if(!p.parse()) {
        printf("Parser error: %s\n", p.get_error().c_str());
        return 1;
    }
    FuncallPrinter fp;
    Interpreter i(p, &fp);
    if(!i.execute_program()) {
        printf("Interpreter error: %s\n", i.get_error().c_str());
        return 1;
    }
    printf("Value of %s is %f\n", "x", *i.get_variable("x"));
    return 0;
}
