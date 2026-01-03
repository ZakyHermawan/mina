#pragma once

#include <any>
#include <iostream>
#include <stdexcept>
#include <string>

enum TokenType
{
  // Single character tokens
  LEFT_BRACE,  // '{'
  RIGHT_BRACE,
  LEFT_PAREN,  // '('
  RIGHT_PAREN,
  LEFT_SQUARE,  // '['
  RIGHT_SQUARE,
  COLON,
  SEMI,  // this can be called as ENDSTMT
  EQUAL,
  HASH,
  LESS,
  GREATER,
  PLUS,
  MIN,
  PIPE,
  STAR,
  SLASH,
  AMPERSAND,
  TILDE,
  COMMA,

  // Multi cahracter tokens
  COLON_EQUAL,  // 20
  LESS_EQUAL,
  GREATER_EQUAL,
  BANG_EQUAL,

  // Literals
  IDENTIFIER,  // 24
  STRING,
  NUMBER,
  BOOL,  // BOOL literal can only be "true" or "false"

  // Keywords
  IF,  // 28
  THEN,
  ELSE,
  END,
  REPEAT,
  UNTIL,
  LOOP,
  EXIT,
  PUT,
  GET,
  VAR,
  FUNC,
  PROC,
  BOOLEAN,
  INTEGER,
  SKIP,
  RETURN,

  TOK_BEGIN,  // just an initial value, not a keyword or anything
  TOK_EOF
};

std::string tokenTypeToString(TokenType type);

class Token
{
private:
	TokenType m_tokenType;
	std::string m_lexme;
	std::any m_literal;
	int m_line;

public:
	// make sure Token initialization is done  with all argument using temporary
	// objects (r-value)
	Token(TokenType type, std::string lexme, std::any literal, int line);
	Token() = default;
	Token(Token &&) = default;
	Token(const Token &) = default;
	Token &operator=(Token &&) = default;
	Token &operator=(const Token &) = default;
	~Token() = default;

	TokenType getTokenType() const;
	std::string getLexme() const;
	std::any getLiteral() const;
	int getLine() const;

	friend std::ostream &operator<<(std::ostream &out, const Token &token);
};
