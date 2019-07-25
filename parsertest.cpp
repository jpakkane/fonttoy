#include<regex>
#include<string>
#include<cstdio>

// clang-format off

const std::regex id       (R"([a-zA-Z_]+[a-zA-Z0-9_]*)");
const std::regex number   (R"([0-9]+(.[0-9]*)?)");
const std::regex plus     (R"(\+)");
const std::regex minus    (R"(-)");
const std::regex multiply (R"(\*)");
const std::regex divide   (R"(/)");

// clang-format on

int main(int argc, char **argv) {
    if(argc != 2) {
        printf("Fail.\n");
        return 1;
    }
    if(std::regex_search(argv[1], id)) {
        printf("Was found.\n");
    } else {
        printf("Was not found.\n");
    }
    return 0;
}
