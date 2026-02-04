#include "Ast.hpp"
#include "Token.hpp"
#include "Types.hpp"
#include "Parser.hpp"
#include "Symbol.hpp"
#include "IRVisitor.hpp"
#include "arena_alloc.hpp"

#include <any>
#include <stack>
#include <cctype>
#include <memory>
#include <string>
#include <cstdlib>
#include <sstream>
#include <utility>
#include <iostream>
#include <stdexcept>
#include <unordered_map>

Parser::Parser(std::string source)
    : m_lexer{std::move(source)},
      m_isError{false},
      m_lexical_level{-1},
      m_arrSize{0},
      m_parsing_function{0},
      m_local_numVar{0},
      m_procName{""},
      m_funcName{""},
      m_type{Type::UNDEFINED}
{
}

Parser::Parser()
      : m_lexer{},
      m_isError{false},
      m_lexical_level{-1},
      m_arrSize{0},
      m_parsing_function{0},
      m_local_numVar{0},
      m_procName{""},
      m_funcName{""},
      m_type{Type::UNDEFINED}
{
}

void Parser::exitParse(std::string msg) const
{
    std::cerr << "Error" << " : " << msg << ", got "
        << getCurrToken() << std::endl;
    exit(1);
}

bool Parser::isFinished() const { return m_lexer.isFinished(); }
Token Parser::getCurrToken() const { return m_lexer.getCurrToken(); }
TokenType Parser::getCurrTokenType() const { return getCurrToken().getTokenType(); }
unsigned int Parser::getCurrLine() const { return m_lexer.getCurrLine(); }
void Parser::advance() { m_lexer.advance(); }

// Return the iterator if a variable defined
// Throw error if the variable is defined
std::pair<std::unordered_map<std::string, Bucket>::iterator, int> Parser::variableDefined(const std::string &varName)
{
    for (int i = m_lexical_level; i >= 0; --i)
    {  // reference, no copy
        auto it = m_symTab[i].find(varName);
        if (it != m_symTab[i].end()) return {it, i};  // iterator into live map
    }
    throw std::runtime_error("symbol '" + varName + "' does not exist!");
}

// Return the iterator if a variable defined
// Throw error if the variable is defined
std::pair<std::unordered_map<std::string, Bucket>::iterator, int> Parser::variableDefinedOnLexicalLevel(const std::string &varName, int lexical_level)
{
    // Iterator, no copy
    auto currSymTabIt = m_symTab.begin() + lexical_level;
    auto it = currSymTabIt->find(varName);
    if (it != currSymTabIt->end())
        return {it, lexical_level};  // iterator into live map
    throw std::runtime_error("symbol '" + varName + "' does not exist!");
}

std::pair<std::unordered_map<std::string, Bucket>::iterator, int> Parser::variableDefinedOnCurrentLexicalLevel(const std::string &varName)
{
    return variableDefinedOnLexicalLevel(varName, m_lexical_level);
}

  // Throw if symbol is defined
void Parser::symbolNotDefined(const std::string &identifier)
{
    for (int i = 0; i < m_symTab.size(); ++i)
    {
        auto& currSymTab = m_symTab[i];
        auto it = currSymTab.find(identifier);
        if (it != currSymTab.end())
        {
            throw std::runtime_error("symbol " + identifier + " does exist!");
        }
    }

    for (int i = 0; i < m_functionTab.size(); ++i)
    {
        auto& currFuncTab = m_functionTab[i];
        auto it = currFuncTab.find(identifier);
        if (it != currFuncTab.end())
        {
            throw std::runtime_error("symbol " + identifier + " does exist!");
        }
    }
}

// Throw if symbol on lexical level defined
void Parser::symbolNotDefinedOnLexicalLevel(const std::string &identifier,
                                    int lexical_level)
{
    if (m_symTab.size() == 0)
    {
        return;
    }

    auto& currSymTab = m_symTab[lexical_level];
    auto it = currSymTab.find(identifier);
    if (it != currSymTab.end())
    {
        throw std::runtime_error("symbol " + identifier + " does exist!");
    }

    auto& currFuncTab = m_functionTab[lexical_level];
    auto it2 = currFuncTab.find(identifier);
    if (it2 != currFuncTab.end())
    {
        throw std::runtime_error("symbol " + identifier + " does exist!");
    }
}

void Parser::symbolNotDefinedOnCurrentLexicalLevel(const std::string &identifier)
{
    symbolNotDefinedOnLexicalLevel(identifier, m_lexical_level);
}

Type Parser::getTypeFromSymTab(std::string& identifier)
{
    arena::unordered_map < std::string, Bucket > symTab;
    for (int i = m_lexical_level; i >= 0; --i)
    {
        auto& currSymTab = m_symTab[i];
        auto it = currSymTab .find(identifier);
        if (it != currSymTab.end())
        {
            symTab = currSymTab;
            break;
        }
    }

    if (symTab.find(identifier) == symTab.end())
    {
        throw std::runtime_error("identifier " + identifier + " not found!");
    }
    auto type = symTab[identifier].getType();
    return type;
}

/**
 * At the end of every parsing functions,
 * if there is no error,
 * always make sure the current token is pointing to the next token (use
 * advance)
 * */

/*
 * program            ::= scope ;
 * */
