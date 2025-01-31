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
    std::cout << parser.getCurrToken() << std::endl;
  }
}

void runFile() {}

int main(int argc, char *argv[])
{
  if (argc == 1)
  {
    repl();
  }
  else if (argc == 2)
  {
    runFile();
  }
  else
  {
    std::cerr << "Usage: ./mina [path]\n";
  }
  Parser p("true");
  std::cout << p.getCurrToken() << std::endl;

  return 0;
}
