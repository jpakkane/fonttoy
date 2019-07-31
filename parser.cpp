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

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif

#include <parser.hpp>
#include <regex>
#include <cmath>

struct TokenDefinition {
    TokenType type;
    const char *typestr;
    std::regex r;
};

// NOTE: these need to be in the same order as the TokenType declarations
// so you can do token_rules[TokenType::linefeed] etc.

// In C++ regexes "match" does not mean "starts with" but "matches all of".
// Thus we need to use search combined with ^.

// WARNING: either Gcc or VS is buggy. The former matches '^foo' as
// "find foo starting fromthe beginning of the given string" whereas the
// latter does "find foo following a line feed anywhere in the string".

// clang-format off

const std::vector<TokenDefinition> token_rules{
    {TokenType::id,            "id",         std::regex(R"(^[a-zA-Z_]+[a-zA-Z0-9_]*)")},
    {TokenType::number,        "number",     std::regex(R"(^[0-9]+(\.[0-9]*)?)")},
    {TokenType::plus,          "plus",       std::regex(R"(^\+)")},
    {TokenType::minus,         "minus",      std::regex(R"(^-)")},
    {TokenType::multiply,      "multiply",   std::regex(R"(^\*)")},
    {TokenType::divide,        "divide",     std::regex(R"(^/)")},
    {TokenType::equal,         "equal",      std::regex(R"(^=)")},
    {TokenType::dot,           "dot",        std::regex(R"(^\.)")},
    {TokenType::semicolon,     "semicolon",  std::regex(R"(^;)")},
    {TokenType::whitespace,    "whitespace", std::regex(R"(^[ \r\t]+)")},
    {TokenType::linefeed,      "linefeed",   std::regex(R"(^\n)")},
    {TokenType::lparen,        "lparen",     std::regex(R"(^\()")},
    {TokenType::rparen,        "rparen",     std::regex(R"(^\))")},
    {TokenType::comma,         "comma",      std::regex(R"(^,)")},
    {TokenType::end_of_tokens, "eot",        std::regex("^DONOTMATCH¤")}, // Never produced.
    {TokenType::error,         "error",      std::regex("^DONOTMATCH§")},
    {TokenType::eof,           "eof",        std::regex("^DONOTMATCH½")},
};

// clang-format on

const char *token_name(const TokenType t) {
    assert((int)t >= 0);
    assert((size_t)t < token_rules.size());
    return token_rules[(int)t].typestr;
}

Token Lexer::next() {
    Token t;
    t.byte_offset = byte_offset;
    t.line_number = line_number;
    t.column_number = column_number;
    if(byte_offset >= (int)text.size()) {
        t.type = TokenType::eof;
        t.contents = "(EOF)";
        return t;
    }
    if(error_encountered) {
        t.type = TokenType::error;
        t.contents = "Unknown character: ";
        t.contents += text[byte_offset];
        return t;
    }
    for(const auto &tr : token_rules) {
        if(tr.type == TokenType::end_of_tokens) {
            break;
        }
        std::cmatch m;
        if(std::regex_search(text.c_str() + byte_offset, m, tr.r)) {
            assert(m.prefix().length() == 0); // You found it, you get to fix it.
            auto &submatch = m[0];
            t.type = tr.type;
            t.contents = text.substr(byte_offset, submatch.length());
            byte_offset += submatch.length();
            if(tr.type == TokenType::linefeed) {
                line_number += 1;
                column_number = 1;
            } else {
                column_number += submatch.length();
            }
            if(t.type == TokenType::whitespace) {
                // The parser does not care about whitespace.
                // Dump it here for simplicity.
                return next();
            }
            return t;
        }
    }
    error_encountered = true;
    return next();
}

const std::vector<const char *> node_names{
    "id",
    "number",
    "assignment",
    "plus",
    "minus",
    "multiply",
    "divide",
    "negate",
    "parentheses",
    "statement",
    "comma",
    "fncall",
    "empty",
};

const char *node_name(const NodeType t) {
    assert((int)t >= 0);
    assert((size_t)t < node_names.size());
    return node_names[(int)t];
}

bool Parser::parse() {
    assert(nodes.size() == 0);
    assert(!is_error());
    t = l.next();
    do {
        if(accept(TokenType::eof)) {
            return true;
        }
        if(t.type >= TokenType::end_of_tokens) {
            error_message = "Lexing failed: " + t.contents;
            return false;
        }
        if(!e1_statement()) {
            return false;
        }
    } while(!is_error());
    return false;
}