std::shared_ptr<ProgramAST> Parser::program()
{
    m_symTab.push_back(arena::unordered_map<std::string, Bucket>());
    m_functionTab.push_back(
        arena::unordered_map<std::string, FunctionBucket>());

    auto scopeAST = scope();
    auto programAST = std::make_shared<ProgramAST>(std::move(scopeAST));

    IRVisitor dv;
    programAST->accept(dv);
    m_symTab.pop_back();
    m_functionTab.pop_back();

    return programAST;
}

  /*
 * scope              ::= LEFT_BRACE declarations SEMI statements RIGHT_BRACE
                      |   LEFT_BRACE SEMI statements RIGHT_BRACE ;
 * */
std::shared_ptr<ScopeAST> Parser::scope()
{
    ++m_lexical_level;
    if (getCurrTokenType() != LEFT_BRACE)
    {
        exitParse("Expected '{'");
    }

    advance();

    // Parse: LEFT_BRACE SEMI statements RIGHT_BRACE
    if (getCurrTokenType() == SEMI)
    {
        advance();
        auto statementsAST = statements(RIGHT_BRACE);

        if (getCurrTokenType() != RIGHT_BRACE)
        {
            exitParse("Expected '}'");
        }

        advance();
        --m_lexical_level;
        return std::make_shared<ScopeAST>(nullptr, statementsAST);
    }

    auto decls = declarations(SEMI);

    if (getCurrTokenType() != SEMI)
    {
        exitParse("Expected ';'");
    }

    advance();
    auto statementsAST = statements(RIGHT_BRACE);

    if (getCurrTokenType() != RIGHT_BRACE)
    {
        exitParse("Expected '}'");
    }

    advance();

    --m_lexical_level;
    return std::make_shared<ScopeAST>(std::move(decls), std::move(statementsAST));
}

/*
 * declarations       ::=
                        | declaration declarations
 * */
std::shared_ptr<DeclarationsAST> Parser::declarations(TokenType stopToken)
{
    if (isFinished() || getCurrTokenType() == stopToken)
    {
        return nullptr;
    }

    auto decl = declaration();
    auto decls = declarations(stopToken);
    return std::make_shared<DeclarationsAST>(decl, decls);
}

/*
* declaration        ::= VAR IDENTIFIER optArrayBound COLON type
               |   type FUNC IDENTIFIER funcBody
               |   PROC IDENTIFIER procBody ;
* */
std::shared_ptr<DeclAST> Parser::declaration()
{
    if (getCurrTokenType() == VAR)
    {
        advance();
        
        if (getCurrTokenType() != IDENTIFIER)
        {
            exitParse("Expected identifier");
        }
        
        auto currToken = getCurrToken();
        auto varName = currToken.getLexme();

        advance();
        auto isArrDecl = optArrayBound(varName);
        
        if (getCurrTokenType() != COLON)
        {
            exitParse("Expected ':'");
        }
        
        advance();
        type();
        auto identifierAST =
            std::make_shared<VariableAST>(varName, m_type, IdentType::VARIABLE);

        std::string theName;
        if (m_procName != "")
        {
            theName = m_procName;
        }
        else
        {
            theName = m_funcName;
        }

        if (isArrDecl)
        {
            if (m_parsing_function)
            {
                m_functionTab[m_lexical_level][theName].setSymTab(
                    varName, Bucket(arenaVectorInt(m_arrSize), m_local_numVar, m_type));
            }
            m_symTab[m_lexical_level][varName] =
                Bucket(arenaVectorInt(m_arrSize), 0, m_type);
            return std::make_shared<ArrDeclAST>(identifierAST, m_arrSize);
        }

        m_symTab[m_lexical_level][varName] = Bucket(0, 0, m_type);
        if (m_parsing_function)
        {
            m_functionTab[m_lexical_level][theName].setSymTab(
                varName, Bucket(0, m_local_numVar++, m_type));
            auto decl = std::make_shared<VarDeclAST>(identifierAST, m_type);

            return decl;
        }

        auto decl =
            std::make_shared<VarDeclAST>(identifierAST, m_type);
        return decl;
    }
    else if (getCurrTokenType() == PROC)
    {
        advance();
        
        if (getCurrTokenType() != IDENTIFIER)
        {
            exitParse("Expected identifier");
        }
        
        auto currToken = getCurrToken();
        auto procName = currToken.getLexme();
        
        advance();
        
        m_procName = procName;
        m_parsing_function = true;
        auto procDecl = procBody();

        auto& funcTab = m_functionTab[m_lexical_level][m_procName];
        m_procName = "";
        m_parsing_function = false;

        return procDecl;
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

        auto currToken = getCurrToken();
        auto funcName = currToken.getLexme();

        advance();        
        m_funcName = funcName;
        m_parsing_function = true;
        auto funcDecl = funcBody();
        
        auto& funcTab = m_functionTab[m_lexical_level][m_funcName];
        m_funcName = "";
        m_parsing_function = false;

        return funcDecl;
    }
}

