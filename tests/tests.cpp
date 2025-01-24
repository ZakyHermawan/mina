#include "Token.hpp"

#include <cassert>

void tests_token()
{
  Token token(INTEGER, "", 32, 1);
  assert(token.getTokenType() == TokenType::INTEGER);
  assert(std::any_cast<int>(token.getLiteral()) == 32);

  Token t(STRING, "", std::string("asdasd"), 2);
  assert(t.getTokenType() == TokenType::STRING);
  assert(std::any_cast<std::string>(t.getLiteral()) == std::string("asdasd"));
  
  Token t2(BOOL, "", true, 3);
  assert(t2.getTokenType() == TokenType::BOOL);
  assert(std::any_cast<bool>(t2.getLiteral()) == true);

  Token t3(VAR, "var", std::string(), 4);
  assert(t3.getTokenType() == TokenType::VAR);
  assert(std::any_cast<std::string>(t3.getLexme()) == std::string("var"));

  std::cout << "TESTS TOKEN SUCCESS\n";
}

int main (int argc, char *argv[]) {
  tests_token();

  return 0;
}