bool Parser::e1_statement() {
    if(!e3_expression()) {
        return false;
    }
    if(nodes.empty()) {
        set_error("Something really weird has happened.", -1, -1);
        return false;
    }
    if(nodes.back().type == NodeType::id) {
        if(!expect(TokenType::equal)) {
            return false;
        }
        int id_index = nodes.size() - 1;
        if(!e3_expression()) {
            return false;
        }
        int val_index = nodes.size() - 1;
        nodes.emplace_back(NodeType::assignment, t);
        statements.push_back(nodes.size() - 1);
        nodes.back().left = id_index;
        nodes.back().right = val_index;
    } else {
        // A plain expression like: fun_call(1)
        statements.push_back(nodes.size() - 1);
    }
    if(!expect(TokenType::linefeed)) {
        return false;
    }
    return true;
}

bool Parser::e2_comma() {
    if(!e3_expression()) {
        return false;
    }
    Token comma_token = t;
    if(accept(TokenType::comma)) {
        int left = nodes.size() - 1;
        if(!e2_comma()) {
            return false;
        }
        int right = nodes.size() - 1;
        nodes.emplace_back(NodeType::comma, comma_token);
        nodes.back().left = left;
        nodes.back().right = right;
    }
    return true;
}

bool Parser::e4_add() {
    if(!e5_subtract()) {
        return false;
    }
    Token add_token = t;
    if(accept(TokenType::plus)) {
        int left = nodes.size() - 1;
        if(!e4_add()) {
            return false;
        }
        int right = nodes.size() - 1;
        nodes.emplace_back(NodeType::plus, add_token);
        nodes.back().left = left;
        nodes.back().right = right;
    }
    return true;
}

bool Parser::e5_subtract() {
    if(!e6_multiply()) {
        return false;
    }
    Token minus_token = t;
    if(accept(TokenType::minus)) {
        int left = nodes.size() - 1;
        if(!e5_subtract()) {
            return false;
        }
        int right = nodes.size() - 1;
        nodes.emplace_back(NodeType::minus, minus_token);
        nodes.back().left = left;
        nodes.back().right = right;
    }
    return true;
}

bool Parser::e6_multiply() {
    if(!e7_divide()) {
        return false;
    }
    Token mul_token = t;
    if(accept(TokenType::multiply)) {
        int left = nodes.size() - 1;
        if(!e6_multiply()) {
            return false;
        }
        int right = nodes.size() - 1;
        nodes.emplace_back(NodeType::multiply, mul_token);
        nodes.back().left = left;
        nodes.back().right = right;
    }
    return true;
}

bool Parser::e7_divide() {
    if(!e8_parentheses()) {
        return false;
    }
    Token div_token = t;
    if(accept(TokenType::divide)) {
        int left = nodes.size() - 1;
        if(!e7_divide()) {
            return false;
        }
        int right = nodes.size() - 1;
        nodes.emplace_back(NodeType::divide, div_token);
        nodes.back().left = left;
        nodes.back().right = right;
    }
    return true;
}

bool Parser::e8_parentheses() {
    if(!e9_token()) {
        return false;
    }
    Token previous = t;
    if(accept(TokenType::lparen)) {
        if(nodes.back().type == NodeType::empty) {
            // Parenthesized evaluation: 3*(1+2)
            if(!e3_expression()) {
                return false;
            }
            if(!expect(TokenType::rparen)) {
                return false;
            }
        } else if(nodes.back().type == NodeType::id) {
            // Function call: max(1, 2)
            int left = nodes.size() - 1;
            if(!e2_comma()) {
                return false;
            }
            if(!expect(TokenType::rparen)) {
                return false;
            }
            int right = nodes.size() - 1;
            nodes.emplace_back(NodeType::fncall, previous);
            nodes.back().left = left;
            nodes.back().right = right;
            return true;
        } else {
            // Multiplication, e.g.: 3(1+2)
            printf("Not implemented yet.\n");
            assert(false);
        }
    }
    return true;
}

bool Parser::e9_token() {
    auto current_token = t;
    if(accept(TokenType::id)) {
        nodes.emplace_back(NodeType::id, current_token);
        nodes.back().value = current_token.contents;
        return true;
    }
    if(accept(TokenType::number)) {
        nodes.emplace_back(NodeType::number, current_token);
        // WARNING: locale dependent! FIXME.
        nodes.back().value = strtod(current_token.contents.c_str(), nullptr);
        return true;
    }
    nodes.emplace_back(NodeType::empty, t);
    return true;
}

bool Parser::accept(const TokenType type) {
    if(t.type == type) {
        t = l.next();
        return true;
    }
    return false;
}

bool Parser::expect(const TokenType type) {
    if(!accept(type)) {
        std::string err = "Parse error: got token ";
        err += token_name(t.type);
        err += " expected ";
        err += token_name(type);
        err += ".";
        set_error(err.c_str(), t.line_number, t.column_number);
        return false;
    }
    return true;
}

void Parser::set_error(const char *msg, int line_number, int column_number) {
    assert(error_message.empty());
    error_message = std::to_string(line_number);
    error_message += ':';
    error_message += std::to_string(column_number);
    error_message += ' ';
    error_message += msg;
}

