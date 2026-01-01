#include "tests/test_lexer.hpp"
#include "Token.hpp"
#include "Lexer.hpp"

#include <utility>
#include <cassert>

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
    
    Token la(LEFT_SQUARE, "", "", 5);
    assert(la.getTokenType() == LEFT_SQUARE);
    assert(la.getLine() == 5);
    
    Token ra(RIGHT_SQUARE, "", "", 6);
    assert(ra.getTokenType() == RIGHT_SQUARE);
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
    
    Token t1(NUMBER, "", 1234, 1);
    assert(t1.getTokenType() == TokenType::NUMBER);
    assert(std::any_cast<int>(t1.getLiteral()) == 1234);
    
    Token t2(BOOL, "", true, 3);
    assert(t2.getTokenType() == TokenType::BOOL);
    assert(std::any_cast<bool>(t2.getLiteral()) == true);
    
    Token t3(IF, "if", "", 1);
    assert(t3.getTokenType() == TokenType::IF);
    assert(t3.getLexme() == "if");
    
    Token t4(THEN, "then", "", 1);
    assert(t4.getTokenType() == TokenType::THEN);
    assert(t4.getLexme() == "then");
    
    Token t5(ELSE, "else", "", 1);
    assert(t5.getTokenType() == TokenType::ELSE);
    assert(t5.getLexme() == "else");
    
    Token t6(END, "end", "", 1);
    assert(t6.getTokenType() == TokenType::END);
    assert(t6.getLexme() == "end");
    
    Token t7(REPEAT, "repeat", "", 1);
    assert(t7.getTokenType() == TokenType::REPEAT);
    assert(t7.getLexme() == "repeat");
    
    Token t8(UNTIL, "until", "", 1);
    assert(t8.getTokenType() == TokenType::UNTIL);
    assert(t8.getLexme() == "until");
    
    Token t9(LOOP, "loop", "", 1);
    assert(t9.getTokenType() == TokenType::LOOP);
    assert(t9.getLexme() == "loop");
    
    Token t10(EXIT, "exit", "", 1);
    assert(t10.getTokenType() == TokenType::EXIT);
    assert(t10.getLexme() == "exit");
    
    Token t11(UNTIL, "until", "", 1);
    assert(t11.getTokenType() == TokenType::UNTIL);
    assert(t11.getLexme() == "until");
    
    Token t12(PUT, "put", "", 1);
    assert(t12.getTokenType() == TokenType::PUT);
    assert(t12.getLexme() == "put");
    
    Token t13(GET, "get", "", 1);
    assert(t13.getTokenType() == TokenType::GET);
    assert(t13.getLexme() == "get");
    
    Token t14(VAR, "var", std::string(), 4);
    assert(t14.getTokenType() == TokenType::VAR);
    assert(std::any_cast<std::string>(t14.getLexme()) == std::string("var"));
    
    Token t15(FUNC, "func", "", 1);
    assert(t15.getTokenType() == TokenType::FUNC);
    assert(t15.getLexme() == "func");
    
    Token t16(PROC, "proc", "", 1);
    assert(t16.getTokenType() == TokenType::PROC);
    assert(t16.getLexme() == "proc");
    
    Token t17(BOOLEAN, "boolean", "", 1);
    assert(t17.getTokenType() == TokenType::BOOLEAN);
    assert(t17.getLexme() == "boolean");
    
    Token t18(INTEGER, "integer", "", 1);
    assert(t18.getTokenType() == TokenType::INTEGER);
    assert(t18.getLexme() == "integer");
    
    Token t19(SKIP, "skip", "", 1);
    assert(t19.getTokenType() == TokenType::SKIP);
    assert(t19.getLexme() == "skip");
    
    Token t20(TOK_BEGIN, "", "", 1);
    assert(t20.getTokenType() == TokenType::TOK_BEGIN);
    
    Token t21(TOK_EOF, "", "", 1);
    assert(t21.getTokenType() == TokenType::TOK_EOF);
    
    std::cout << "TESTS TOKEN SUCCESS\n";
}

void tests_lexer()
{
    std::string source = "{}  false !=true myidentifier if then else end repeat until loop exit put get var func proc boolean integer skip return";
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

    lexer.advance();
    assert(lexer.getCurrToken().getTokenType() == IDENTIFIER);

    lexer.advance();
    assert(lexer.getCurrToken().getTokenType() == IF);

    lexer.advance();
    assert(lexer.getCurrToken().getTokenType() == THEN);

    lexer.advance();
    assert(lexer.getCurrToken().getTokenType() == ELSE);

    lexer.advance();
    assert(lexer.getCurrToken().getTokenType() == END);

    lexer.advance();
    assert(lexer.getCurrToken().getTokenType() == REPEAT);

    lexer.advance();
    assert(lexer.getCurrToken().getTokenType() == UNTIL);

    lexer.advance();
    assert(lexer.getCurrToken().getTokenType() == LOOP);

    lexer.advance();
    assert(lexer.getCurrToken().getTokenType() == EXIT);

    lexer.advance();
    assert(lexer.getCurrToken().getTokenType() == PUT);

    lexer.advance();
    assert(lexer.getCurrToken().getTokenType() == GET);

    lexer.advance();
    assert(lexer.getCurrToken().getTokenType() == VAR);

    lexer.advance();
    assert(lexer.getCurrToken().getTokenType() == FUNC);

    lexer.advance();
    assert(lexer.getCurrToken().getTokenType() == PROC);

    lexer.advance();
    assert(lexer.getCurrToken().getTokenType() == BOOLEAN);

    lexer.advance();
    assert(lexer.getCurrToken().getTokenType() == INTEGER);

    lexer.advance();
    assert(lexer.getCurrToken().getTokenType() == SKIP);

    lexer.advance();
    assert(lexer.getCurrToken().getTokenType() == RETURN);

    Lexer l{std::move(std::string())};
    assert(l.getCurrToken().getTokenType() == TOK_BEGIN);
    
    std::cout << "TESTS LEXER SUCCESS\n";
}