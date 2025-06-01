#include "Lexer.hpp"

Lexer::Lexer(std::string source)
    : m_source{std::move(source)},
      m_currChar{' '},
      m_currLine{1},
      m_currIdx{0},
      m_currToken{Token(TOK_BEGIN, std::string(), std::string(), 1)}
{
    if (m_source.length() == 0)
    {
        return;
    }
    m_source += '\n';
    advance();
}
void Lexer::read_file(std::string source)
{
    m_source = std::move(source);
    advance();
}

bool Lexer::isFinished() const
{
    if (getCurrTokenType() == TOK_EOF)
    {
        return true;
    }

    return false;
}

Token Lexer::getCurrToken() const { return m_currToken; }
TokenType Lexer::getCurrTokenType() const { return m_currToken.getTokenType(); }
unsigned int Lexer::getCurrLine() const { return m_currLine; }

void Lexer::skipWhitespace()
{
    while (isspace(m_currChar))
    {
        if (m_currIdx >= m_source.length())
        {
            m_currToken = Token(TOK_EOF, std::string(), std::string(), m_currLine);
            return;
        }

        m_currChar = m_source[m_currIdx];
        if (m_currChar == '\n')
        {
            ++m_currLine;
        }
        ++m_currIdx;
    }
}

int Lexer::scanInt()
{
    int result = 0;
    while (isdigit(m_currChar))
    {
        result = result * 10 + m_currChar - '0';
        if (m_currIdx >= m_source.length())
        {
            break;
        }
        m_currChar = m_source[m_currIdx++];
    }

    // because m_currIdx always point to the next character,
    // currently m_currIdx pointing to the next next character
    // since m_currChar is not digit
    --m_currIdx;
    return result;
}

