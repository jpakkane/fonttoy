#include <regex>
#include <string>
#include <vector>
#include <variant>
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

struct TokenDefinition {
    TokenType type;
    const char *typestr;
    std::regex r;
};

// NOTE: these need to be in the same order as the TokenType declarations
// so you can do token_rules[TokenType::linefeed] etc.

// In C++ regexes "match" does not mean "starts with" but "matches all of".
// Thus we need to use search combined with ^.

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

    Token next() {
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

struct Node final {
    explicit Node(NodeType type, const Token &t)
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

    bool parse() {
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

    const std::string get_error() const { return error_message; }

    const std::vector<Node> &get_nodes() const { return nodes; }
    const std::vector<int> &get_statements() const { return statements; }

private:
    bool is_error() const { return !error_message.empty(); }

    bool e1_statement() {
        if(!e3_expression()) {
            return false;
        }
        if(nodes.empty()) {
            set_error("Something really weird has happened.", -1, -1);
            return false;
        }
        if(nodes.back().type != NodeType::id) {
            std::string err = "Incorrect node type for statement: ";
            err += token_name(t.type);
            set_error(err.c_str(), nodes.back().line_number, nodes.back().column_number);
            return false;
        }
        if(!expect(TokenType::equal)) {
            set_error("Missing equals sign", t.line_number, t.column_number);
            return false;
        }
        int id_index = nodes.size()-1;
        if(!e3_expression()) {
            return false;
        }
        int val_index = nodes.size()-1;
        nodes.emplace_back(NodeType::assignment, t);
        statements.push_back(nodes.size()-1);
        nodes.back().left = id_index;
        nodes.back().right = val_index;
        if(!expect(TokenType::linefeed)) {
            return false;
        }
        return true;
    }

    bool e2_comma() {
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

    bool e3_expression() {
        return e4_add();
    }

    bool e4_add() {
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

    bool e5_subtract() {
        return e6_multiply();
    }

    bool e6_multiply() {
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

    bool e7_divide() {
        if(!e8_parentheses()) {
            return false;
        }
        return true;
    }

    bool e8_parentheses() {
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
                int left = nodes.size()-1;
                if(!e2_comma()) {
                    return false;
                }
                if(!expect(TokenType::rparen)) {
                    return false;
                }
                int right = nodes.size()-1;
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

    bool e9_token() {
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

    bool accept(const TokenType type) {
        if(t.type == type) {
            t = l.next();
            return true;
        }
        return false;
    }

    bool expect(const TokenType type) {
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

    void set_error(const char *msg, int line_number, int column_number) {
        assert(error_message.empty());
        error_message = std::to_string(line_number);
        error_message += ':';
        error_message += std::to_string(column_number);
        error_message += ' ';
        error_message += msg;
    }

    Lexer &l;
    std::vector<Node> nodes;
    std::vector<int> statements;
    Token t;
    std::string error_message;
};

class Interpreter final {
public:

    explicit Interpreter(const Parser &p) : nodes(p.get_nodes()), statements(p.get_statements()) {
        set_variable("pi", M_PI);
        set_variable("e", M_E);
    }

    bool execute_program() {
        for(const auto statement: statements) {
            const auto &n = nodes[statement];
            if (n.type == NodeType::assignment) {
                if(!assignment(n)) {
                    return false;
                }
            } else {
                std::string err("Unknown node type ");
                err += node_name(n.type);
                set_error(err, n.line_number, n.column_number);
                return false;
            }
        }
        return true;
    }

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

    const std::string& get_error() const { return error_message; }

private:

    bool set_variable(const std::string &name, double value) {
        // FIXME, check that we don't override global constants.
        variables[name] = value;
        return true;
    }

    bool assignment(const Node &n) {
        assert(n.type == NodeType::assignment);
        const std::string &varname = std::get<std::string>(nodes[n.left.value()].value);
        auto value = expression(nodes[n.right.value()]);
        if(!value) {
            return false;
        }
        return set_variable(varname, value.value());
    }

    std::optional<double> expression(const Node &n) {
        switch(n.type) {
        case NodeType::number: return std::get<double>(n.value);
        case NodeType::multiply: return eval_multiply(n);
        case NodeType::plus: return eval_plus(n);
        case NodeType::id: return eval_variable(n);
        case NodeType::fncall: return eval_fncall(n);
        default:
            std::string err("Unknown node type: ");
            err += node_name(n.type);
            set_error(err, n);
            return std::optional<double>();
        }
    }

    std::optional<double> eval_multiply(const Node &n) {
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

    std::optional<double> eval_plus(const Node &n) {
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

    std::optional<double> eval_variable(const Node &n) {
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

    std::optional<std::vector<double>> eval_arguments(const Node &n) {
        std::vector<double> args;
        if(!eval_args_recursive(args, n)) {
            return std::optional<std::vector<double>>();
        }
        return args;
    }

    bool eval_args_recursive(std::vector<double> &args, const Node &n) {
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

    std::optional<double> eval_fncall(const Node &n) {
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
            std::string err("Unknown function: ");
            err += fname;
            set_error(err, n);
            return std::optional<double>();
        }
    }

    void set_error(const char *msg, int line_number, int column_number) {
        assert(error_message.empty());
        error_message = std::to_string(line_number);
        error_message += ':';
        error_message += std::to_string(column_number);
        error_message += ' ';
        error_message += msg;
    }

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

};

int main(int, char **) {
    std::string input("y=2\nx = 3*cos(y*pi)");
    //std::string input("x = (1 + 2)*3");
    Lexer tokenizer(input);
    Parser p(tokenizer);
    if(!p.parse()) {
        printf("Parser error: %s\n", p.get_error().c_str());
        return 1;
    }
    Interpreter i(p);
    if(!i.execute_program()) {
        printf("Interpreter error: %s\n", i.get_error().c_str());
        return 1;
    }
    printf("Value of %s is %f\n", "x", *i.get_variable("x"));
    return 0;
}
