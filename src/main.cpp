#define ASMJIT_STATIC
#include <stdio.h>
#include <asmjit/asmjit.h>

#include <fstream>
#include <sstream>

#include "Lexer.hpp"
#include "Parser.hpp"
#include "Token.hpp"

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

using namespace asmjit;

typedef void (*Func)(void);

int main(int argc, char* argv[]) {
    FileLogger logger(stdout);  // Logger should always survive CodeHolder.
    JitRuntime rt;
    CodeHolder code;

    code.init(rt.environment(), rt.cpuFeatures());
    code.setLogger(&logger); // attach logger

    runFile("C:\\Users\\zakyh\\source\\repos\\mina\\samples\\tes5.txt");
    return 0;
}