void Lexer::advance()
{
    // skip whitespace
    skipWhitespace();

    // currently, m_currIdx pointing to the next char instead of curr char
    if (m_currIdx > m_source.length())
    {
        m_currToken = Token(TOK_EOF, std::string(), std::string(), m_currLine);
    }

    if (m_currToken.getTokenType() == TOK_EOF)
    {
        return;
    }

    if (isdigit(m_currChar))
    {
        int numVal = scanInt();
        m_currToken = Token(NUMBER, std::string(), numVal, m_currLine);
    }
    else if (isalpha(m_currChar))
    {
        std::string identifier;

        // since we already get the first character from the last loop, add it to
        // the identifier
        identifier += m_currChar;
        while (m_currIdx < m_source.length() && isalnum(m_source[m_currIdx]))
        {
            m_currChar = m_source[m_currIdx++];
            identifier += m_currChar;
        }

        if (identifier == "true")
        {
            m_currToken = Token(BOOL, std::string(), true, m_currIdx);
        }
        else if (identifier == "false")
        {
            m_currToken = Token(BOOL, std::string(), false, m_currIdx);
        }
        else if (identifier == "if")
        {
            m_currToken = Token(IF, std::move(identifier), std::string(), m_currLine);
        }
        else if (identifier == "then")
        {
            m_currToken =
                Token(THEN, std::move(identifier), std::string(), m_currLine);
        }
        else if (identifier == "else")
        {
        m_currToken =
            Token(ELSE, std::move(identifier), std::string(), m_currLine);
        }
        else if (identifier == "end")
        {
        m_currToken =
            Token(END, std::move(identifier), std::string(), m_currLine);
        }
        else if (identifier == "repeat")
        {
        m_currToken =
            Token(REPEAT, std::move(identifier), std::string(), m_currLine);
        }
        else if (identifier == "until")
        {
            m_currToken = 
                Token(UNTIL, std::move(identifier), std::string(), m_currLine);
        }
        else if (identifier == "loop")
        {
            m_currToken =
                Token(LOOP, std::move(identifier), std::string(), m_currLine);
        }
        else if (identifier == "exit")
        {
            m_currToken =
                Token(EXIT, std::move(identifier), std::string(), m_currLine);
        }
        else if (identifier == "put")
        {
        m_currToken =
            Token(PUT, std::move(identifier), std::string(), m_currLine);
        }
        else if (identifier == "get")
        {
        m_currToken =
            Token(GET, std::move(identifier), std::string(), m_currLine);
        }
        else if (identifier == "var")
        {
            m_currToken =
            Token(VAR, std::move(identifier), std::string(), m_currLine);
        }
        else if (identifier == "func")
        {
            m_currToken =
                Token(FUNC, std::move(identifier), std::string(), m_currLine);
        }
        else if (identifier == "proc")
        {
            m_currToken =
                Token(PROC, std::move(identifier), std::string(), m_currLine);
        }
        else if (identifier == "boolean")
        {
            m_currToken =
                Token(BOOLEAN, std::move(identifier), std::string(), m_currLine);
        }
        else if (identifier == "integer")
        {
            m_currToken =
                Token(INTEGER, std::move(identifier), std::string(), m_currLine);
        }
        else if (identifier == "skip")
        {
            m_currToken =
                Token(SKIP, std::move(identifier), std::string(), m_currLine);
        }
        else if (identifier == "return")
        {
            m_currToken =
                Token(RETURN, std::move(identifier), std::string(), m_currLine);
        }
        else
        {
            m_currToken =
                Token(IDENTIFIER, std::move(identifier), std::string(), m_currLine);
        }
    }
    else if (m_currChar == '"')
    {
        std::string raw_str;
        while (1)
        {
            if (m_currIdx >= m_source.length())
            {
                std::string errMsg = "Error, at line " + std::to_string(m_currLine) +
                                 "\nExpected '\"' at the end of string literal";
                throw std::runtime_error(errMsg);
            }

            if (m_source[m_currIdx] == '"')
            {
                break;
            }
            raw_str += m_source[m_currIdx++];
        }
        m_currToken = Token(STRING, std::string(), raw_str, m_currLine);
        ++m_currIdx;
    }
    else if (m_currChar == '{')
    {
        m_currToken = Token(LEFT_BRACE, std::string("{"), std::string(), m_currLine);
    }
    else if (m_currChar == '}')
    {
        m_currToken = Token(RIGHT_BRACE, std::string("}"), std::string(), m_currLine);
    }
    else if (m_currChar == '(')
    {
        m_currToken = Token(LEFT_PAREN, std::string("("), std::string(), m_currLine);
    }
    else if (m_currChar == ')')
    {
        m_currToken = Token(RIGHT_PAREN, std::string(")"), std::string(), m_currLine);
    }
    else if (m_currChar == '[')
    {
        m_currToken = Token(LEFT_SQUARE, std::string("["), std::string(), m_currLine);
    }
    else if (m_currChar == ']')
    {
        m_currToken = Token(RIGHT_SQUARE, std::string("]"), std::string(), m_currLine);
    }
    else if (m_currChar == ':')
    {
        if (m_currIdx < m_source.length() && m_source[m_currIdx] == '=')
        {
            m_currToken =
                Token(COLON_EQUAL, std::string(":="), std::string(), m_currLine);
            ++m_currIdx;
        }
        else
        {
            m_currToken = Token(COLON, std::string(":"), std::string(), m_currLine);
        }
    }
    else if (m_currChar == ';')
    {
      m_currToken = Token(SEMI, std::string(";"), std::string(), m_currLine);
    }
    else if (m_currChar == '=')
    {
        m_currToken = Token(EQUAL, std::string("="), std::string(), m_currLine);
    }
    else if (m_currChar == '#')
    {
        m_currToken = Token(HASH, std::string("#"), std::string(), m_currLine);
    }
    else if (m_currChar == '<')
    {
            if (m_currIdx < m_source.length() && m_source[m_currIdx] == '=')
            {
                m_currToken =
                    Token(LESS_EQUAL, std::string("<="), std::string(), m_currLine);
                ++m_currIdx;
            }
        else
        {
            m_currToken = Token(LESS, std::string("<"), std::string(), m_currLine);
        }
    }
    else if (m_currChar == '>')
    {
        if (m_currIdx < m_source.length() && m_source[m_currIdx] == '=')
        {
            m_currToken =
              Token(GREATER_EQUAL, std::string(">="), std::string(), m_currLine);
            ++m_currIdx;
        }
        else
        {
            m_currToken =
              Token(GREATER, std::string(">"), std::string(), m_currLine);
        }
    }
    else if (m_currChar == '!')
    {
        if (m_currIdx < m_source.length() && m_source[m_currIdx] == '=')
        {
          m_currToken =
              Token(BANG_EQUAL, std::string("!="), std::string(), m_currLine);
            ++m_currIdx;
        }
        else
        {
            std::string errMsg = "Error, at line " + std::to_string(m_currLine) +
                               "\nExpected '=' after '!'";
            throw std::runtime_error(errMsg);
        }
    }
    else if (m_currChar == '+')
    {
      m_currToken = Token(PLUS, std::string("+"), std::string(), m_currLine);
    }
    else if (m_currChar == '-')
    {  
        m_currToken = Token(MIN, std::string("-"), std::string(), m_currLine);
    }
    else if (m_currChar == '|')
    {
        m_currToken = Token(PIPE, std::string("|"), std::string(), m_currLine);
    }
    else if (m_currChar == '*')
    {
        m_currToken = Token(STAR, std::string("*"), std::string(), m_currLine);
    }
    else if (m_currChar == '/')
    {
        m_currToken = Token(SLASH, std::string("/"), std::string(), m_currLine);
    }
    else if (m_currChar == '&')
    {
        m_currToken = Token(AMPERSAND, std::string("&"), std::string(), m_currLine);
    }
    else if (m_currChar == '~')
    {
        m_currToken = Token(TILDE, std::string("~"), std::string(), m_currLine);
    }
    else if (m_currChar == ',')
    {
        m_currToken = Token(COMMA, std::string(","), std::string(), m_currLine);
    }
    else
    {
        std::string errMsg = "Error, at line " + std::to_string(m_currLine) +
                           "\nUnknown char: " + std::to_string(m_currChar);
        throw std::runtime_error(errMsg);
    }

    if (m_currIdx >= m_source.length())
    {
        m_currToken = Token(TOK_EOF, std::string(), std::string(), m_currLine);
        return;
    }
    m_currChar = m_source[m_currIdx++];
}
