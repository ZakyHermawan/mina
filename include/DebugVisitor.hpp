#pragma once

#include "Visitors.hpp"

class DebugVisitor : public Visitor
{
public:
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
    void visit(ArrDeclAST&) override;
    void visit(DeclarationsAST&) override;
};
