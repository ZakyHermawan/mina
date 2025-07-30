#include "Ast.hpp"

StatementsAST::StatementsAST(std::shared_ptr<StatementAST> statement,
                             std::shared_ptr<StatementsAST> statements)
    : m_statement(std::move(statement)), m_statements(std::move(statements))
{
}
std::shared_ptr<StatementAST> StatementsAST::getStatement(){ return m_statement; }
std::shared_ptr<StatementsAST> StatementsAST::getStatements() { return m_statements; }
void StatementsAST::accept(Visitor& v)
{ 
    v.visit(*this);
}

NumberAST::NumberAST(int val) : m_val{ val }
{
}
int NumberAST::getVal() const { return m_val; }
void NumberAST::accept(Visitor& v) { v.visit(*this); }

BoolAST::BoolAST(bool val) : m_val{val} {}
bool BoolAST::getVal() const { return m_val; }
void BoolAST::accept(Visitor& v) { v.visit(*this); }

StringAST::StringAST(std::string str) : m_str{str} {}
std::string StringAST::getVal() const { return m_str; }
void StringAST::accept(Visitor& v) { v.visit(*this); }

VariableAST::VariableAST(std::string name, Type type, IdentType identType)
    : m_name(std::move(name)), m_type(type), m_identType(identType)
{
}
void VariableAST::accept(Visitor& v) { v.visit(*this); }
std::string& VariableAST::getName() { return m_name; }
Type VariableAST::getType() const { return m_type; }
IdentType VariableAST::getIdentType() const { return m_identType; }

ArrAccessAST::ArrAccessAST(std::string name, Type type, IdentType identType,
                           std::shared_ptr<ExprAST> subscript)
    : m_name(std::move(name)),
      m_type(type),
      m_identType(identType),
      m_subsExpr(std::move(subscript))
{
}
std::string& ArrAccessAST::getName() { return m_name; }
Type ArrAccessAST::getType() const { return m_type; }
IdentType ArrAccessAST::getIdentType() const { return m_identType; }

std::shared_ptr<ExprAST> ArrAccessAST::getSubsExpr() {
    return m_subsExpr;
}
void ArrAccessAST::accept(Visitor& v)
{
    v.visit(*this);
}

ArgumentsAST::ArgumentsAST(std::shared_ptr<ExprAST> expr,
                           std::shared_ptr<ArgumentsAST> arguments)
    : m_expr(std::move(expr)), m_arguments(std::move(arguments))
{
}
std::shared_ptr<ExprAST> ArgumentsAST::getExpr() { return m_expr; }
std::shared_ptr<ArgumentsAST> ArgumentsAST::getArgs() { return m_arguments; }
void ArgumentsAST::accept(Visitor& v) {
    v.visit(*this);
}

CallAST::CallAST(std::string funcName,
                 std::shared_ptr<ArgumentsAST> arguments)
    : m_funcName(std::move(funcName)), m_arguments(std::move(arguments))
{
}
std::string CallAST::getFuncName() const { return m_funcName; }
std::shared_ptr<ArgumentsAST> CallAST::getArgs() { return m_arguments; }
  void CallAST::accept(Visitor & v) {
    v.visit(*this);
}

FactorAST::FactorAST(Token op, std::shared_ptr<ExprAST> factor)
    : m_op(op), m_factor(std::move(factor))
{
}
Token FactorAST::getOp() const { return m_op; }
std::shared_ptr<ExprAST> FactorAST::getFactor() { return m_factor; }
void FactorAST::accept(Visitor& v)
{
    v.visit(*this);
}

FactorsAST::FactorsAST(Token op, std::shared_ptr<ExprAST> factor,
                       std::shared_ptr<ExprAST> factors)
    : m_op(op), m_factor(std::move(factor)), m_factors(std::move(factors))
{
}
Token FactorsAST::getOp() { return m_op; }
std::shared_ptr<ExprAST> FactorsAST::getFactor() { return m_factor; }
std::shared_ptr<ExprAST> FactorsAST::getFactors() { return m_factors; }
void FactorsAST::accept(Visitor& v)
{
    v.visit(*this);
}

