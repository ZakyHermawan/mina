/*
#include <fstream>
#include <sstream>

#include "Lexer.hpp"
#include "Parser.hpp"
#include "Token.hpp"

void repl()
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

void runFile(const char* fileName)
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
  // uncomment this line and comment the line before return statement if you
  // dont want to hardcode the source code location.

  //if (argc == 1)
  //{
  //  repl();
  //}
  //else if (argc == 2)
  //{
  //  runFile(argv[1]);
  //}
  //else
  //{
  //  std::cerr << "Usage: ./mina [path]\n";
  //}

    runFile("C:\\Users\\zakyh\\source\\repos\\mina\\samples\\tes.txt");
    return 0;
}
*/


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
