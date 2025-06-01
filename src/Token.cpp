#include "Token.hpp"

std::string tokenTypeToString(TokenType type)
{
    switch (type)
    {
        case LEFT_BRACE:
            return "LEFT_BRACE";
        case RIGHT_BRACE:
            return "RIGHT_BRACE";
        case LEFT_PAREN:
            return "LEFT_PAREN";
        case RIGHT_PAREN:
            return "RIGHT_PAREN";
        case LEFT_SQUARE:
            return "LEFT_SQUARE";
        case RIGHT_SQUARE:
            return "RIGHT_SQURE";
        case COLON:
            return "COLON";
        case SEMI:
            return "SEMI";
        case EQUAL:
            return "EQUAL";
        case HASH:
            return "HASH";
        case LESS:
            return "LESS";
        case GREATER:
            return "GREATER";
        case PLUS:
            return "PLUS";
        case MIN:
            return "MIN";
        case PIPE:
            return "PIPE";
        case STAR:
            return "STAR";
        case SLASH:
            return "SLASH";
        case AMPERSAND:
            return "AMPERSAND";
        case TILDE:
            return "TILDE";
        case COMMA:
            return "COMMA";
        case COLON_EQUAL:
            return "COLON_EQUAL";
        case LESS_EQUAL:
            return "LESS_EQUAL";
        case GREATER_EQUAL:
            return "GREATER_EQUAL";
        case BANG_EQUAL:
            return "BANG_EQUAL";
        case IDENTIFIER:
            return "IDENTIFIER";
        case STRING:
            return "STRING";
        case NUMBER:
            return "NUMBER";
        case BOOL:
            return "BOOL";
        case IF:
            return "IF";
        case THEN:
            return "THEN";
        case ELSE:
            return "ELSE";
        case END:
            return "END";
        case REPEAT:
            return "REPEAT";
        case UNTIL:
            return "UNTIL";
        case LOOP:
            return "LOOP";
        case EXIT:
            return "EXIT";
        case PUT:
            return "PUT";
        case GET:
            return "GET";
        case VAR:
            return "VAR";
        case FUNC:
            return "FUNC";
        case PROC:
            return "PROC";
        case BOOLEAN:
            return "BOOLEAN";
        case INTEGER:
            return "INTEGER";
        case SKIP:
            return "SKIP";
        case RETURN:
            return "RETURN";
        case TOK_BEGIN:
            return "TOK_BEGIN";
        case TOK_EOF:
            return "TOK_EOF";
        default:
            return "UNKNOWN";
    }
}

Token::Token(TokenType type, std::string lexme, std::any literal, int line)
    : m_tokenType{type},
      m_lexme{std::move(lexme)},
      m_literal{std::move(literal)},
      m_line{line}
{
}

TokenType Token::getTokenType() const { return m_tokenType; }
std::string Token::getLexme() const { return m_lexme; }
std::any Token::getLiteral() const { return m_literal; }
int Token::getLine() const { return m_line; }

std::ostream &operator<<(std::ostream &out, const Token &token)
{
    out << tokenTypeToString(token.m_tokenType) << " " << token.m_lexme << " ";

    if (token.m_literal.type() == typeid(int))
    {
        out << std::any_cast<int>(token.m_literal);
    }
    else if (token.m_literal.type() == typeid(bool))
    {
        out << std::any_cast<bool>(token.m_literal);
    }
    else if (token.m_literal.type() == typeid(std::string))
    {
        out << std::any_cast<std::string>(token.m_literal);
    }
    else
    {
        std::cout << token.m_literal.type().name() << std::endl;
        throw std::runtime_error("Literal value is not printable\n");
    }

    return out;
}