/*
* allow func funcame and func procName()
* funcBody           ::= scope
               |   LEFT_PAREN parameters RIGHT_PAREN EQ scope ;
* */
std::shared_ptr<FuncDeclAST> Parser::funcBody()
{
    m_local_numVar = 0;
    m_parameters.clear();
    m_parameterTypes.clear();
    symbolNotDefinedOnCurrentLexicalLevel(m_funcName);

    // New symbol table for procedure
    // Later we use + 1 for declaration parameters, because we want to keep it
    // inside procedure scope
    m_symTab.push_back(arena::unordered_map<std::string, Bucket>());
    m_functionTab.push_back(
        arena::unordered_map<std::string, FunctionBucket>());
    
    std::shared_ptr<ParametersAST> paramsAST = nullptr;
    std::shared_ptr<ScopeAST> scopeAST = nullptr;
    
    if (getCurrTokenType() == LEFT_PAREN)
    {
        advance();

        if (getCurrTokenType() == COMMA)
        {
            exitParse("Parameters should not started with','");
        }

        paramsAST = parameters();
        m_local_numVar = m_parameters.size();

        if (getCurrTokenType() != RIGHT_PAREN)
        {
            exitParse("Expected ')'");
        }
        
        advance();

        m_functionTab[(size_t)m_lexical_level + 1][m_funcName] =
            FunctionBucket(m_parameters);
        
        for (int i = 0; i < m_parameters.size(); ++i)
        {
            m_functionTab[(size_t)m_lexical_level + 1][m_funcName].setSymTab(
                m_parameters[i], Bucket(0, i, m_parameterTypes[i]));
        }
        scopeAST = scope();
    }
    else
    {
        arena::vector<std::string> tmp;
        m_functionTab[(size_t)m_lexical_level + 1][m_funcName] =
            FunctionBucket(tmp);        
        scopeAST = scope();
    }

    m_functionTab[(size_t)m_lexical_level + 1][m_funcName].setLocalNumVar(
        m_local_numVar);
    auto funcDecl = std::make_shared<FuncDeclAST>(m_funcName, paramsAST,
                                                  std::move(scopeAST), m_type);

    m_symTab.pop_back();
    m_functionTab.pop_back();

    return funcDecl;
}

/*
* allow proc procName and proc procName()
* procBody           ::= scope
               |   LEFT_PAREN parameters RIGHT_PAREN scope ;
* */
std::shared_ptr<ProcDeclAST> Parser::procBody()
{
    m_local_numVar = 0;
    m_parameters.clear();
    m_parameterTypes.clear();
    symbolNotDefinedOnCurrentLexicalLevel(m_procName);

    // New symbol table for procedure
    // Later we use + 1 for declaration parameters, because we want to keep it inside
    // procedure scope
    m_symTab.push_back(arena::unordered_map<std::string, Bucket>());
    m_functionTab.push_back(
        arena::unordered_map<std::string, FunctionBucket>());


    std::shared_ptr<ParametersAST> paramsAST = nullptr;
    std::shared_ptr<ScopeAST> scopeAST = nullptr;

    if (getCurrTokenType() == LEFT_PAREN)
    {
        advance();
        if (getCurrTokenType() == COMMA)
        {
            exitParse("Parameters should not started with','");
        }
        paramsAST = parameters();
        m_local_numVar = m_parameters.size();
        
        if (getCurrTokenType() != RIGHT_PAREN)
        {
            exitParse("Expected ')'");
        }

        advance();        
        m_functionTab[(size_t)m_lexical_level + 1][m_procName] = FunctionBucket(m_parameters);
        
        for (int i = 0; i < m_parameters.size(); ++i)
        {
            m_functionTab[(size_t)m_lexical_level+ 1][m_procName].setSymTab(
                m_parameters[i], Bucket(0, i, m_parameterTypes[i]));
        }
        scopeAST = scope();
    }
    else
    {
        arena::vector<std::string> tmp;
        m_functionTab[(size_t)m_lexical_level + 1][m_procName] =
            FunctionBucket(tmp);
        scopeAST = scope();
    }

    m_functionTab[(size_t)m_lexical_level + 1][m_procName].setLocalNumVar(
        m_local_numVar);
    
    auto procDecl = std::make_shared<ProcDeclAST>(
        m_procName, paramsAST, scopeAST);

    m_symTab.pop_back();
    m_functionTab.pop_back();

    return procDecl;
}

/*
* type               ::= INTEGER
               |   BOOLEAN ;
* */
void Parser::type()
{
    m_type = Type::UNDEFINED;
    if (getCurrTokenType() == INTEGER)
    {
        m_type = Type::INTEGER;
        advance();
    }
    else if (getCurrTokenType() == BOOLEAN)
    {
        m_type = Type::BOOLEAN;
        advance();
    }
    else
    {
        exitParse("Unknown type");
    }
}

/*
 * optArrayBound      ::=
                 |   LEFT_SQUARE constantsExpression RIGHT_SQUARE ;
 * */
bool Parser::optArrayBound(std::string varName)
{
    if (getCurrTokenType() == LEFT_SQUARE)
    {
        advance();
        m_arrSize = constantsExpression();
        
        if (getCurrTokenType() != RIGHT_SQUARE)
        {
            exitParse("Expected ']'");
        }
        
        if (m_parsing_function)
        {        
            m_local_numVar += m_arrSize;
            return true;
        }
        
        // allocating array
        // store the first stack address for the array
        auto& source = m_lexer.getSource();
        auto currIdx = m_lexer.getCurrIdx();

        advance();
        return true;
    }
    return false;
}

