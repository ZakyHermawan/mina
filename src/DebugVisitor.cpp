#include "DebugVisitor.hpp"
#include "Ast.hpp"

void DebugVisitor::visit(StatementsAST& v) { std::cout << "Statements AST\n"; }
void DebugVisitor::visit(NumberAST& v)
{
  std::cout << "Number: " << v.getVal();
}

void DebugVisitor::visit(BoolAST& v)
{
    std::cout << "Bool: " << v.getVal();
}
void DebugVisitor::visit(StringAST& v) { std::cout << "String AST: " << v.getVal(); }

void DebugVisitor::visit(IdentifierAST& v)
{
  std::cout << "Identifier: " << v.getName() << ":" << typeToStr(v.getType()) << std::endl;
}

void DebugVisitor::visit(ProgramAST& v) {std::cout << "Program AST\n"; }
void DebugVisitor::visit(ScopeAST& v) { std::cout << "Scope AST\n"; }
void DebugVisitor::visit(ScopedExprAST& v) { std::cout << "Scoped Expression AST\n"; }
void DebugVisitor::visit(AssignmentAST& v) { std::cout << "Assignment AST\n"; }
void DebugVisitor::visit(OutputAST& v) { std::cout << "Output AST\n"; }
void DebugVisitor::visit(OutputsAST& v) { std::cout << "Outputs AST\n"; }
void DebugVisitor::visit(InputAST& v) { std::cout << "Input AST\n"; }
void DebugVisitor::visit(InputsAST& v) { std::cout << "Inputs AST\n"; }
void DebugVisitor::visit(IfAST& v) { std::cout << "If AST\n"; }
void DebugVisitor::visit(RepeatUntilAST& v) { std::cout << "Repeat Until AST\n"; }
void DebugVisitor::visit(LoopAST& v) { std::cout << "Loop AST\n"; }

void DebugVisitor::visit(ParameterAST& v)
{
  std::cout << "Parameter " << v.getIdentifier()->getName() << " : "
            << typeToStr(v.getType()) << '\n';
}
void DebugVisitor::visit(ArrAccessAST& v) { std::cout << "Array access node"; }
void DebugVisitor::visit(ArgumentsAST& v) { std::cout << "Argument node"; }

void DebugVisitor::visit(CallAST& v) { std::cout << "Calling function " << v.getFuncName() << '\n'; }
void DebugVisitor::visit(FactorAST& v) { std::cout << "Factor AST " << '\n'; }
void DebugVisitor::visit(FactorsAST& v) { std::cout << "Factors AST " << '\n'; }
void DebugVisitor::visit(TermAST& v) { std::cout << "Term AST " << '\n'; }
void DebugVisitor::visit(TermsAST& v) { std::cout << "Terms AST " << '\n'; }
void DebugVisitor::visit(SimpleExprAST& v) { std::cout << "SimpleExpr AST " << '\n'; }
void DebugVisitor::visit(OptRelationAST& v) { std::cout << "OptRelation AST " << '\n'; }
void DebugVisitor::visit(ExpressionAST& v) { std::cout << "Expression AST " << '\n'; }

void DebugVisitor::visit(VarDeclAST& v)
{
  std::cout << "Var: " << v.getIdentifier()->getName() << " : "
            << typeToStr(v.getIdentifier()->getType()) << '\n';
}

void DebugVisitor::visit(ArrDeclAST& v)
{
  std::cout << "Array: " << v.getIdentifier()->getName() << ": "
            << typeToStr(v.getIdentifier()->getType()) << '\n';
}

void DebugVisitor::visit(DeclarationsAST&) { std::cout << "Declarations node\n"; }
void DebugVisitor::visit(ParametersAST&) { std::cout << "Parameters node\n"; }
void DebugVisitor::visit(ProcDeclAST&) { std::cout << "Procedure Declaration\n"; }
void DebugVisitor::visit(FuncDeclAST&) { std::cout << "Function Declaration\n"; }
