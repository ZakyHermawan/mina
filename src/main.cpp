#include <string>
#include <fstream>
#include <sstream>
#include <utility>
#include <iostream>

#include "Token.hpp"
#include "Parser.hpp"

//#include "tests/test_lexer.hpp"

using namespace mina;

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

static void runAllSamples()
{
    runFile("E:\\SourceCodes\\mina\\mina\\samples\\tes.txt");
    runFile("E:\\SourceCodes\\mina\\mina\\samples\\tes2.txt");
    runFile("E:\\SourceCodes\\mina\\mina\\samples\\tes3.txt");
    runFile("E:\\SourceCodes\\mina\\mina\\samples\\tes4.txt");
    runFile("E:\\SourceCodes\\mina\\mina\\samples\\tes5.txt");
    runFile("E:\\SourceCodes\\mina\\mina\\samples\\tes6.txt");
    runFile("E:\\SourceCodes\\mina\\mina\\samples\\tes7.txt");
    runFile("E:\\SourceCodes\\mina\\mina\\samples\\tes8.txt");
    runFile("E:\\SourceCodes\\mina\\mina\\samples\\tes9.txt");
    runFile("E:\\SourceCodes\\mina\\mina\\samples\\tes10.txt");
}

int main(int argc, char* argv[])
{
    //tests_token();
    //tests_lexer();

    //runAllSamples();

    runFile("E:\\SourceCodes\\mina\\mina\\samples\\tes2.txt");
    return 0;
}