/*
   * statements         ::=
                   |   statement statements ;
   * */
std::shared_ptr<StatementsAST> Parser::statements(TokenType stopToken,
                TokenType secondStopToken)
{
    if (isFinished())
    {
        return nullptr;
    }

    if (getCurrTokenType() == stopToken || getCurrTokenType() == secondStopToken)
    {
        return nullptr;
    }

    auto statementAST = statement();
    auto statementsAST = statements(stopToken, secondStopToken);
    return std::make_shared<StatementsAST>(std::move(statementAST), std::move(statementsAST));
}

/*
 * statement          ::= IDENTIFIER assignOrCall
                 |   IF expression THEN statements optElse END IF
                 |   REPEAT statements UNTIL expression
                 |   LOOP statements END LOOP |  EXIT
                 |   PUT LEFT_PAREN outputs RIGHT_PAREN
                 |   GET LEFT_PAREN inputs RIGHT_PAREN
                 |   RETURN expresssion
                 |   scope ;
 * */
std::shared_ptr<StatementAST> Parser::statement()
{
    if (getCurrTokenType() == IDENTIFIER)
    {
        auto identifier = getCurrToken().getLexme();
        advance();
        auto assignOrCallAST = assignOrCall(identifier);
        return assignOrCallAST;
    }
    else if (getCurrTokenType() == IF)
    {
        advance();
        auto conditionAST = expression();

        if (getCurrTokenType() != THEN)
        {
            exitParse("Expected 'then' after if expression");
        }

        advance();
        auto thenAST = statements(ELSE, END);
        auto elseAST = optElse();

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
        return std::make_shared<IfAST>(conditionAST, thenAST, elseAST);
    }
    else if (getCurrTokenType() == REPEAT)
    {
        advance();
        auto statementsAST = statements(UNTIL);

        if (getCurrTokenType() != UNTIL)
        {
            exitParse("Expected 'until'");
        }

        advance();
        auto exitConditionAST = expression();
        return std::make_shared<RepeatUntilAST>(statementsAST,
                                                exitConditionAST);
    }
    else if (getCurrTokenType() == LOOP)
    {
        advance();
        auto statementsAST = statements(END);

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
        return std::make_shared<LoopAST>(statementsAST);
    }
    else if (getCurrTokenType() == EXIT)
    {
        advance();
        return nullptr;
    }
    else if (getCurrTokenType() == PUT)
    {

        advance();
        if (getCurrTokenType() != LEFT_PAREN)
        {
            exitParse("expected '('");
        }
        advance();

        auto outputsAST = outputs();

        if (getCurrTokenType() != RIGHT_PAREN)
        {
            exitParse("expected ')'");
        }
        advance();

        return outputsAST;
    }
    else if (getCurrTokenType() == GET)
    {
        advance();
        if (getCurrTokenType() != LEFT_PAREN)
        {
            exitParse("expected '('");
        }
        advance();

        auto inputsAST = inputs();

        if (getCurrTokenType() != RIGHT_PAREN)
        {
            exitParse("expected ')'");
        }
        advance();

        return inputsAST;
    }
    else if (getCurrTokenType() == RETURN)
    {
        if (!m_parsing_function)
        {
            exitParse("return statement only exist in a function");
        }
        advance();
        auto exprAST = expression();
        return std::make_shared<ReturnAST>(exprAST);
    }
    else
    {
        m_symTab.push_back(arena::unordered_map<std::string, Bucket>());
        m_functionTab.push_back(
            arena::unordered_map<std::string, FunctionBucket>());

        auto scopeAST = scope();
        m_symTab.pop_back();
        m_functionTab.pop_back();

        return scopeAST;
    }
}

  /*
 * optElse            ::=
                 |   ELSE statements ;
 * Because else statement will always end with END statement,
 * put END as stopToken for statements
 * */
std::shared_ptr<StatementsAST> Parser::optElse()
{
    if (getCurrTokenType() == ELSE)
    {
        advance();
        return statements(END);
    }
    return nullptr;
}

