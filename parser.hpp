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

#pragma once

#include <string>
#include <vector>
#include <variant>
#include <optional>
#include <unordered_map>
#include <cstdio>
#include <cassert>
#include <cmath>

// These are in matching priority order.

enum class TokenType : char {
    id,
    number,
    plus,
    minus,
    multiply,
    divide,
    equal,
    dot,
    semicolon,
    whitespace,
    linefeed,
    lparen,
    rparen,
    comma,
    end_of_tokens,
    error,
    eof,
};

struct Token final {
    TokenType type;
    std::string contents;
    int byte_offset;
    int line_number;
    int column_number;
};

class Lexer final {
public:
    explicit Lexer(const std::string &s) : text(s) {
        if(text.empty() || text.back() != '\n') {
            text += '\n';
        }
    }

    Token next();

private:
    std::string text;
    bool error_encountered = false;
    int byte_offset = 0;
    int line_number = 1;
    int column_number = 1;
};

enum class NodeType : char {
    id,
    number,
    assignment,
    plus,
    minus,
    multiply,
    divide,
    negate,
    parentheses,
    statement,
    comma,
    fncall,
    empty,
};

struct Node final {
    Node(NodeType type, const Token &t)
        : type(type), value(std::monostate()), left{}, right{}, line_number(t.line_number),
          column_number(t.column_number) {}
    NodeType type;
    std::variant<std::monostate, double, std::string> value;
    std::optional<int> left;
    std::optional<int> right;
    int line_number;
    int column_number;
};

class Parser final {

public:
    Parser(Lexer &l) : l(l){};

    bool parse();

    const std::string get_error() const { return error_message; }

    const std::vector<Node> &get_nodes() const { return nodes; }
    const std::vector<int> &get_statements() const { return statements; }

private:
    bool is_error() const { return !error_message.empty(); }

    bool e1_statement();

    bool e2_comma();

    bool e3_expression() { return e4_add(); }

    bool e4_add();

    bool e5_subtract();

    bool e6_multiply();

    bool e7_divide();

    bool e8_parentheses();

    bool e9_token();

    bool accept(const TokenType type);

    bool expect(const TokenType type);

    void set_error(const char *msg, int line_number, int column_number);

    Lexer &l;
    std::vector<Node> nodes;
    std::vector<int> statements;
    Token t;
    std::string error_message;
};

typedef std::variant<double, std::string> funcall_result;

class ExternalFuncall {
public:
    virtual ~ExternalFuncall() = default;

    virtual funcall_result funcall(const std::string &funname, const std::vector<double> &args) = 0;
};

class FuncallPrinter : public ExternalFuncall {

    funcall_result funcall(const std::string &funname, const std::vector<double> &) override {
        printf("Function %s called.\n", funname.c_str());
        if(funname == "bad_function") {
            return "Bad function name.";
        }
        return 0.0;
    }
};

class Interpreter final {
public:
    explicit Interpreter(const Parser &p, ExternalFuncall *fp)
        : nodes(p.get_nodes()), statements(p.get_statements()), fp(fp) {
        set_variable("pi", M_PI);
        set_variable("e", M_E);
    }

    bool execute_program();

    std::optional<double> get_variable(const char *varname) const {
        return get_variable(std::string(varname));
    }

    std::optional<double> get_variable(const std::string &varname) const {
        auto node = variables.find(varname);
        if(node != variables.end()) {
            return node->second;
        }
        return std::optional<double>();
    }

    const std::string &get_error() const { return error_message; }

private:
    bool set_variable(const std::string &name, double value) {
        // FIXME, check that we don't override global constants.
        variables[name] = value;
        return true;
    }

    bool assignment(const Node &n);

    std::optional<double> expression(const Node &n);

    std::optional<double> eval_multiply(const Node &n);

    std::optional<double> eval_divide(const Node &n);

    std::optional<double> eval_plus(const Node &n);

    std::optional<double> eval_minus(const Node &n);

    std::optional<double> eval_variable(const Node &n);

    std::optional<std::vector<double>> eval_arguments(const Node &n);

    bool eval_args_recursive(std::vector<double> &args, const Node &n);

    std::optional<double> eval_fncall(const Node &n);

    void set_error(const char *msg, int line_number, int column_number);

    void set_error(const std::string &msg, int line_number, int column_number) {
        set_error(msg.c_str(), line_number, column_number);
    }

    void set_error(const std::string &msg, const Node &n) {
        set_error(msg, n.line_number, n.column_number);
    }

    const std::vector<Node> &nodes;
    const std::vector<int> &statements;
    std::string error_message;
    std::unordered_map<std::string, double> variables;
    ExternalFuncall *fp;
};
