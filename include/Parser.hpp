#include "Lexer.hpp"

class Parser
{
 public:
  Parser(std::string val) : m_lexer{std::move(val)} {}

  Parser() : m_lexer{} {}

  Parser(Parser &&) = default;
  Parser(const Parser &) = default;
  Parser &operator=(Parser &&) = default;
  Parser &operator=(const Parser &) = default;
  ~Parser() = default;

  Token getCurrToken() const { return m_lexer.getCurrToken(); }

  // temporary
  void advance() { m_lexer.advance(); }

 private:
  Lexer m_lexer;
};