TermAST::TermAST(std::shared_ptr<ExprAST> factor,
                 std::shared_ptr<ExprAST> factors)
    : m_factor(std::move(factor)), m_factors(std::move(factors))
{
}
std::shared_ptr<ExprAST> TermAST::getFactor() { return m_factor; }
std::shared_ptr<ExprAST> TermAST::getFactors() { return m_factors; }
void TermAST::accept(Visitor& v)
{
    v.visit(*this);
}

TermsAST::TermsAST(Token op, std::shared_ptr<ExprAST> term,
                 std::shared_ptr<ExprAST> terms)
    : m_op(op), m_term(std::move(term)), m_terms(std::move(terms))
{
}
Token TermsAST::getOp() { return m_op; }
std::shared_ptr<ExprAST> TermsAST::getTerm() { return m_term; }
std::shared_ptr<ExprAST> TermsAST::getTerms() { return m_terms; }
void TermsAST::accept(Visitor& v)
{
    v.visit(*this);
}

SimpleExprAST::SimpleExprAST(std::shared_ptr<ExprAST> term,
                   std::shared_ptr<ExprAST> terms)
    : m_term(std::move(term)), m_terms(std::move(terms))
{
}
std::shared_ptr<ExprAST> SimpleExprAST::getTerm() { return m_term; }
std::shared_ptr<ExprAST> SimpleExprAST::getTerms() { return m_terms; }
void SimpleExprAST::accept(Visitor& v)
{
    v.visit(*this);
}

OptRelationAST::OptRelationAST(Token op, std::shared_ptr<ExprAST> terms)
    : m_op(op), m_terms(std::move(terms))
{
}
Token OptRelationAST::getOp() { return m_op; }
std::shared_ptr<ExprAST> OptRelationAST::getTerms() { return m_terms; }
void OptRelationAST::accept(Visitor& v)
{
    v.visit(*this);
}

ExpressionAST::ExpressionAST(std::shared_ptr<ExprAST> terms,
                             std::shared_ptr<ExprAST> optRelation)
    : m_terms(std::move(terms)), m_optRelation(std::move(optRelation))
{
}
std::shared_ptr<ExprAST> ExpressionAST::getTerms() { return m_terms; }
std::shared_ptr<ExprAST> ExpressionAST::getOptRelation() { return m_optRelation; }
void ExpressionAST::accept(Visitor& v)
{
    v.visit(*this);
}

VarDeclAST::VarDeclAST(std::shared_ptr<VariableAST> identifier, Type type)
    : m_identifier(std::move(identifier)), m_type(type)
{
}
std::shared_ptr<VariableAST> VarDeclAST::getIdentifier()
{
    return m_identifier;
}
void VarDeclAST::accept(Visitor& v)
{
    v.visit(*this);
}

ArrDeclAST::ArrDeclAST(std::shared_ptr<VariableAST> identifier,
                       unsigned int size)
    : m_identifier(std::move(identifier)), m_size(size)
{
}
std::shared_ptr<VariableAST> ArrDeclAST::getIdentifier()
{
    return m_identifier;
}
void ArrDeclAST::accept(Visitor& v) { v.visit(*this); }
unsigned int ArrDeclAST::getSize() const { return m_size; }

DeclarationsAST::DeclarationsAST(std::shared_ptr<DeclAST> decl,
                                 std::shared_ptr<DeclarationsAST> next)
    : m_declaration(std::move(decl)), m_declarations(std::move(next))
{
}
std::shared_ptr<DeclAST> DeclarationsAST::getDeclaration()
{
  return m_declaration;
}
std::shared_ptr<DeclarationsAST> DeclarationsAST::getDeclarations()
{
  return m_declarations;
}

