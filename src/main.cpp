#include <sstream>

#include "Lexer.hpp"
#include "Token.hpp"
#include "Parser.hpp"

#include "tests/test_lexer.hpp"

#include <fstream>

static void repl()
{
    while (1)
    {
        std::string source;
        std::cout << "repl> ";
        getline(std::cin, source);
        if (std::cin.eof())
        {
            break;
        }
        Parser parser(std::move(source));
        Token currToken;
        parser.program();
    }
}

static void runFile(const char* fileName)
{
    std::ifstream fileStream;
    fileStream.open(fileName);
    std::string source;
    if (fileStream.is_open())
    {
        std::stringstream ss;
        ss << fileStream.rdbuf();
        source = ss.str();
    }

    Parser parser(std::move(source));
    Token currToken;
    parser.program();
}

int main(int argc, char* argv[])
{
    tests_token();
    tests_lexer();

    runFile("C:\\Users\\zakyh\\source\\repos\\mina\\samples\\tes5.txt");
    return 0;
}