/*
 * assignOrCall       ::=
                 |   LEFT_PAREN arguments RIGHT_PAREN
                 |   COLON_EQUAL assignExpression
                 |   LEFT_SQUARE subscript RIGHT_SQUARE EQUAL assignExpression ;
* */
std::shared_ptr<StatementAST> Parser::assignOrCall(std::string &identifier)
{
    if (getCurrTokenType() == LEFT_PAREN)
    {
        advance();
        m_arguments.clear();
        auto argumentsAST = arguments();

        if (getCurrTokenType() != RIGHT_PAREN)
        {
            exitParse("Expected ')'");
        }

        advance();

        int startAddr = -1;
        int lexical_level;
        for (int i = m_lexical_level; i >= 0; --i)
        {
          auto& currFuncTab = m_functionTab[i];
          auto it = currFuncTab.find(identifier);
            if (it != currFuncTab.end())
            {
                startAddr = (it->second).getStartAddr();
                lexical_level = i;
            }
        }

        if (startAddr != -1)
        {
            return std::make_shared<CallAST>(identifier, argumentsAST);
        }
        else
        {
          throw std::runtime_error("function or procedure " + identifier +
                                   " is not defined!");
        }
    }
    else if (getCurrTokenType() == COLON_EQUAL)
    {
        advance();
        auto leftAST = std::make_shared<VariableAST>(
            identifier, getTypeFromSymTab(identifier), IdentType::VARIABLE);
        auto exprAST = assignExpression();

        if (m_parsing_function)
        {
            return std::make_shared<AssignmentAST>(leftAST, exprAST);
        }
        else
        {
            auto _ = variableDefined(identifier);
            return std::make_shared<AssignmentAST>(leftAST, exprAST);
        }
    }
    else if (getCurrTokenType() == LEFT_SQUARE)
    {
        advance();
        auto subscriptAST = subscript();
        auto x = getTypeFromSymTab(identifier);
        auto arrAccessAST = std::make_shared<ArrAccessAST>(
            identifier, getTypeFromSymTab(identifier), IdentType::ARRAY, subscriptAST);
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
        auto rightAST = assignExpression();
        return std::make_shared<AssignmentAST>(arrAccessAST, rightAST);
    }
    else
    {
        // function/procedure call without argument
        int startAddr = -1;
        int lexical_level;
        for (int i = m_lexical_level; i >= 0; --i)
        {
            auto& currFuncTab = m_functionTab[i];
            auto it = currFuncTab.find(identifier);
            if (it != currFuncTab.end())
            {
                startAddr = (it->second).getStartAddr();
                lexical_level = i;
            }
        }

        if (startAddr != -1)
        {
            return std::make_shared<CallAST>(identifier, nullptr);
        }
        else
        {
            throw std::runtime_error("function or procedure " + identifier +
                               " is not defined!");
        }
    }
    return nullptr;
}

  /*
 * assignExpression   ::= expression ;
 * */
std::shared_ptr<ExprAST> Parser::assignExpression() { return expression(); }

/*
 * subscript          ::= simpleExpression ;
 * */
std::shared_ptr<ExprAST> Parser::subscript() { return simpleExpression(); }

/*
 * expression         ::= simpleExpression optRelation;
 * */
std::shared_ptr<ExprAST> Parser::expression()
{
    auto termsAST = simpleExpression();
    if(termsAST == nullptr) return nullptr;
    auto optRelationAST = optRelation();
    return std::make_shared<ExpressionAST>(termsAST, optRelationAST);
}

  /*
 * optRelation        ::=
                 |   EQUAL simpleExpression
                 |   BANG_EQUAL simpleExpression
                 |   LESS simpleExpression
                 |   GREATER simpleExpression
                 |   GREATER_EQUAL simpleExpression
                 |   LESS_EQUAL simpleExpression ;
 * */

// EQUAL ('=') token is used to check for equality (same as "==" token in C++)
std::shared_ptr<ExprAST> Parser::optRelation()
{
    if (getCurrTokenType() == EQUAL)
    {
        auto op = getCurrToken();
        advance();
        auto terms = simpleExpression();
        return std::make_shared<OptRelationAST>(op, terms);
    }
    else if (getCurrTokenType() == BANG_EQUAL)
    {
        auto op = getCurrToken();
        advance();
        auto terms = simpleExpression();
        return std::make_shared<OptRelationAST>(op, terms);
    }
    else if (getCurrTokenType() == LESS)
    {
        auto op = getCurrToken();
        advance();
        auto terms = simpleExpression();
        return std::make_shared<OptRelationAST>(op, terms);
    }
    else if (getCurrTokenType() == GREATER)
    {
        auto op = getCurrToken();
        advance();
        auto terms = simpleExpression();
        return std::make_shared<OptRelationAST>(op, terms);
    }
    else if (getCurrTokenType() == GREATER_EQUAL)
    {
        auto op = getCurrToken();
        advance();
        auto terms = simpleExpression();
        return std::make_shared<OptRelationAST>(op, terms);
    }
    else if (getCurrTokenType() == LESS_EQUAL)
    {
        auto op = getCurrToken();
        advance();
        auto terms = simpleExpression();
        return std::make_shared<OptRelationAST>(op, terms);
    }
    return nullptr;
}

  /*
 * simpleExpression   ::= term terms
 * */
std::shared_ptr<ExprAST> Parser::simpleExpression()
{
    auto termAST = term();
    if(termAST == nullptr) return nullptr;
    auto termsAST = terms();
    return std::make_shared<SimpleExprAST>(termAST, termsAST);
}

/*
 * terms          ::=
                 |   PLUS term terms
                 |   MIN term terms
                 |   PIPE term terms
 * */
// PIPE '|' token is used for logical or operation (same as "||" token in C++)
std::shared_ptr<ExprAST> Parser::terms()
{
    if (getCurrTokenType() == PLUS)
    {
        auto op = getCurrToken();
        advance();
        auto termAST = term();
        auto termsAST = terms();
        return std::make_shared<TermsAST>(op, termAST, termsAST);
    }
    else if (getCurrTokenType() == MIN)
    {
        auto op = getCurrToken();
        advance();
        auto termAST = term();
        auto termsAST = terms();
        return std::make_shared<TermsAST>(op, termAST, termsAST);
    }
    else if (getCurrTokenType() == PIPE)
    {
        auto op = getCurrToken();
        advance();
        auto termAST = term();
        auto termsAST = terms();
        return std::make_shared<TermsAST>(op, termAST, termsAST);
    }
    return nullptr;
}

