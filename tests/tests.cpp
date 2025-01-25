#include <cassert>

#include "Lexer.hpp"
#include "Token.hpp"

void tests_token()
{
  Token token(INTEGER, "", 32, 1);
  assert(token.getTokenType() == TokenType::INTEGER);
  assert(std::any_cast<int>(token.getLiteral()) == 32);
  assert(token.getLine() == 1);

  Token lb(LEFT_BRACE, "", "", 1);
  assert(lb.getTokenType() == LEFT_BRACE);
  assert(token.getLine() == 1);

  Token rb(RIGHT_BRACE, "", "", 2);
  assert(rb.getTokenType() == RIGHT_BRACE);
  assert(rb.getLine() == 2);

  Token lp(LEFT_PAREN, "", "", 3);
  assert(lp.getTokenType() == LEFT_PAREN);
  assert(lp.getLine() == 3);

  Token rp(RIGHT_PAREN, "", "", 4);
  assert(rp.getTokenType() == RIGHT_PAREN);
  assert(rp.getLine() == 4);

  Token la(LEFT_ANGLE, "", "", 5);
  assert(la.getTokenType() == LEFT_ANGLE);
  assert(la.getLine() == 5);

  Token ra(RIGHT_ANGLE, "", "", 6);
  assert(ra.getTokenType() == RIGHT_ANGLE);
  assert(ra.getLine() == 6);

  Token col(COLON, "", "", 1);
  assert(col.getTokenType() == COLON);

  Token semi(SEMI, "", "", 1);
  assert(semi.getTokenType() == SEMI);

  Token eq(EQUAL, "", "", 1);
  assert(eq.getTokenType() == EQUAL);

  Token hash(HASH, "", "", 1);
  assert(hash.getTokenType() == HASH);

  Token less(LESS, "", "", 1);
  assert(less.getTokenType() == LESS);

  Token greater(GREATER, "", "", 1);
  assert(greater.getTokenType() == GREATER);

  Token plus(PLUS, "", "", 1);
  assert(plus.getTokenType() == PLUS);

  Token min(MIN, "", "", 1);
  assert(min.getTokenType() == MIN);

  Token pipe(PIPE, "", "", 1);
  assert(pipe.getTokenType() == PIPE);

  Token star(STAR, "", "", 1);
  assert(star.getTokenType() == STAR);

  Token slash(SLASH, "", "", 1);
  assert(slash.getTokenType() == SLASH);

  Token amp(AMPERSAND, "", "", 1);
  assert(amp.getTokenType() == AMPERSAND);

  Token tilde(TILDE, "", "", 1);
  assert(tilde.getTokenType() == TILDE);

  Token comma(COMMA, "", "", 1);
  assert(comma.getTokenType() == COMMA);

  Token col_eq(COLON_EQUAL, "", "", 1);
  assert(col_eq.getTokenType() == COLON_EQUAL);

  Token le(LESS_EQUAL, "", "", 1);
  assert(le.getTokenType() == LESS_EQUAL);

  Token ge(GREATER_EQUAL, "", "", 1);
  assert(ge.getTokenType() == GREATER_EQUAL);

  Token be(BANG_EQUAL, "", "", 1);
  assert(be.getTokenType() == BANG_EQUAL);

  Token id(IDENTIFIER, "sample_id", "", 1);
  assert(id.getTokenType() == IDENTIFIER);
  assert(id.getLexme() == std::string("sample_id"));

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
