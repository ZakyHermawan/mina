#pragma once

#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <cassert>
#include <memory>

#include "Lexer.hpp"
#include "vm.hpp"
#include "Symbol.hpp"
#include "arena_alloc.hpp"
#include "Ast.hpp"

class Parser
{
private:
    Lexer m_lexer;
    bool m_isError;
    int m_lexical_level;
    int m_local_numVar;
    bool m_parsing_function;
    std::string m_procName;
    std::string m_funcName;
    int m_arrSize;
    Type m_type; // data type for identifier
    arena::vector<std::string> m_parameters;
    arena::vector<int> m_arguments;
    unsigned int m_ip;  // instruction pointer
    int m_sp;           // stack pointer, always point to the next valid address
    int m_bp;           // base pointer for stack frame
    // arena::unordered_map<std::string, Bucket> m_symTab; // symbol table
    arena::vector<int>
        m_globalDisplay;  // store base pointer address on each stack frame
    arena::vector<arena::unordered_map<std::string, Bucket>> m_symTab;
    arena::vector<arena::unordered_map<std::string, FunctionBucket>>
        m_functionTab;
    arena::vector<int> m_instructions;

 public:
    Parser(std::string source);
    Parser();
    Parser(Parser &&) = default;
    Parser(const Parser &) = default;
    Parser &operator=(Parser &&) = default;
    Parser &operator=(const Parser &) = default;
    ~Parser() = default;
    
    // panic mode
    void exitParse(std::string msg);
    bool isFinished() const;
    Token getCurrToken() const;
    TokenType getCurrTokenType() const;
    unsigned int getCurrLine() const;
    void advance();
    
    std::pair<std::unordered_map<std::string, Bucket>::iterator, int>
    variableDefined(const std::string &varName);
    
    std::pair<std::unordered_map<std::string, Bucket>::iterator, int>
    variableDefinedOnLexicalLevel(const std::string &varName, int lexical_level);
    
    std::pair<std::unordered_map<std::string, Bucket>::iterator, int>
    variableDefinedOnCurrentLexicalLevel(const std::string &varName);
    
    void symbolNotDefined(const std::string &identifier);
    void symbolNotDefinedOnLexicalLevel(const std::string &identifier,
                                        int lexical_level);
    void symbolNotDefinedOnCurrentLexicalLevel(const std::string &identifier);
    void printInstructions();
    
    std::shared_ptr<ProgramAST> program();
    std::shared_ptr<ScopeAST> scope();
    std::shared_ptr<DeclarationsAST> declarations(
        TokenType stopToken = TOK_EOF);
    std::shared_ptr<DeclAST> declaration();
    std::shared_ptr<FuncDeclAST> funcBody();
    std::shared_ptr<ProcDeclAST> procBody();
    void type();
    bool optArrayBound(std::string varName);
    std::shared_ptr<StatementsAST> statements(
        TokenType stopToken = TOK_EOF, TokenType secondStopToken = TOK_EOF);
    std::shared_ptr<StatementAST> statement();
    std::shared_ptr<StatementsAST> optElse();
    std::shared_ptr<StatementAST> assignOrCall(std::string &identifier);
    std::shared_ptr<ExprAST> assignExpression();
    std::shared_ptr<ExprAST> subscript();
    std::shared_ptr<ExprAST> expression();
    std::shared_ptr<ExprAST> optRelation();
    std::shared_ptr<ExprAST> simpleExpression();
    std::shared_ptr<ExprAST> terms();
    std::shared_ptr<ExprAST> term();
    std::shared_ptr<ExprAST> factors();
    std::shared_ptr<ExprAST> factor();
    std::shared_ptr<ExprAST> primary();
    std::shared_ptr<ExprAST> subsOrCall(std::string &identifier);
    std::shared_ptr<ArgumentsAST> arguments();
    std::shared_ptr<ArgumentsAST> moreArguments();

    int evalRPN(const std::string &expr);
    std::string infixToPostfix(const std::string &infix);
    int calculateConstantExpr(std::string &expr);

    int constantsExpression();
    std::shared_ptr<ParametersAST> parameters();
    std::shared_ptr<OutputsAST> outputs();
    std::shared_ptr<ExprAST> output();
    std::shared_ptr<OutputsAST> moreOutput();
    std::shared_ptr<InputsAST> inputs();
    std::shared_ptr<ExprAST> input();
    std::shared_ptr<InputsAST> moreInputs();
    std::shared_ptr<ExprAST> optSubscript();
};
