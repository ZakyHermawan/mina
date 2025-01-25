#include <cassert>

#include "Lexer.hpp"
#include "Token.hpp"

void tests_token()
{
  Token token(INTEGER, "", 32, 1);
  assert(token.getTokenType() == TokenType::INTEGER);
  assert(std::any_cast<int>(token.getLiteral()) == 32);
  assert(token.getLine() == 1);

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

void tests_lexer()
{
  std::string source = "{}  false !=true";
  Lexer lexer(source);

  assert(lexer.getCurrToken().getTokenType() == LEFT_BRACE);

  lexer.advance();
  assert(lexer.getCurrToken().getTokenType() == RIGHT_BRACE);

  lexer.advance();
  assert(lexer.getCurrToken().getTokenType() == BOOL);

  lexer.advance();
  assert(lexer.getCurrToken().getTokenType() == BANG_EQUAL);

  lexer.advance();
  assert(lexer.getCurrToken().getTokenType() == BOOL);

  Lexer l{std::move(std::string())};
  assert(l.getCurrToken().getTokenType() == TOK_BEGIN);

  std::cout << "TESTS LEXER SUCCESS\n";
}

int main(int argc, char *argv[])
{
  tests_token();
  tests_lexer();

  return 0;
}