/*
 * term               ::= factor factors ;
 * */
std::shared_ptr<ExprAST> Parser::term()
{
    auto factorAST = factor();
    if (factorAST == nullptr) return nullptr;
    auto factorsAST = factors();
    return std::make_shared<TermAST>(factorAST, factorsAST);
}

  /*
 * factors        ::=
                 |   STAR factor factors
                 |   SLASH factor factors
                 |   AMPERSAND factor factors ;
 * */
// Slash '/' token is used for integer division
// Ampersand '&' token is used for logical and operation (same as "&&" token in
// C++)
std::shared_ptr<ExprAST> Parser::factors()
{
    if (getCurrTokenType() == STAR)
    {
        auto op = getCurrToken();
        advance();
        auto factorAST = factor();
        auto factorsAST = factors();
        return std::make_shared<FactorsAST>(op, factorAST, factorsAST);
    }
    else if (getCurrTokenType() == SLASH)
    {
        auto op = getCurrToken();
        advance();
        auto factorAST = factor();
        auto factorsAST = factors();
        return std::make_shared<FactorsAST>(op, factorAST, factorsAST);
    }
    else if (getCurrTokenType() == AMPERSAND)
    {
        auto op = getCurrToken();
        advance();
        auto factorAST = factor();
        auto factorsAST = factors();
        return std::make_shared<FactorsAST>(op, factorAST, factorsAST);
    }
    return nullptr;
}

  /*
 * factor             ::= primary
                 |   PLUS factor
                 |   MIN factor
                 |   TILDE factor ;
 * */
// TILDE '~' token is used for logical not operation (same as '!' token in C++)
std::shared_ptr<ExprAST> Parser::factor()
{
    if (getCurrTokenType() == PLUS)
    {
        auto op = getCurrToken();
        advance();
        auto factorAST = factor();
        return std::make_shared<FactorAST>(op, factorAST);
    }
    else if (getCurrTokenType() == MIN)
    {
        auto op = getCurrToken();
        advance();
        auto factorAST = factor();
        return std::make_shared<FactorAST>(op, factorAST);
    }
    else if (getCurrTokenType() == TILDE)
    {
        auto op = getCurrToken();
        advance();
        auto factorAST = factor();
        return std::make_shared<FactorAST>(op, factorAST);
    }
    else
    {
        return primary();
    }
}

  /*
 * primary            ::= NUMBER
                 |   TRUE
                 |   FALSE
                 |   LPAREN expression RPAREN
                 |   LEFT_BRACE declarations SEMI statements SEMI expression
 RIGHT_BRACE |   IDENTIFIER subsOrCall ;
 * */
std::shared_ptr<ExprAST> Parser::primary()
{
    if (getCurrTokenType() == NUMBER)
    {
        int num = std::any_cast<int>(getCurrToken().getLiteral());
        advance();
        return std::make_shared<NumberAST>(num);
    }
    else if (getCurrTokenType() == BOOL)
    {
        if (std::any_cast<bool>(getCurrToken().getLiteral()) == true)
        {
            advance();
            return std::make_shared<BoolAST>(true);
        }
        else
        {
            advance();
            return std::make_shared<BoolAST>(false);
        }
    }
    else if (getCurrTokenType() == LEFT_PAREN)
    {
        advance();
        auto exprAST = expression();

        if (getCurrTokenType() != RIGHT_PAREN)
        {
          exitParse("Expected ')'");
        }

        advance();
        return exprAST;
    }
    else if (getCurrTokenType() == LEFT_BRACE)
    {
        advance();
        auto declsAST = declarations(SEMI);

        if (getCurrTokenType() != SEMI)
        {
            exitParse("Expected ';'");
        }
        advance();
        auto stmts = statements(SEMI);
        advance();
        auto exprAST = expression();

        if (getCurrTokenType() != RIGHT_BRACE)
        {
          exitParse("Expected '}'");
        }

        advance();
        return std::make_shared<ScopedExprAST>(declsAST, stmts, exprAST);
    }
    else if (getCurrTokenType() == IDENTIFIER)
    {
        auto varName = getCurrToken().getLexme();
        advance();
        return subsOrCall(varName);  // check is the variable defined or not is done here
    }
    else
    {
        //exitParse("Unknown token");
        return nullptr;
    }
}

  /*
 * subsOrCall         ::=
                 |   LEFT_PAREN arguments RIGHT_PAREN
                 |   LEFT_SQUARE subscript RIGHT_SQUARE ;
 * */