void DeclarationsAST::accept(Visitor& v)
{
    v.visit(*this);
}

ScopeAST::ScopeAST(std::shared_ptr<DeclarationsAST> decls,
                   std::shared_ptr<StatementsAST> stmts)
    : m_declarations(std::move(decls)), m_statements(std::move(stmts))
{
}
std::shared_ptr<DeclarationsAST> ScopeAST::getDeclarations()
{
    return m_declarations;
}
std::shared_ptr<StatementsAST> ScopeAST::getStatements()
{
    return m_statements;
}
void ScopeAST::accept(Visitor& v)
{
    v.visit(*this);
}

ScopedExprAST::ScopedExprAST(std::shared_ptr<DeclarationsAST> decls,
                   std::shared_ptr<StatementsAST> stmts,
                   std::shared_ptr<ExprAST> expr)
    : m_declarations(std::move(decls)), m_statements(std::move(stmts)), m_expr(std::move(expr))
{
}
std::shared_ptr<DeclarationsAST> ScopedExprAST::getDeclarations()
{
  return m_declarations;
}
std::shared_ptr<StatementsAST> ScopedExprAST::getStatements()
{
  return m_statements;
}
std::shared_ptr<ExprAST> ScopedExprAST::getExpr() { return m_expr; }
void ScopedExprAST::accept(Visitor& v)
{
    v.visit(*this);
}

AssignmentAST::AssignmentAST(std::shared_ptr<IdentifierAST> left,
                             std::shared_ptr<ExprAST> right)
    : m_left(std::move(left)), m_right(std::move(right))
{
}
std::shared_ptr<IdentifierAST> AssignmentAST::getIdentifier() { return m_left; }
std::shared_ptr<ExprAST> AssignmentAST::getExpr() { return m_right; }
void AssignmentAST::accept(Visitor& v)
{
    v.visit(*this);
}

OutputAST::OutputAST(std::shared_ptr<ExprAST> expr)
    : m_expr(std::move(expr))
{
}
void OutputAST::accept(Visitor& v)
{
    v.visit(*this);
}
std::shared_ptr<StatementAST> OutputAST::getExpr() { return m_expr; }

OutputsAST::OutputsAST(std::shared_ptr<ExprAST> output,
                             std::shared_ptr<OutputsAST> outputs)
    : m_output(std::move(output)), m_outputs(std::move(outputs))
{
}
std::shared_ptr<ExprAST> OutputsAST::getOutput() { return m_output; }
std::shared_ptr<OutputsAST> OutputsAST::getOutputs() { return m_outputs; }
void OutputsAST::accept(Visitor& v)
{
    v.visit(*this);
}

InputAST::InputAST(std::shared_ptr<IdentifierAST> expr)
    : m_expr(std::move(expr))
{
}
std::shared_ptr<IdentifierAST> InputAST::getInput() { return m_expr; }
void InputAST::accept(Visitor& v) { v.visit(*this); }

InputsAST::InputsAST(std::shared_ptr<InputAST> input,
                     std::shared_ptr<InputsAST> inputs)
    : m_input(std::move(input)), m_inputs(std::move(inputs))
{
}
std::shared_ptr<InputAST> InputsAST::getInput() { return m_input; }
std::shared_ptr<InputsAST> InputsAST::getInputs() { return m_inputs; }
void InputsAST::accept(Visitor& v) { v.visit(*this); }

IfAST::IfAST(std::shared_ptr<ExprAST> condition,
             std::shared_ptr<StatementsAST> thenArm,
             std::shared_ptr<StatementsAST> elseArm)
    : m_condition(std::move(condition)),
      m_thenArm(std::move(thenArm)),
      m_elseArm(std::move(elseArm))
{
}
std::shared_ptr<ExprAST> IfAST::getCondition() { return m_condition; }
std::shared_ptr<StatementsAST> IfAST::getThen() { return m_thenArm; }
std::shared_ptr<StatementsAST> IfAST::getElse() { return m_elseArm; }

