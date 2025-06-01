#pragma once

#include <memory>
#include <vector>
#include <iostream>

class StatementsAST;
class NumberAST;
class BoolAST;
class StringAST;
class IdentifierAST;
class ProgramAST;
class ScopeAST;
class ScopedExprAST;
class AssignmentAST;
class OutputAST;
class OutputsAST;
class InputAST;
class InputsAST;
class IfAST;
class RepeatUntilAST;
class LoopAST;
class ExitAST;
class ReturnAST;
class ParameterAST;
class ParametersAST;
class ArrAccessAST;
class ArgumentsAST;
class CallAST;
class FactorAST;
class FactorsAST;
class TermAST;
class TermsAST;
class SimpleExprAST;
class OptRelationAST;
class ExpressionAST;
class VarDeclAST;
class ArrDeclAST;
class DeclarationsAST;
class ProcDeclAST;
class FuncDeclAST;

class Visitor
{
public:
    virtual ~Visitor() = default;
    virtual void visit(StatementsAST&) = 0;
    virtual void visit(NumberAST&) = 0;
    virtual void visit(BoolAST&) = 0;
    virtual void visit(StringAST&) = 0;
    virtual void visit(IdentifierAST&) = 0;
    virtual void visit(ProgramAST&) = 0;
    virtual void visit(ScopeAST&) = 0;
    virtual void visit(ScopedExprAST&) = 0;
    virtual void visit(AssignmentAST&) = 0;
    virtual void visit(OutputAST&) = 0;
    virtual void visit(OutputsAST&) = 0;
    virtual void visit(InputAST&) = 0;
    virtual void visit(InputsAST&) = 0;
    virtual void visit(IfAST&) = 0;
    virtual void visit(RepeatUntilAST&) = 0;
    virtual void visit(LoopAST&) = 0;
    virtual void visit(ExitAST&) = 0;
    virtual void visit(ReturnAST&) = 0;
    virtual void visit(ParameterAST&) = 0;
    virtual void visit(ParametersAST&) = 0;
    virtual void visit(ArrAccessAST&) = 0;
    virtual void visit(ArgumentsAST&) = 0;
    virtual void visit(CallAST&) = 0;
    virtual void visit(FactorAST&) = 0;
    virtual void visit(FactorsAST&) = 0;
    virtual void visit(TermAST&) = 0;
    virtual void visit(TermsAST&) = 0;
    virtual void visit(SimpleExprAST&) = 0;
    virtual void visit(OptRelationAST&) = 0;
    virtual void visit(ExpressionAST&) = 0;
    virtual void visit(VarDeclAST&) = 0;
    virtual void visit(ArrDeclAST&) = 0;
    virtual void visit(DeclarationsAST&) = 0;
    virtual void visit(ProcDeclAST&) = 0;
    virtual void visit(FuncDeclAST&) = 0;
};