Interpreter::Interpreter(const Parser &p, ExternalFuncall *fp)
    : nodes(p.get_nodes()), statements(p.get_statements()), fp(fp) {
    set_variable("pi", M_PI);
    set_variable("e", M_E);
}

bool Interpreter::execute_program() {
    for(const auto statement : statements) {
        const auto &n = nodes[statement];
        if(n.type == NodeType::assignment) {
            if(!assignment(n)) {
                return false;
            }
        } else {
            if(!expression(n)) {
                return false;
            }
        }
    }
    return true;
}

bool Interpreter::assignment(const Node &n) {
    assert(n.type == NodeType::assignment);
    const std::string &varname = std::get<std::string>(nodes[n.left.value()].value);
    auto value = expression(nodes[n.right.value()]);
    if(!value) {
        return false;
    }
    return set_variable(varname, value.value());
}

std::optional<double> Interpreter::expression(const Node &n) {
    switch(n.type) {
    case NodeType::number:
        return std::get<double>(n.value);
    case NodeType::multiply:
        return eval_multiply(n);
    case NodeType::divide:
        return eval_divide(n);
    case NodeType::plus:
        return eval_plus(n);
    case NodeType::minus:
        return eval_minus(n);
    case NodeType::id:
        return eval_variable(n);
    case NodeType::fncall:
        return eval_fncall(n);
    case NodeType::empty:
        return 0.0;
    default:
        std::string err("Unknown node type: ");
        err += node_name(n.type);
        set_error(err, n);
        return std::optional<double>();
    }
}

std::optional<double> Interpreter::eval_multiply(const Node &n) {
    auto lval = expression(nodes[n.left.value()]);
    if(!lval) {
        return lval;
    }
    auto rval = expression(nodes[n.right.value()]);
    if(!rval) {
        return rval;
    }
    return *lval * *rval;
}

std::optional<double> Interpreter::eval_divide(const Node &n) {
    auto lval = expression(nodes[n.left.value()]);
    if(!lval) {
        return lval;
    }
    auto rval = expression(nodes[n.right.value()]);
    if(!rval) {
        return rval;
    }
    if(fabs(*rval) < 0.00001) {
        set_error("Divide by zero.", n);
        return std::optional<double>();
    }
    return *lval / *rval;
}

std::optional<double> Interpreter::eval_plus(const Node &n) {
    auto lval = expression(nodes[n.left.value()]);
    if(!lval) {
        return lval;
    }
    auto rval = expression(nodes[n.right.value()]);
    if(!rval) {
        return rval;
    }
    return *lval + *rval;
}

std::optional<double> Interpreter::eval_minus(const Node &n) {
    auto lval = expression(nodes[n.left.value()]);
    if(!lval) {
        return lval;
    }
    auto rval = expression(nodes[n.right.value()]);
    if(!rval) {
        return rval;
    }
    return *lval - *rval;
}

std::optional<double> Interpreter::eval_variable(const Node &n) {
    const auto &varname = std::get<std::string>(n.value);
    auto val = get_variable(varname);
    if(!val) {
        std::string err("Unknown variable: ");
        err += varname;
        err += ".";
        set_error(err, n);
        return std::optional<double>();
    }
    return val.value();
}

std::optional<std::vector<double>> Interpreter::eval_arguments(const Node &n) {
    std::vector<double> args;
    if(!eval_args_recursive(args, n)) {
        return std::optional<std::vector<double>>();
    }
    return args;
}

bool Interpreter::eval_args_recursive(std::vector<double> &args, const Node &n) {
    if(n.type == NodeType::comma) {
        if(!eval_args_recursive(args, nodes[n.left.value()])) {
            return false;
        }
        if(!eval_args_recursive(args, nodes[n.right.value()])) {
            return false;
        }
        return true;
    }
    auto v = expression(n);
    if(!v) {
        return false;
    }
    args.push_back(*v);
    return true;
}

std::optional<double> Interpreter::eval_fncall(const Node &n) {
    auto argsp = eval_arguments(nodes[n.right.value()]);
    if(!argsp) {
        return std::optional<double>();
    }
    const auto &args = *argsp;
    const auto &fname = std::get<std::string>(nodes[n.left.value()].value);
    if(fname == "cos") {
        if(args.size() != 1) {
            set_error("Incorrect number of arguments.", n);
            return std::optional<double>();
        }
        return cos(args[0]);
    } else {
        auto res = fp->funcall(fname, args);
        if(std::holds_alternative<std::string>(res)) {
            set_error(fname + ": " + std::get<std::string>(res), n);
            return std::optional<double>();
        }
        return std::get<double>(res);
    }
}

void Interpreter::set_error(const char *msg, int line_number, int column_number) {
    assert(error_message.empty());
    error_message = std::to_string(line_number);
    error_message += ':';
    error_message += std::to_string(column_number);
    error_message += ' ';
    error_message += msg;
}
