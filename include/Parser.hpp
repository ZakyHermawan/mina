#pragma once
#include "Lexer.hpp"

#include <iostream>

class Parser
{
 public:
  Parser(std::string val) : m_lexer{std::move(val)}, m_isError(false) {}

  Parser() : m_lexer{} {}

  Parser(Parser &&) = default;
  Parser(const Parser &) = default;
  Parser &operator=(Parser &&) = default;
  Parser &operator=(const Parser &) = default;
  ~Parser() = default;

  // panic mode
  void exitParse(std::string msg)
  {
    std::cerr << "Error on line " << getCurrLine();
              << ": " << msg << std::endl;
    exit(1);
  }

  bool isFinished() const { return m_lexer.isFinished(); }

  Token getCurrToken() const { return m_lexer.getCurrToken(); }

  TokenType getCurrTokenType() const { return getCurrToken.getTokenType(); }

  unsigned int getCurrLine() const { return m_lexer.getCurrLine(); }

  void advance() { m_lexer.advance(); }

  /**
   * At the end of every parsing functions,
   * if there is no error,
   * always make sure the current token is pointing to the next token (use advance)
   * */

  /*
   * program            ::= scope R38 R0
                   |   scope error
                   |   error scope
                   |   error ;
   * */
  void program()
  {
    scope();
    if(m_isError && !isFinished())
    {
      scope();
    }
    std::cout << "Finish parsing\n";
  }

  /*
   * scope              ::= OPEN C0 R1 declarations C1 R3 ENDSTMT statements C2 R5 CLOSE
                   |   OPEN ENDSTMT statements CLOSE;
   * */
  void scope()
  {
    if(getCurrTokenType() != LEFT_BRACE)
    {
      exitParse("Expected '{'");
    }

    advance();

    if(getCurrTokenType == SEMI)
    {
      advance();
      statements();

      if(getCurrTokenType != RIGHT_BRACE)
      {
        exitParse("Expected '}'");
      }

      advance();
    }
    declarations();

    if(getCurrTokenType() != SEMI)
    {
      exitParse("Expected '('");
    }

    advance();
    statements();

    if(getCurrTokenType() != RIGHT_BRACE)
    {
      exitParse("Expected '}'");
    }

    advance();
  }

  /*
   * statements         ::= 
                   |   statement statements
                   |   error statements ;
   * */
  void statements()
  {
    if(isFinished())
    {
      return;
    }

    try {
      statement();
      statements();
    }
    catch (const std::runtime_error&)
    {
      m_isError = true;
      statements();
    }
  }

 private:
  Lexer m_lexer;
  bool m_isError;
};

