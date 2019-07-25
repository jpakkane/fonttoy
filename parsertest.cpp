#include<regex>
#include<string>
#include<vector>
#include<cstdio>

const std::regex id         (R"([a-zA-Z_]+[a-zA-Z0-9_]*)");
const std::regex number     (R"([0-9]+(.[0-9]*)?)");
const std::regex plus       (R"(\+)");
const std::regex minus      (R"(-)");
const std::regex multiply   (R"(\*)");
const std::regex divide     (R"(/)");
const std::regex equal      (R"(=)");
const std::regex dot        (R"(\.)");
const std::regex semicolon  (R"(;)");
const std::regex whitespace (R"([ \r\t]+)");
const std::regex linefeed   (R"(\n)");
const std::regex lparen     (R"(\()");
const std::regex rparen     (R"(\))");

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
    {TokenType::end_of_tokens, "eot",        std::regex("^DONOTMATCH¤")},
    {TokenType::error,         "error",      std::regex("^DONOTMATCH@")},
    {TokenType::eof,           "eof",        std::regex("^DONOTMATCH½")},
};

// clang-format on

struct Token {
    TokenType type;
    std::string contents;
    int byte_offset;
    int line_number;
    int column_number;
};

class Tokenizer final {
public:
    explicit Tokenizer(const std::string &s) : text(s) {}
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
        for(const auto &tr: token_rules) {
            if(tr.type == TokenType::end_of_tokens) {
                break;
            }
            std::cmatch m;
            if(std::regex_search(text.c_str()+byte_offset, m, tr.r)) {
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

// clang-format on

int main(int, char **) {
    std::string input("1.0 + 2.0*(aaa-b*c)!");
    Tokenizer tokenizer(input);
    Token t = tokenizer.next();
    while(t.type != TokenType::eof && t.type != TokenType::error) {
        printf("Token: %s\n", token_rules[(size_t)t.type].typestr);
        printf(" value: %s\n", t.contents.c_str());
        printf(" line: %d\n col: %d\n", t.line_number, t.column_number);
        t = tokenizer.next();
    }
    if(t.type == TokenType::error) {
        printf("An error occurred: %s\n", t.contents.c_str());
    }
    return 0;
}