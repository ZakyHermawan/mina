#pragma once

#include "Ast.hpp"
#include "SSA.hpp"
#include "CodeGen.hpp"
#include "Visitors.hpp"
#include "BasicBlock.hpp"
#include "InstIR.hpp"

#include <stack>
#include <memory>
#include <unordered_map>


class IRVisitor : public Visitor
{
private:
    int m_tempCounter;
    int m_labelCounter;
    std::vector<std::shared_ptr<Inst>> m_arguments;
    std::vector<std::string> m_argNames;
    std::vector<std::shared_ptr<IdentifierAST>> m_parameters;
    std::stack<std::string> m_temp;
    std::stack<std::shared_ptr<Inst>> m_instStack;
    std::stack<std::string> m_labels;

    SSA m_ssa;
    CodeGen m_cg;

    std::shared_ptr<BasicBlock> m_currentBB; // current basic block
    std::unordered_map<std::string, std::shared_ptr<Func>> m_funcBB;

public:
    IRVisitor();
    void visit(StatementsAST& v) override;
    void visit(NumberAST& v) override;
    void visit(BoolAST& v) override;
    void visit(StringAST& v) override;
    void visit(VariableAST& v) override;
    void visit(ProgramAST& v) override;
    void visit(ScopeAST& v) override;
    void visit(ScopedExprAST& v) override;
    void visit(AssignmentAST& v) override;
    void visit(OutputAST& v) override;
    void visit(OutputsAST& v) override;
    void visit(InputAST& v) override;
    void visit(InputsAST& v) override;
    void visit(IfAST& v) override;
    void visit(RepeatUntilAST& v) override;
    void visit(LoopAST& v) override;
    void visit(ExitAST& v) override;
    void visit(ReturnAST& v) override;
    void visit(ParameterAST& v) override;
    void visit(ParametersAST& v) override;
    void visit(ArrAccessAST& v) override;
    void visit(ArgumentsAST& v) override;
    void visit(CallAST& v) override;
    void visit(FactorAST& v) override;
    void visit(FactorsAST& v) override;
    void visit(TermAST& v) override;
    void visit(SimpleExprAST& v) override;
    void visit(TermsAST& v) override;
    void visit(OptRelationAST& v) override;
    void visit(ExpressionAST& v) override;
    void visit(VarDeclAST& v) override;
    void visit(ArrDeclAST& v) override;
    void visit(DeclarationsAST& v) override;
    void visit(ProcDeclAST& v) override;
    void visit(FuncDeclAST& v) override;

    std::string getCurrentTemp() const;
    void pushCurrentTemp();
    std::string popTemp();
    std::string getLastTemp() const;

    std::shared_ptr<Inst> popInst();
};