// will emit / push smething to the stack, this is derived from primary
// Expression (not a statement)
std::shared_ptr<ExprAST> Parser::subsOrCall(std::string &identifier)
{
    if (getCurrTokenType() == LEFT_PAREN)
    {
        advance();
        auto argumentsAST = arguments();
        if (argumentsAST == nullptr)
        {
            std::cout << "nullarg\n";
        }

        if (getCurrTokenType() != RIGHT_PAREN)
        {
          exitParse("Expected ')'");
        }

        advance();
        return std::make_shared<CallAST>(identifier, argumentsAST);
    }
    else if (getCurrTokenType() == LEFT_SQUARE)
    {
        advance();

        if (m_parsing_function)
        {
            auto addr = m_functionTab[m_lexical_level][m_procName]
                          .getSymTab(identifier)
                          .getStackAddr();
        }
        else
        {
            auto _ = variableDefined(identifier);
        }

        auto subsExprAST = subscript();

        if (getCurrTokenType() != RIGHT_SQUARE)
        {
            exitParse("Expected ']'");
        }
        advance();
        return std::make_shared<ArrAccessAST>(identifier, getTypeFromSymTab(identifier),
                                              IdentType::ARRAY, subsExprAST);
    }
    else
    {
        int startAddr = -1;
        for (int i = m_lexical_level; i >= 0; --i)
        {
            auto& currFuncTab = m_functionTab[i];
            auto it = currFuncTab.find(identifier);
            if (it != currFuncTab.end())
            {
                startAddr = (it->second).getStartAddr();
            }
        }

        if (startAddr != -1)
        {
            throw std::runtime_error("Calling function or procedure "
                                     " without parentheses is not allowed!");
        }

        // just an identifier
        if (m_parsing_function)
        {
            std::string theName;
            if (m_procName != "")
            {
                theName = m_procName;
            }
            else
            {
                theName = m_funcName;
            }
            auto addr = m_functionTab[m_lexical_level][theName]
                          .getSymTab(identifier)
                          .getStackAddr();
            auto& bucket =
                m_functionTab[m_lexical_level][theName].getSymTab(
                    identifier);
            return std::make_shared<VariableAST>(identifier, bucket.getType(),
                                                 IdentType::VARIABLE);
        }
        else
        {
            auto _ = variableDefined(identifier);
        }
        return std::make_shared<VariableAST>(identifier, getTypeFromSymTab(identifier),
                                             IdentType::VARIABLE);
    }
}

  /*
 * arguments          ::= expression moreArguments ;
 * */
std::shared_ptr<ArgumentsAST> Parser::arguments()
{
    auto exprAST = expression();
    if (exprAST == nullptr) return nullptr;
    m_arguments.push_back(exprAST);
    
    auto argumentsAST = moreArguments();
    return std::make_shared<ArgumentsAST>(exprAST, argumentsAST);
}

/*
 * moreArguments      ::=
                 |   COMMA expression moreArguments ;
 * */
std::shared_ptr<ArgumentsAST> Parser::moreArguments()
{
    if (getCurrTokenType() == COMMA)
    {
        advance();
        auto exprAST = expression();
        auto argumentsAST = moreArguments();
        return std::make_shared<ArgumentsAST>(exprAST, argumentsAST);
    }
    return nullptr;
}

int Parser::evalRPN(const std::string &expr)
{
    std::istringstream iss(expr);
    std::stack<int> stack;
    std::string token;
    
    while (iss >> token)
    {
        if (std::isdigit(token[0]) ||
            (token.size() > 1 && token[0] == '-' && std::isdigit(token[1])))
        {
            stack.push(std::stoi(token));
        }
        else
        {
            int b = stack.top();
            stack.pop();
            int a = stack.top();
            stack.pop();

            if (token == "+")
                stack.push(a + b);
            else if (token == "-")
                stack.push(a - b);
            else if (token == "*")
                stack.push(a * b);
            else if (token == "/")
                stack.push(a / b);
        }
    }
    
    return stack.top();
}

std::string Parser::infixToPostfix(const std::string &infix)
{
    std::istringstream iss(infix);
    std::stack<std::string> ops;
    std::ostringstream out;
    std::string token;
    
    auto prec = [](const std::string &op)
    {
        if (op == "+" || op == "-") return 1;
        if (op == "*" || op == "/") return 2;
        return 0;
    };
    
    while (iss >> token)
    {
        if (std::isdigit(token[0]))
        {
            out << token << " ";
        }
        else if (token == "(")
        {
            ops.push(token);
        }
        else if (token == ")")
        {
            while (!ops.empty() && ops.top() != "(")
            {
                out << ops.top() << " ";
                ops.pop();
            }
            ops.pop();  // pop '('
        }
        else
        {  // operator
            while (!ops.empty() && prec(ops.top()) >= prec(token))
            {
                out << ops.top() << " ";
                ops.pop();
            }
            ops.push(token);
        }
    }
    
    while (!ops.empty())
    {
        out << ops.top() << " ";
        ops.pop();
    }
    
    return out.str();
}

int Parser::calculateConstantExpr(std::string &expr)
{
    auto postfixExpr = infixToPostfix(expr);
    return evalRPN(postfixExpr);
}

/*
 * constantsExpression is just arithmethics expressions while all of the
 * operands are constants
 * */
int Parser::constantsExpression()
{
    std::string expr;
    while (1)
    {
        if (getCurrTokenType() == NUMBER)
        {
            int intVal = std::any_cast<int>(getCurrToken().getLiteral());
            expr += std::to_string(intVal);
        }
        else if (getCurrTokenType() == STAR)
        {
            expr += " * ";
        }
        else if (getCurrTokenType() == SLASH)
        {
            expr += " / ";
        }
        else if (getCurrTokenType() == PLUS)
        {
            expr += " + ";
        }
        else if (getCurrTokenType() == MIN)
        {
            expr += " - ";
        }
        else if (getCurrTokenType() == RIGHT_SQUARE)
        {
            break;
        }
        else
        {
            exitParse("Expected arithmethic expression");
        }
        advance();
    }
    
    return calculateConstantExpr(expr);
}

