#pragma once
#include <iostream>
#include <stdexcept>

#include "Lexer.hpp"
#include "Semantic.hpp"

class Parser
{
 public:
  Parser(std::string source) : m_lexer{std::move(source)}, m_isError(false) {}

  Parser() : m_lexer{} {}

  Parser(Parser &&) = default;
  Parser(const Parser &) = default;
  Parser &operator=(Parser &&) = default;
  Parser &operator=(const Parser &) = default;
  ~Parser() = default;

  // panic mode
  void exitParse(std::string msg)
  {
    std::cerr << "Error on line " << getCurrLine() << ": " << msg << ", got "
              << getCurrToken() << std::endl;
    exit(1);
  }

  bool isFinished() const { return m_lexer.isFinished(); }

  Token getCurrToken() const { return m_lexer.getCurrToken(); }

  TokenType getCurrTokenType() const { return getCurrToken().getTokenType(); }

  unsigned int getCurrLine() const { return m_lexer.getCurrLine(); }

  void advance() { m_lexer.advance(); }

  /**
   * At the end of every parsing functions,
   * if there is no error,
   * always make sure the current token is pointing to the next token (use
   * advance)
   * */

  /*
   * program            ::= scope R38 R0
                   |   scope error
                   |   error scope
                   |   error ;
   * */
  void program()
  {
    try
    {
      scope();
    }
    catch (const std::runtime_error &)
    {
      // parse: error scope
      if (m_isError && !isFinished())
      {
        scope();
      }
    }

    std::cout << "Finish parsing\n";
  }