void IfAST::accept(Visitor& v) { v.visit(*this); }

RepeatUntilAST::RepeatUntilAST(std::shared_ptr<StatementsAST> statements,
                               std::shared_ptr<ExprAST> exitCondition)
    : m_statements(std::move(statements)), m_exitCondition(std::move(exitCondition))
{
}
std::shared_ptr<StatementsAST> RepeatUntilAST::getStatements()
{
  return m_statements;
}
std::shared_ptr<ExprAST> RepeatUntilAST::getExitCond()
{
  return m_exitCondition;
}

void RepeatUntilAST::accept(Visitor& v) {
    v.visit(*this);
}

LoopAST::LoopAST(std::shared_ptr<StatementsAST> statements)
    : m_statements(std::move(statements))
{
}
std::shared_ptr<StatementsAST> LoopAST::getStatements() { return m_statements; }
void LoopAST::accept(Visitor& v)
{
  v.visit(*this);
}

void ExitAST::accept(Visitor& v) { v.visit(*this); }

ReturnAST::ReturnAST(std::shared_ptr<ExprAST> retExpr)
    : m_retExpr(std::move(retExpr))
{
}
std::shared_ptr<ExprAST> ReturnAST::getRetExpr() { return m_retExpr; }
  void ReturnAST::accept(Visitor& v) { v.visit(*this); }

ProgramAST::ProgramAST(std::shared_ptr<ScopeAST> scope)
    : m_scope(std::move(scope))
{
}
std::shared_ptr<ScopeAST> ProgramAST::getScope() { return m_scope; }
void ProgramAST::accept(Visitor& v)
{
    v.visit(*this);
}

ParameterAST::ParameterAST(std::shared_ptr<VariableAST> identifier, Type type)
    : m_identifier(std::move(identifier)), m_type(type)
{
}
std::shared_ptr<VariableAST> ParameterAST::getIdentifier()
{
  return m_identifier;
}
Type ParameterAST::getType() const { return m_type; }
void ParameterAST::accept(Visitor& v)
{
    v.visit(*this);
}

ParametersAST::ParametersAST(std::shared_ptr<ParameterAST> param,
                             std::shared_ptr<ParametersAST> params)
    : m_param(std::move(param)), m_params(std::move(params))
{
}
std::shared_ptr<ParameterAST> ParametersAST::getParam() { return m_param; }
std::shared_ptr<ParametersAST> ParametersAST::getParams() { return m_params; }
void ParametersAST::accept(Visitor& v)
{
    v.visit(*this);
}

ProcDeclAST::ProcDeclAST(std::string procName,
                         std::shared_ptr<ParametersAST> params,
                         std::shared_ptr<ScopeAST> scope)
    : m_procName(std::move(procName)),
      m_params(std::move(params)),
      m_scope(std::move(scope))
{
}
void ProcDeclAST::accept(Visitor& v)
{
    v.visit(*this);
}
std::string ProcDeclAST::getProcName() const { return m_procName; }
std::shared_ptr<ParametersAST> ProcDeclAST::getParams() { return m_params; }
std::shared_ptr<ScopeAST> ProcDeclAST::getScope() { return m_scope; }

FuncDeclAST::FuncDeclAST(std::string funcName,
                         std::shared_ptr<ParametersAST> params,
                         std::shared_ptr<ScopeAST> scope, Type type)
    : m_funcName(std::move(funcName)),
      m_params(std::move(params)),
      m_scope(std::move(scope)),
      m_type(type)
{
}
std::shared_ptr<ParametersAST> FuncDeclAST::getParams() { return m_params; }
std::shared_ptr<ScopeAST> FuncDeclAST::getScope() { return m_scope; }
void FuncDeclAST::accept(Visitor& v)
{
    v.visit(*this);
}
std::string FuncDeclAST::getFuncName() const
{ return m_funcName; }
