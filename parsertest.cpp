#include <regex>
#include <string>
#include <vector>
#include <variant>
#include <cstdio>
#include <cassert>

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
    {TokenType::number,        "number",     std::regex(R"(^[0-9]+(.[0-9]*)?)")},
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
    {TokenType::end_of_tokens, "eot",        std::regex("^DONOTMATCH¤")}, // Never produced.
    {TokenType::error,         "error",      std::regex("^DONOTMATCH§")},
    {TokenType::eof,           "eof",        std::regex("^DONOTMATCH½")},
};

// clang-format on

const char* token_name(const TokenType t) {
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
    empty,
};

struct Node final {
    explicit Node(NodeType type, const Token &t)
        : type(type), value(std::monostate()), left{}, right{}, line_number(t.line_number), column_number(t.column_number) {
    }
    NodeType type;
    std::variant<std::monostate, double, std::string> value;
    std::optional<int> left;
    std::optional<int> right;
    int line_number;
    int column_number;
};

class Parser final {

public:
    Parser(Lexer &l) : l(l) {};

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

private:
    bool is_error() const { return !error_message.empty(); }

    bool e1_statement() {
        if(t.type != TokenType::id) {
            error_message = "Incorrect node type for statement: ";
            error_message += token_name(t.type);
            return false;
        }
        auto id_index = nodes.size();
        nodes.emplace_back(NodeType::id, t);
        nodes.back().value = t.contents;
        if(!expect(TokenType::equal)) {
            return false;
        }
        auto assignment_index = nodes.size();
        nodes.emplace_back(NodeType::assignment, t);
        statements.push_back(assignment_index);
        if(!e2_arithmetic()) {
            return false;
        }
        if(!expect(TokenType::linefeed)) {
            return false;
        }
        nodes[assignment_index].left = id_index;
        nodes[assignment_index].right = nodes.size()-1;

        return true;
    }

    bool e2_arithmetic() {
        // FIXME: add implementation here.
        return false;
    }

    bool e3_parentheses() {
        if(!e4_token()) {
            return false;
        }
        if(accept(TokenType::lparen)) {
            if(!e2_arithmetic()) {
                return false;
            }
            if(!expect(TokenType::rparen)) {
                return false;
            }
        }
        return true;
    }

    bool e4_token() {
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
        std::string err{"Got unexpected token: "};
        err += token_name(t.type);
        set_error(err.c_str(), t.line_number, t.column_number);
        return false;
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

int main(int, char **) {
    std::string input("x = 1.0 + 2.0*3");
    Lexer tokenizer(input);
    Token t = tokenizer.next();
    while(t.type != TokenType::eof && t.type != TokenType::error) {
        printf("Token: %s\n", token_name(t.type));
        printf(" value: \"%s\"\n", t.contents.c_str());
        printf(" line: %d\n col: %d\n", t.line_number, t.column_number);
        t = tokenizer.next();
    }
    if(t.type == TokenType::error) {
        printf("An error occurred: %s\n", t.contents.c_str());
    }
    return 0;
}
