#include "Token.hpp"

int main (int argc, char *argv[]) {
  Token token(INTEGER, "", 32, 1);
  std::string tmp = "asdad";
  Token t(STRING, tmp, tmp, 2);
  std::cout << token << std::endl;
  std::cout << t << std::endl;

  return 0;
}