/*
 * parameters         ::= 
                      |  IDENTIFIER COLON type moreParameters ;
 *                    |  COMMA IDENTIFIER COLON type moreParameters
 * */
std::shared_ptr<ParametersAST> Parser::parameters()
{
    if (getCurrTokenType() == IDENTIFIER)
    {
    }
    else if (getCurrTokenType() == COMMA)
    {
        advance();
        if (getCurrTokenType() != IDENTIFIER)
        {
            exitParse("Expected identifier");
        }
    }
    else
    {
        return nullptr;
    }
    auto identifier = m_lexer.getCurrToken().getLexme();
    m_parameters.push_back(identifier);

    std::string theName;
    if (m_procName != "")
    {
        theName = m_procName;
    }
    else
    {
        theName = m_funcName;
    }
    if (m_parsing_function)
    {
        m_functionTab[m_lexical_level][theName].setSymTab(
            identifier, Bucket(0, m_local_numVar++, m_type));
        m_symTab[m_lexical_level][identifier] = Bucket(0, 0, m_type);
    }
    advance();
    
    if (getCurrTokenType() != COLON)
    {
        exitParse("Expected ':'");
    }
    
    advance();
    type();
    m_parameterTypes.push_back(m_type);
    auto identifierAST =
        std::make_shared<VariableAST>(identifier, m_type, IdentType::VARIABLE);
    auto paramAST =
        std::make_shared<ParameterAST>(identifierAST, m_type);
    auto moreParamAST = parameters();
    return std::make_shared<ParametersAST>(std::move(paramAST),
                                              std::move(moreParamAST));
}

  /*
 * outputs            ::= output moreOutput ;
 * */
std::shared_ptr<OutputsAST> Parser::outputs()
{
    auto outputAST = output();
    auto outputsAST = moreOutput();
    return std::make_shared<OutputsAST>(outputAST, outputsAST);
}

/*
 * output             ::= expression
                 |   STRING
                 |   SKIP ;
 * */
std::shared_ptr<ExprAST> Parser::output()
{
    if (getCurrTokenType() == STRING)
    {
        auto stringLiteral =
            std::any_cast<std::string>(getCurrToken().getLiteral());
        for (unsigned int i = 0; i < stringLiteral.length(); ++i)
        {
        }
        
        advance();
        return std::make_shared<StringAST>(stringLiteral);
    }
    else if (getCurrTokenType() == SKIP)
    {
        advance();
        return std::make_shared<StringAST>("\n");
    }
    else
    {
        auto exprAST = expression();
        return exprAST;
    }
}

/*
 * moreOutput         ::=
                 |   COMMA output moreOutput ;
 * */
std::shared_ptr<OutputsAST> Parser::moreOutput()
{
    if (getCurrTokenType() == COMMA)
    {
        advance();
        auto outputAST = output();
        auto outputsAST = moreOutput();
        return std::make_shared<OutputsAST>(outputAST, outputsAST);
    }
    return nullptr;
}

/*
 * inputs             ::= input moreInputs ;
 * */
std::shared_ptr<InputsAST> Parser::inputs()
{
    auto inputAST = input();
    auto inputsAST = moreInputs();
    return std::make_shared<InputsAST>(inputAST, inputsAST);
}

/*
 * moreInputs         ::=
                 |   COMMA input moreInputs ;
 * */
std::shared_ptr<InputsAST> Parser::moreInputs()
{
    if (getCurrTokenType() == COMMA)
    {
        advance();
        auto inputAST = input();
        auto inputsAST = moreInputs();
        return std::make_shared<InputsAST>(inputAST, inputsAST);
    }
    return nullptr;
}

/*
 * input              ::= IDENTIFIER optSubscript ;
 * */
std::shared_ptr<InputAST> Parser::input()
{
    if (getCurrTokenType() != IDENTIFIER)
    {
        exitParse("Expected identifier");
    }
    auto varName = getCurrToken().getLexme();
    auto [it, lexical_level] = variableDefined(varName);
    auto identifierAST = std::make_shared<VariableAST>(
        varName, getTypeFromSymTab(varName), IdentType::VARIABLE);

    advance();
    auto subsExpr = optSubscript();
    if (subsExpr)
    {
        auto x = std::make_shared<ArrAccessAST>(varName, getTypeFromSymTab(varName),
                                              IdentType::ARRAY, subsExpr);
        auto inputAST = std::make_shared<InputAST>(x);
        return inputAST;
    }
    return std::make_shared<InputAST>(identifierAST);
}

/*
 * optSubscript       ::=
                 |   LEFT_SQUARE subscript RIGHT_SQUARE ;
 * */
std::shared_ptr<ExprAST> Parser::optSubscript()
{
    if (getCurrTokenType() == LEFT_SQUARE)
    {
        advance();
        auto subsAST = subscript();
        
        if (getCurrTokenType() != RIGHT_SQUARE)
        {
            exitParse("Expected ']'");
        }
        
        advance();
        return subsAST;
    }
    return nullptr;
}
