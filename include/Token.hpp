#include <any>
#include <string>
#include <iostream>
#include <stdexcept>

enum TokenType {
  // Single character tokens
  LEFT_BRACE, RIGHT_BRACE,
  LEFT_PAREN, RIGHT_PAREN,
  LEFT_ANGLE, RIGHT_ANGLE,
  COLON,
  SEMI,
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
  EQUAL_COLON,
  LESS_EQUAL,
  GREATER_EQUAL,

  // Literals
  IDENTIFIER, STRING, NUMBER, BOOL,
  
  // Keywords
  IF,
  THEN,
  ELSE,
  END,
  REPEAT,
  UNTIL,
  LOOP,
  EXIT,
  PUT,
  GET,
  TRUE,
  FALSE,
  VAR,
  FUNC,
  PROC,
  BOOLEAN,
  INTEGER,
  SKIP,

  _EOF
};

std::string tokenTypeToString(TokenType type) {
  switch (type) {
    case LEFT_BRACE: return "LEFT_BRACE";
    case RIGHT_BRACE: return "RIGHT_BRACE";
    case LEFT_PAREN: return "LEFT_PAREN";
    case RIGHT_PAREN: return "RIGHT_PAREN";
    case LEFT_ANGLE: return "LEFT_ANGLE";
    case RIGHT_ANGLE: return "RIGHT_ANGLE";
    case COLON: return "COLON";
    case SEMI: return "SEMI";
    case EQUAL: return "EQUAL";
    case HASH: return "HASH";
    case LESS: return "LESS";
    case GREATER: return "GREATER";
    case PLUS: return "PLUS";
    case MIN: return "MIN";
    case PIPE: return "PIPE";
    case STAR: return "STAR";
    case SLASH: return "SLASH";
    case AMPERSAND: return "AMPERSAND";
    case TILDE: return "TILDE";
    case COMMA: return "COMMA";
    case EQUAL_COLON: return "EQUAL_COLON";
    case LESS_EQUAL: return "LESS_EQUAL";
    case GREATER_EQUAL: return "GREATER_EQUAL";
    case IDENTIFIER: return "IDENTIFIER";
    case STRING: return "STRING";
    case NUMBER: return "NUMBER";
    case BOOL: return "BOOL";
    case IF: return "IF";
    case THEN: return "THEN";
    case ELSE: return "ELSE";
    case END: return "END";
    case REPEAT: return "REPEAT";
    case UNTIL: return "UNTIL";
    case LOOP: return "LOOP";
    case EXIT: return "EXIT";
    case PUT: return "PUT";
    case GET: return "GET";
    case TRUE: return "TRUE";
    case FALSE: return "FALSE";
    case VAR: return "VAR";
    case FUNC: return "FUNC";
    case PROC: return "PROC";
    case BOOLEAN: return "BOOLEAN";
    case INTEGER: return "INTEGER";
    case SKIP: return "SKIP";
    case _EOF: return "EOF";
    default: return "UNKNOWN";
  }
}

class Token {
private:
  TokenType m_tokenType;
  std::string m_lexme;
  std::any m_literal;
  int m_line;

public:
  Token(TokenType type, const std::string& lexme, const std::any& literal, int line)
    : m_tokenType{type}, m_lexme{lexme}, m_literal{literal}, m_line{line}
  {}
  Token(Token &&) = default;
  Token(const Token &) = default;
  Token &operator=(Token &&) = default;
  Token &operator=(const Token &) = default;
  ~Token() = default;

  TokenType getTokenType() const
  {
    return m_tokenType;
  }

  std::string getLexme() const
  {
    return m_lexme;
  }

  std::any getLiteral() const
  {
    return m_literal;
  }

  friend std::ostream& operator<< (std::ostream& out, const Token& token)
  {
    out << tokenTypeToString(token.m_tokenType) << " " << token.m_lexme << " ";
    if(token.m_literal.type() == typeid(int))
    {
      out << std::any_cast<int>(token.m_literal);
    }
    else if (token.m_literal.type() == typeid(bool)) {
      out << std::any_cast<bool>(token.m_literal);
    }
    else if(token.m_literal.type() == typeid(std::string))
    {
      out << std::any_cast<std::string>(token.m_literal);
    }
    else {
      throw std::runtime_error("Literal value is not printable\n");
    }
    return out;
  }
};

