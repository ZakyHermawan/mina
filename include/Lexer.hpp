#pragma once

#include <cctype>
#include <memory>
#include <stdexcept>

#include "Token.hpp"

class Lexer
{
private:
	std::string m_source;
	Token m_currToken;
	char m_currChar;
	unsigned int m_currLine;
	unsigned int m_currIdx;  // always point to the next character

public:
	Lexer(std::string source);
	Lexer() = default;
	Lexer(Lexer &&) = default;
	Lexer(const Lexer &) = default;
	Lexer &operator=(Lexer &&) = default;
	Lexer &operator=(const Lexer &) = default;
	~Lexer() = default;

	void read_file(std::string source);
	bool isFinished() const;
	Token getCurrToken() const;
	TokenType getCurrTokenType() const;
	unsigned int getCurrLine() const;
    std::string &getSource();
    unsigned int getCurrIdx() const;
	void skipWhitespace();
	int scanInt();
	void advance();
};