  /*
   * scope              ::= LEFT_BRACE S0 R1 declarations S1 R3 SEMI statements
   C2 R5 RIGHT_BRACE |   LEFT_BRACE SEMI statements RIGHT_BRACE;
   * */
  void scope()
  {
    if (getCurrTokenType() != LEFT_BRACE)
    {
      exitParse("Expected '{'");
    }

    m_sema.S0();
    advance();

    // Parse: LEFT_BRACE SEMI statements RIGHT_BRACE
    if (getCurrTokenType() == SEMI)
    {
      advance();
      statements(RIGHT_BRACE);

      if (getCurrTokenType() != RIGHT_BRACE)
      {
        exitParse("Expected '}'");
      }

      advance();
      return;
    }

    declarations(SEMI);
    m_sema.S1();

    if (getCurrTokenType() != SEMI)
    {
      exitParse("Expected ';'");
    }

    advance();
    statements(RIGHT_BRACE);

    if (getCurrTokenType() != RIGHT_BRACE)
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
  void statements(TokenType stopToken = TOK_EOF,
                  TokenType secondStopToken = TOK_EOF)
  {
    if (isFinished())
    {
      return;
    }

    if (getCurrTokenType() == stopToken ||
        getCurrTokenType() == secondStopToken)
    {
      return;
    }

    try
    {
      statement();
    }
    catch (const std::runtime_error &)
    {
      m_isError = true;
    }

    statements(stopToken, secondStopToken);
  }

  /*
   * statement          ::= IDENTIFIER S6 assignOrCall
                   |   IF expression C13 C11 R8 THEN statements optElse END IF
                   |   REPEAT R11 statements UNTIL expression C13 C11 R18 R8 R12
   R10 |   LOOP R11 R53 statements R12 END LOOP R51 |   EXIT R52 |   PUT outputs
                   |   GET inputs
                   |   scope ;
   * */
  void statement()
  {
    if (getCurrTokenType() == IDENTIFIER)
    {
      m_sema.S6();
      advance();
      assignOrCall();
    }
    else if (getCurrTokenType() == IF)
    {
      advance();
      expression();

      if (getCurrTokenType() != THEN)
      {
        exitParse("Expected 'then' after if expression");
      }

      advance();
      statements(ELSE, END);
      optElse();

      if (getCurrTokenType() != END)
      {
        exitParse("Expected 'END'");
      }

      advance();

      if (getCurrTokenType() != IF)
      {
        exitParse("Expected 'IF'");
      }

      advance();
    }
    else if (getCurrTokenType() == REPEAT)
    {
      advance();
      statements(UNTIL);

      if (getCurrTokenType() != UNTIL)
      {
        exitParse("Expected 'until'");
      }

      advance();
      expression();
    }
    else if (getCurrTokenType() == LOOP)
    {
      advance();
      statements(END);

      if (getCurrTokenType() != END)
      {
        exitParse("Expected 'end'");
      }

      advance();

      if (getCurrTokenType() != LOOP)
      {
        exitParse("expected 'loop'");
      }

      advance();
    }
    else if (getCurrTokenType() == END)
    {
      advance();
    }
    else if (getCurrTokenType() == PUT)
    {
      advance();
      outputs();
    }
    else if (getCurrTokenType() == GET)
    {
      advance();
      inputs();
    }
    else
    {
      scope();
    }
  }

  /*
   * optElse            ::= R10
                   |   ELSE R7 R10 statements R9 ;
   * Because else statement will always end with END statement,
   * put END as stopToken for statements
   * */
  void optElse()
  {
    if (getCurrTokenType() == ELSE)
    {
      advance();
      statements(END);
    }
  }

  /*
   * assignOrCall       ::= R45 R44 C7
                   |   LEFT_PAREN R45 arguments RIGHT_PAREN R44 C7
                   |   COLON_EQUAL C20 R31 assignExpression
                   |   LEFT_SQUARE C21 R40 subscript RIGHT_SQUARE EQUAL
  assignExpression ;
  * */
  void assignOrCall()
  {
    if (getCurrTokenType() == LEFT_PAREN)
    {
      advance();
      arguments();

      if (getCurrTokenType() != RIGHT_PAREN)
      {
        exitParse("Expected ')'");
      }

      advance();
    }
    else if (getCurrTokenType() == COLON_EQUAL)
    {
      advance();
      assignExpression();
    }
    else if (getCurrTokenType() == LEFT_SQUARE)
    {
      advance();
      subscript();

      if (getCurrTokenType() != RIGHT_SQUARE)
      {
        exitParse("Expected ']'");
      }

      advance();

      if (getCurrTokenType() != COLON_EQUAL)
      {
        exitParse("Expected ':='");
      }

      advance();
      assignExpression();
    }
  }

  /*
   * assignExpression   ::= expression C16 C11 R33 C7 ;
   * */
  void assignExpression() { expression(); }

  /*
   * subscript          ::= simpleExpression C12 C11 R41 ;
   * */
  void subscript() { simpleExpression(); }

  /*
   * expression         ::= simpleExpression optRelation;
   * */
  void expression()
  {
    simpleExpression();
    optRelation();
  }

  /*
   * optRelation        ::=
                   |   EQUAL simpleExpression C14 C11 C11 C10 R21
                   |   BANG_EQUAL simpleExpression C14 C11 C11 C10 R22
                   |   LESS uusimpleExpression C15 C11 C11 C10 R23
                   |   GREATER simpleExpression C15 C11 C11 C10 R25
                   |   GREATER_EQUAL simpleExpression C15 C11 C11 C10 R26
                   |   LESS_EQUAL simpleExpression C15 C11 C11 C10 R24 ;
   * */
  void optRelation()
  {
    if (getCurrTokenType() == EQUAL)
    {
      advance();
      simpleExpression();
    }
    else if (getCurrTokenType() == BANG_EQUAL)
    {
      advance();
      simpleExpression();
    }
    else if (getCurrTokenType() == LESS)
    {
      advance();
      simpleExpression();
    }
    else if (getCurrTokenType() == GREATER)
    {
      advance();
      simpleExpression();
    }
    else if (getCurrTokenType() == GREATER_EQUAL)
    {
      advance();
      simpleExpression();
    }
    else if (getCurrTokenType() == LESS_EQUAL)
    {
      advance();
      simpleExpression();
    }
  }

  /*
   * simpleExpression   ::= term moreTerms
                   |   error moreTerms ;
   * */
  void simpleExpression()
  {
    try
    {
      term();
    }
    catch (const std::runtime_error &)
    {
      m_isError = true;
    }

    moreTerms();
  }

  /*
   * moreTerms          ::=
                   |   PLUS C12 C11 term C12 R14 moreTerms
                   |   MIN C12 C11 term C12 R15 moreTerms
                   |   PIPE C13 C11 term C13 R20 moreTerms
   * */
  void moreTerms()
  {
    if (getCurrTokenType() == PLUS)
    {
      advance();
      term();
      moreTerms();
    }
    else if (getCurrTokenType() == MIN)
    {
      advance();
      term();
      moreTerms();
    }
    else if (getCurrTokenType() == PIPE)
    {
      advance();
      term();
      moreTerms();
    }
  }

  /*
   * term               ::= factor moreFactors ;
   * */
  void term()
  {
    factor();
    moreFactors();
  }

  /*
   * moreFactors        ::=
                   |   STAR C12 C11 factor C12 R16 moreFactors
                   |   SLASH C12 C11 factor C12 R17 moreFactors
                   |   AMPERSAND C13 C11 factor C13 R19 moreFactors ;
   * */
  void moreFactors()
  {
    if (getCurrTokenType() == STAR)
    {
      advance();
      factor();
      moreFactors();
    }
    else if (getCurrTokenType() == SLASH)
    {
      advance();
      factor();
      moreFactors();
    }
    else if (getCurrTokenType() == AMPERSAND)
    {
      advance();
      factor();
      moreFactors();
    }
    // TODO:: implement SLASH (div) operator
  }

  /*
   * factor             ::= primary
                   |   PLUS factor C12
                   |   MIN factor C12 R13
                   |   TILDE factor C13 R18 ;
   * */
  void factor()
  {
    if (getCurrTokenType() == PLUS)
    {
      advance();
      factor();
    }
    else if (getCurrTokenType() == MIN)
    {
      advance();
      factor();
    }
    else if (getCurrTokenType() == TILDE)
    {
      advance();
      factor();
    }
    else
    {
      primary();
    }
  }

  /*
   * primary            ::= NUMBER C9 R36
                   |   TRUE C10 R35
                   |   FALSE C10 R34
                   |   LPAREN expression RPAREN
                   |   LEFT_BRACE C0 R2 declarations C1 R3 SEMI statements SEMI
   expression C2 R6 RIGHT_BRACE |   IDENTIFIER C6 subsOrCall ;
   * */
  void primary()
  {
    if (getCurrTokenType() == NUMBER)
    {
      advance();
    }
    else if (getCurrTokenType() == BOOL)
    {
      if (std::any_cast<bool>(getCurrToken().getLiteral()) == true)
      {
        advance();
      }
      else
      {
        advance();
      }
    }
    else if (getCurrTokenType() == LEFT_PAREN)
    {
      advance();
      expression();

      if (getCurrTokenType() != RIGHT_PAREN)
      {
        exitParse("Expected ')'");
      }

      advance();
    }
    else if (getCurrTokenType() == LEFT_BRACE)
    {
      advance();
      declarations(SEMI);

      if (getCurrTokenType() != SEMI)
      {
        exitParse("Expected ';'");
      }

      advance();
      expression();

      if (getCurrTokenType() != RIGHT_BRACE)
      {
        exitParse("Expected '}'");
      }

      advance();
    }
    else if (getCurrTokenType() == IDENTIFIER)
    {
      advance();
      subsOrCall();
    }
    else
    {
      exitParse("Unknown token");
    }
  }

  /*
   * subsOrCall         ::= R49 C8 R50 C7
                   |   LEFT_PAREN R46 arguments RIGHT_PAREN C8 R47 C7
                   |   LEFT_SQUARE C21 R40 subscript RIGHT_SQUARE C8 R32 C7 ;
   * */
  void subsOrCall()
  {
    if (getCurrTokenType() == LEFT_PAREN)
    {
      advance();
      arguments();

      if (getCurrTokenType() != RIGHT_PAREN)
      {
        exitParse("Expected ')'");
      }

      advance();
    }
    else if (getCurrTokenType() == LEFT_SQUARE)
    {
      advance();
      subscript();

      if (getCurrTokenType() != RIGHT_SQUARE)
      {
        exitParse("Expected ']'");
      }

      advance();
    }
  }

  /*
   * arguments          ::= expression C11 R48 moreArguments ;
   * */
  void arguments()
  {
    expression();
    moreArguments();
  }

  /*
   * moreArguments      ::=
                   |   COMMA expression C11 R48 moreArguments ;
   * */
  void moreArguments()
  {
    if (getCurrTokenType() == COMMA)
    {
      advance();
      expression();
      moreArguments();
    }
  }

  /*
   * declarations       ::= declaration moreDeclarations
                   |   error moreDeclarations;
   * */
  void declarations(TokenType stopToken = TOK_EOF)
  {
    if (isFinished() || getCurrTokenType() == stopToken)
    {
      return;
    }

    try
    {
      declaration();
    }
    catch (const std::runtime_error &)
    {
      m_isError = true;
    }

    moreDeclarations(stopToken);
  }

  /*
   * moreDeclarations   ::=
                   |   declaration moreDeclarations ;
   * */
  void moreDeclarations(TokenType stopToken = TOK_EOF)
  {
    if (isFinished() || getCurrTokenType() == stopToken)
    {
      return;
    }

    declaration();
    moreDeclarations(stopToken);
  }

  /*
   * declaration        ::= VAR IDENTIFIER C3 C4 optArrayBound AS type C5 C11 C7
                   |   type FUNC IDENTIFIER C3 R7 C5 C0 R2 funcBody
                   |   PROC IDENTIFIER C3 R7 C0 R1 procBody ;
   * */
  void declaration()
  {
    if (getCurrTokenType() == VAR)
    {
      advance();

      if (getCurrTokenType() != IDENTIFIER)
      {
        exitParse("Expected identifier");
      }

      advance();
      optArrayBound();

      if (getCurrTokenType() != COLON)
      {
        exitParse("Expected ':'");
      }

      advance();
      type();
    }
    else if (getCurrTokenType() == PROC)
    {
      advance();

      if (getCurrTokenType() != IDENTIFIER)
      {
        exitParse("Expected identifier");
      }

      advance();
      procBody();
    }
    else
    {
      type();

      if (getCurrTokenType() != FUNC)
      {
        exitParse("Expected 'func'");
      }

      advance();

      if (getCurrTokenType() != IDENTIFIER)
      {
        exitParse("Expected identifier");
      }

      advance();
      funcBody();
    }
  }

  /*
   * funcBody           ::= EQ R4 expression R33 C11 C11 C2 R6 R43 R9 C7
                   |   LEFT_PAREN parameters C1 RIGHT_PAREN EQ expression C11
   C11 R6 R43 R9 C7 ;
   * */
  void funcBody()
  {
    if (getCurrTokenType() == EQUAL)
    {
      advance();
      expression();
    }
    else if (getCurrTokenType() == LEFT_PAREN)
    {
      advance();
      parameters();

      if (getCurrTokenType() != RIGHT_PAREN)
      {
        exitParse("Expected ')'");
      }

      advance();

      if (getCurrTokenType() != EQUAL)
      {
        exitParse("Expected '='");
      }

      advance();
      expression();
    }
  }

  /*
   * procBody           ::= scope C2 R5 R42 R9 C7
                   |   LEFT_PAREN parameters C1 RIGHT_PAREN scope R5 R42 R9 C7 ;
   * */
  void procBody()
  {
    if (getCurrTokenType() == LEFT_PAREN)
    {
      advance();
      parameters();

      if (getCurrTokenType() != RIGHT_PAREN)
      {
        exitParse("Expected ')'");
      }

      advance();
      scope();
    }
    else
    {
      scope();
    }
  }

  /*
   * type               ::= INTEGER C9
                   |   BOOLEAN C10;
   * */
  void type()
  {
    if (getCurrTokenType() == INTEGER)
    {
      advance();
    }
    else if (getCurrTokenType() == BOOLEAN)
    {
      advance();
    }
    else
    {
      exitParse("Unknown type");
    }
  }

  /*
   * optArrayBound      ::= C18 R37
                   |   LEFT_SQUARE simpleExpression C12 C11 R39 RIGHT_SQUARE
   C19;
   * */
  void optArrayBound()
  {
    if (getCurrTokenType() == LEFT_SQUARE)
    {
      advance();
      simpleExpression();

      if (getCurrTokenType() == RIGHT_SQUARE)
      {
        advance();
      }
    }
  }

  /*
   * parameters         ::= IDENTIFIER C3 C4 AS type C5 C11 C7 moreParameters ;
   * */
  void parameters()
  {
    if (getCurrTokenType() != IDENTIFIER)
    {
      exitParse("Expected identifier");
    }

    advance();

    if (getCurrTokenType() != COLON)
    {
      exitParse("Expected ':'");
    }

    advance();
    type();
    moreParameters();
  }

  /*
   * moreParameters     ::=
                   |   COMMA IDENTIFIER C3 C4 AS type C5 C11 C7 moreParameters ;
   * */
  void moreParameters()
  {
    if (getCurrTokenType() == COMMA)
    {
      advance();

      if (getCurrTokenType() != IDENTIFIER)
      {
        exitParse("Expected identifier");
      }

      advance();

      if (getCurrTokenType() != COLON)
      {
        exitParse("Expected ':'");
      }

      advance();
      type();
      moreParameters();
    }
  }

  /*
   * outputs            ::= output moreOutput R30 ;
   * */
  void outputs()
  {
    output();
    moreOutput();
  }

  /*
   * output             ::= expression C12 C11 R28
                   |   STRING R29
                   |   SKIP R30 ;
   * */
  void output()
  {
    if (getCurrTokenType() == STRING)
    {
      advance();
    }
    else if (getCurrTokenType() == SKIP)
    {
      advance();
    }
    else
    {
      expression();
    }
  }

  /*
   * moreOutput         ::=
                   |   COMMA output moreOutput ;
   * */
  void moreOutput()
  {
    if (getCurrTokenType() == COMMA)
    {
      advance();
      output();
      moreOutput();
    }
  }

  /*
   * inputs             ::= input moreInputs ;
   * */
  void inputs()
  {
    input();
    moreInputs();
  }

  /*
   * moreInputs         ::=
                   |   COMMA input moreInputs ;
   * */
  void moreInputs()
  {
    if (getCurrTokenType() == COMMA)
    {
      advance();
      input();
      moreInputs();
    }
  }

  /*
   * input              ::= IDENTIFIER C6 optSubscript C17 R27 C7 ;
   * */
  void input()
  {
    if (getCurrTokenType() != IDENTIFIER)
    {
      exitParse("Expected identifier");
    }

    advance();
    optSubscript();
  }

  /*
   * optSubscript       ::= C20 R31
                   |   LEFT_SQUARE C21 R40 subscript RIGHT_SQUARE ;
   * */
  void optSubscript()
  {
    if (getCurrTokenType() == LEFT_SQUARE)
    {
      advance();
      subscript();

      if (getCurrTokenType() != RIGHT_SQUARE)
      {
        exitParse("Expected ']'");
      }

      advance();
    }
  }

 private:
  Lexer m_lexer;
  bool m_isError;
  Semantic m_sema;
};
