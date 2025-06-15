#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Token.hpp"
#include "Types.hpp"
#include "Visitors.hpp"

class StatementAST
{
public:  
    virtual void accept(Visitor& v) = 0;
};


class StatementsAST: public StatementAST
{
private:
    std::shared_ptr<StatementAST> m_statement;
    std::shared_ptr<StatementsAST> m_statements;
public:
    StatementsAST(std::shared_ptr<StatementAST> statement,
                  std::shared_ptr<StatementsAST> statements);

    virtual ~StatementsAST() = default;
    StatementsAST(const StatementsAST&) = delete;
    StatementsAST(StatementsAST&&) noexcept = default;
    StatementsAST& operator=(const StatementsAST&) = delete;
    StatementsAST& operator=(StatementsAST&&) noexcept = default;

    std::shared_ptr<StatementAST> getStatement();
    std::shared_ptr<StatementsAST> getStatements();
    void accept(Visitor& v) override;
};

class ExprAST: public StatementAST
{
public:
    virtual void accept(Visitor& v) = 0;
    virtual ~ExprAST() = default;
};

class NumberAST : public ExprAST
{
private:
    int m_val;

public:
    NumberAST(int val);

    virtual ~NumberAST() = default;
    NumberAST(const NumberAST&) = delete;
    NumberAST(NumberAST&&) noexcept = default;
    NumberAST& operator=(const NumberAST&) = delete;
    NumberAST& operator=(NumberAST&&) noexcept = default;

    int getVal() const;
    void accept(Visitor& v);
};

class BoolAST : public ExprAST
{
private:
    bool m_val;

public:
    BoolAST(bool val);

    virtual ~BoolAST() = default;
    BoolAST(const BoolAST&) = delete;
    BoolAST(BoolAST&&) noexcept = default;
    BoolAST& operator=(const BoolAST&) = delete;
    BoolAST& operator=(BoolAST&&) noexcept = default;

    bool getVal() const;
    void accept(Visitor& v);
};

class StringAST : public ExprAST
{
private:
    std::string m_str;
public:
    StringAST(std::string str);

    virtual ~StringAST() = default;
    StringAST(const StringAST&) = delete;
    StringAST(StringAST&&) noexcept = default;
    StringAST& operator=(const StringAST&) = delete;
    StringAST& operator=(StringAST&&) noexcept = default;

    std::string getVal() const;
    void accept(Visitor& v);
};

class IdentifierAST : public ExprAST
{
private:
    std::string m_name;
    Type m_type;

public:
    IdentifierAST(std::string name, Type type);

    virtual ~IdentifierAST() = default;
    IdentifierAST(const IdentifierAST&) = delete;
    IdentifierAST(IdentifierAST&&) noexcept = default;
    IdentifierAST& operator=(const IdentifierAST&) = delete;
    IdentifierAST& operator=(IdentifierAST&&) noexcept = default;

    std::string& getName();
    Type getType() const;
    void accept(Visitor& v) override;
};

class ArrAccessAST : public ExprAST
{
private:
    std::shared_ptr<IdentifierAST> m_identifier;
    std::shared_ptr<ExprAST> m_subsExpr;

public:
    ArrAccessAST(std::shared_ptr<IdentifierAST> identifier,
                 std::shared_ptr<ExprAST> subscriptAST);

    virtual ~ArrAccessAST() = default;
    ArrAccessAST(const ArrAccessAST&) = delete;
    ArrAccessAST(ArrAccessAST&&) noexcept = default;
    ArrAccessAST& operator=(const ArrAccessAST&) = delete;
    ArrAccessAST& operator=(ArrAccessAST&&) noexcept = default;

    std::shared_ptr<IdentifierAST> getIdentifier();
    std::shared_ptr<ExprAST> getSubsExpr();
    void accept(Visitor& v) override;
};

class ArgumentsAST : public ExprAST
{
private:
    std::shared_ptr<ExprAST> m_expr;
    std::shared_ptr<ArgumentsAST> m_arguments;

public:
    ArgumentsAST(std::shared_ptr<ExprAST> expr,
              std::shared_ptr<ArgumentsAST> arguments);
    
    ~ArgumentsAST() = default;
    ArgumentsAST(const ArgumentsAST&) = delete;
    ArgumentsAST(ArgumentsAST&&) noexcept = default;
    ArgumentsAST& operator=(const ArgumentsAST&) = delete;
    ArgumentsAST& operator=(ArgumentsAST&&) noexcept = default;

    std::shared_ptr<ExprAST> getExpr();
    std::shared_ptr<ArgumentsAST> getArgs();
    void accept(Visitor& v);
};

class CallAST : public ExprAST
{
private:
    std::string m_funcName;
    std::shared_ptr<ArgumentsAST> m_arguments;

public:
    CallAST(std::string m_funcName, std::shared_ptr<ArgumentsAST> arguments);
    ~CallAST() = default;
    CallAST(const CallAST&) = delete;
    CallAST(CallAST&&) noexcept = default;
    CallAST& operator=(const CallAST&) = delete;
    CallAST& operator=(CallAST&&) noexcept = default;

    std::string getFuncName() const;
    std::shared_ptr<ArgumentsAST> getArgs();
    void accept(Visitor& v);
};

class FactorAST : public ExprAST
{
private:
    Token m_op;
    std::shared_ptr<ExprAST> m_factor;

public:
    FactorAST(Token op, std::shared_ptr<ExprAST> factor);

    ~FactorAST() = default;
    FactorAST(const FactorAST&) = delete;
    FactorAST(FactorAST&&) noexcept = default;
    FactorAST& operator=(const FactorAST&) = delete;
    FactorAST& operator=(FactorAST&&) noexcept = default;

    std::shared_ptr<ExprAST> getFactor();
    void accept(Visitor& v);
};

class FactorsAST : public ExprAST
{
private:
    Token m_op;
    std::shared_ptr<ExprAST> m_factor;
    std::shared_ptr<ExprAST> m_factors;

public:
    FactorsAST(Token op, std::shared_ptr<ExprAST> factor,
               std::shared_ptr<ExprAST> factors);
  
    ~FactorsAST() = default;
    FactorsAST(const FactorsAST&) = delete;
    FactorsAST(FactorsAST&&) noexcept = default;
    FactorsAST& operator=(const FactorsAST&) = delete;
    FactorsAST& operator=(FactorsAST&&) noexcept = default;
 
    Token getOp();
    std::shared_ptr<ExprAST> getFactor();
    std::shared_ptr<ExprAST> getFactors();
    void accept(Visitor& v);
};

class TermAST : public ExprAST
{
private:
    std::shared_ptr<ExprAST> m_factor;
    std::shared_ptr<ExprAST> m_factors;

public:
    TermAST(std::shared_ptr<ExprAST> factor,
            std::shared_ptr<ExprAST> factors);

    ~TermAST() = default;
    TermAST(const TermAST&) = delete;
    TermAST(TermAST&&) noexcept = default;
    TermAST& operator=(const TermAST&) = delete;
    TermAST& operator=(TermAST&&) noexcept = default;

    std::shared_ptr<ExprAST> getFactor();
    std::shared_ptr<ExprAST> getFactors();
    void accept(Visitor& v);
};

class TermsAST : public ExprAST
{
private:
    Token m_op;
    std::shared_ptr<ExprAST> m_term;
    std::shared_ptr<ExprAST> m_terms;

public:
    TermsAST(Token op, std::shared_ptr<ExprAST> m_term,
             std::shared_ptr<ExprAST> m_terms);

    ~TermsAST() = default;
    TermsAST(const TermsAST&) = delete;
    TermsAST(TermsAST&&) noexcept = default;
    TermsAST& operator=(const TermsAST&) = delete;
    TermsAST& operator=(TermsAST&&) noexcept = default;

    std::shared_ptr<ExprAST> getTerm();
    std::shared_ptr<ExprAST> getTerms();
    Token getOp();
    void accept(Visitor& v);
};

class SimpleExprAST : public ExprAST
{
private:
    std::shared_ptr<ExprAST> m_term;
    std::shared_ptr<ExprAST> m_terms;

 public:
    SimpleExprAST(std::shared_ptr<ExprAST> m_term,
             std::shared_ptr<ExprAST> m_terms);

    ~SimpleExprAST() = default;
    SimpleExprAST(const SimpleExprAST&) = delete;
    SimpleExprAST(SimpleExprAST&&) noexcept = default;
    SimpleExprAST& operator=(const SimpleExprAST&) = delete;
    SimpleExprAST& operator=(SimpleExprAST&&) noexcept = default;

    std::shared_ptr<ExprAST> getTerm();
    std::shared_ptr<ExprAST> getTerms();
    void accept(Visitor& v);
};

class OptRelationAST : public ExprAST
{
private:
    Token m_op;
    std::shared_ptr<ExprAST> m_terms;

public:
    OptRelationAST(Token op, std::shared_ptr<ExprAST> terms);

    ~OptRelationAST() = default;
    OptRelationAST(const OptRelationAST&) = delete;
    OptRelationAST(OptRelationAST&&) noexcept = default;
    OptRelationAST& operator=(const OptRelationAST&) = delete;
    OptRelationAST& operator=(OptRelationAST&&) noexcept = default;

    Token getOp();
    std::shared_ptr<ExprAST> getTerms();
    void accept(Visitor& v);
};

class ExpressionAST : public ExprAST
{
private:
    std::shared_ptr<ExprAST> m_terms;
    std::shared_ptr<ExprAST> m_optRelation;

public:
    ExpressionAST(std::shared_ptr<ExprAST> m_terms,
                  std::shared_ptr<ExprAST> optRelation);

    ~ExpressionAST() = default;
    ExpressionAST(const ExpressionAST&) = delete;
    ExpressionAST(ExpressionAST&&) noexcept = default;
    ExpressionAST& operator=(const ExpressionAST&) = delete;
    ExpressionAST& operator=(ExpressionAST&&) noexcept = default;

    std::shared_ptr<ExprAST> getTerms();
    std::shared_ptr<ExprAST> getOptRelation();
    void accept(Visitor& v);
};

class DeclAST
{
public:
    virtual void accept(Visitor& v) = 0;
    virtual ~DeclAST() = default;
};

class VarDeclAST : public DeclAST
{
public:
    std::shared_ptr<IdentifierAST> m_identifier;
    Type m_type;

public:
    VarDeclAST(std::shared_ptr<IdentifierAST> identifier, Type type);

    ~VarDeclAST() = default;
    VarDeclAST(const VarDeclAST&) = delete;
    VarDeclAST(VarDeclAST&&) noexcept = default;
    VarDeclAST& operator=(const VarDeclAST&) = delete;
    VarDeclAST& operator=(VarDeclAST&&) noexcept = default;

    std::shared_ptr<IdentifierAST> getIdentifier();
    void accept(Visitor& v) override;
};

class ArrDeclAST : public DeclAST
{
public:
    std::shared_ptr<IdentifierAST> m_identifier;
    unsigned int m_size;

public:
    ArrDeclAST(std::shared_ptr<IdentifierAST> identifier, unsigned int size);

    ~ArrDeclAST() = default;
    ArrDeclAST(const ArrDeclAST&) = delete;
    ArrDeclAST(ArrDeclAST&&) noexcept = default;
    ArrDeclAST& operator=(const ArrDeclAST&) = delete;
    ArrDeclAST& operator=(ArrDeclAST&&) noexcept = default;

    std::shared_ptr<IdentifierAST> getIdentifier();
    unsigned int getSize() const;
    void accept(Visitor& v) override;
};

class DeclarationsAST : public DeclAST
{
private:
    std::shared_ptr<DeclAST> m_declaration;
    std::shared_ptr<DeclarationsAST> m_declarations;

public:
    DeclarationsAST(std::shared_ptr<DeclAST> decl,
                    std::shared_ptr<DeclarationsAST> next = nullptr);

    ~DeclarationsAST() = default;
    DeclarationsAST(const DeclarationsAST&) = delete;
    DeclarationsAST(DeclarationsAST&&) noexcept = default;
    DeclarationsAST& operator=(const DeclarationsAST&) = delete;
    DeclarationsAST& operator=(DeclarationsAST&&) noexcept = default;

    std::shared_ptr<DeclAST> getDeclaration();
    std::shared_ptr<DeclarationsAST> getDeclarations();
    void accept(Visitor& v) override;
};

class ScopeAST: public StatementAST
{
private:
    std::shared_ptr<DeclarationsAST> m_declarations;
    std::shared_ptr<StatementsAST> m_statements;

public:
    ScopeAST(std::shared_ptr<DeclarationsAST> decls,
             std::shared_ptr<StatementsAST> stmts);

    ~ScopeAST() = default;
    ScopeAST(const ScopeAST&) = delete;
    ScopeAST(ScopeAST&&) noexcept = default;
    ScopeAST& operator=(const ScopeAST&) = delete;
    ScopeAST& operator=(ScopeAST&&) noexcept = default;

    std::shared_ptr<DeclarationsAST> getDeclarations();
    std::shared_ptr<StatementsAST> getStatements();
    void accept(Visitor& v);
};

class ScopedExprAST : public ExprAST
{
private:
    std::shared_ptr<DeclarationsAST> m_declarations;
    std::shared_ptr<StatementsAST> m_statements;
    std::shared_ptr<ExprAST> m_expr;

public:
    ScopedExprAST(std::shared_ptr<DeclarationsAST> decls,
               std::shared_ptr<StatementsAST> stmts,
               std::shared_ptr<ExprAST> expr);
    ~ScopedExprAST() = default;
    ScopedExprAST(const ScopedExprAST&) = delete;
    ScopedExprAST(ScopedExprAST&&) noexcept = default;
    ScopedExprAST& operator=(const ScopedExprAST&) = delete;
    ScopedExprAST& operator=(ScopedExprAST&&) noexcept = default;

    std::shared_ptr<DeclarationsAST> getDeclarations();
    std::shared_ptr<StatementsAST> getStatements();
    std::shared_ptr<ExprAST> getExpr();

    void accept(Visitor& v);
};

class AssignmentAST : public StatementAST
{
private:
    std::shared_ptr<ExprAST> m_left;
    std::shared_ptr<ExprAST> m_right;

public:
    AssignmentAST(std::shared_ptr<ExprAST> left,
                  std::shared_ptr<ExprAST> right);

    ~AssignmentAST() = default;
    AssignmentAST(const AssignmentAST&) = delete;
    AssignmentAST(AssignmentAST&&) noexcept = default;
    AssignmentAST& operator=(const AssignmentAST&) = delete;
    AssignmentAST& operator=(AssignmentAST&&) noexcept = default;

    std::shared_ptr<ExprAST> getIdentifier();
    std::shared_ptr<ExprAST> getExpr();
    void accept(Visitor& v);
};

class OutputAST : public StatementAST
{
private:
    std::shared_ptr<StatementAST> m_expr;

public:
    OutputAST(std::shared_ptr<ExprAST> expr);

    ~OutputAST() = default;
    OutputAST(const OutputAST&) = delete;
    OutputAST(OutputAST&&) noexcept = default;
    OutputAST& operator=(const OutputAST&) = delete;
    OutputAST& operator=(OutputAST&&) noexcept = default;

    std::shared_ptr<StatementAST> getExpr();
    void accept(Visitor& v);
};

class OutputsAST : public StatementAST
{
private:
    std::shared_ptr<ExprAST> m_output;
    std::shared_ptr<OutputsAST> m_outputs;

public:
    OutputsAST(std::shared_ptr<ExprAST> output,
               std::shared_ptr<OutputsAST> outputs);
    ~OutputsAST() = default;
    OutputsAST(const OutputsAST&) = delete;
    OutputsAST(OutputsAST&&) noexcept = default;
    OutputsAST& operator=(const OutputsAST&) = delete;
    OutputsAST& operator=(OutputsAST&&) noexcept = default;

    std::shared_ptr<ExprAST> getOutput();
    std::shared_ptr<OutputsAST> getOutputs();
    void accept(Visitor& v);
};

class InputAST : public StatementAST
{
private:
    std::shared_ptr<StatementAST> m_expr;

public:
    InputAST(std::shared_ptr<ExprAST> expr);

    ~InputAST() = default;
    InputAST(const InputAST&) = delete;
    InputAST(InputAST&&) noexcept = default;
    InputAST& operator=(const InputAST&) = delete;
    InputAST& operator=(InputAST&&) noexcept = default;

    std::shared_ptr<StatementAST> getExpr();
    void accept(Visitor& v);
};

class InputsAST : public StatementAST
{
private:
    std::shared_ptr<ExprAST> m_input;
    std::shared_ptr<InputsAST> m_inputs;

public:
    InputsAST(std::shared_ptr<ExprAST> input, std::shared_ptr<InputsAST> inputs);
    ~InputsAST() = default;
    InputsAST(const InputsAST&) = delete;
    InputsAST(InputsAST&&) noexcept = default;
    InputsAST& operator=(const InputsAST&) = delete;
    InputsAST& operator=(InputsAST&&) noexcept = default;

    std::shared_ptr<ExprAST> getInput();
    std::shared_ptr<InputsAST> getInputs();
    void accept(Visitor& v);
};

class IfAST : public StatementAST
{
private:
    std::shared_ptr<ExprAST> m_condition;
    std::shared_ptr<StatementsAST> m_thenArm;
    std::shared_ptr<StatementsAST> m_elseArm;

public:
    IfAST(std::shared_ptr<ExprAST> condition,
          std::shared_ptr<StatementsAST> thenArm,
          std::shared_ptr<StatementsAST> elseArm);

    ~IfAST() = default;
    IfAST(const IfAST&) = delete;
    IfAST(IfAST&&) noexcept = default;
    IfAST& operator=(const IfAST&) = delete;
    IfAST& operator=(IfAST&&) noexcept = default;

    std::shared_ptr<ExprAST> getCondition();
    std::shared_ptr<StatementsAST> getThen();
    std::shared_ptr<StatementsAST> getElse();
    void accept(Visitor& v);
};

class RepeatUntilAST : public StatementAST
{
private:
    std::shared_ptr<StatementsAST> m_statements;
    std::shared_ptr<ExprAST> m_exitCondition;

public:
    RepeatUntilAST(std::shared_ptr<StatementsAST> statements,
                std::shared_ptr<ExprAST> exitCondition);

    ~RepeatUntilAST() = default;
    RepeatUntilAST(const RepeatUntilAST&) = delete;
    RepeatUntilAST(RepeatUntilAST&&) noexcept = default;
    RepeatUntilAST& operator=(const RepeatUntilAST&) = delete;
    RepeatUntilAST& operator=(RepeatUntilAST&&) noexcept = default;

    std::shared_ptr<StatementsAST> getStatements();
    std::shared_ptr<ExprAST> getExitCond();
    void accept(Visitor& v);
};

class LoopAST : public StatementAST
{
private:
    std::shared_ptr<StatementsAST> m_statements;

public:
    LoopAST(std::shared_ptr<StatementsAST> statements);

    ~LoopAST() = default;
    LoopAST(const LoopAST&) = delete;
    LoopAST(LoopAST&&) noexcept = default;
    LoopAST& operator=(const LoopAST&) = delete;
    LoopAST& operator=(LoopAST&&) noexcept = default;

    std::shared_ptr<StatementsAST> getStatements();
    void accept(Visitor& v);
};

class ExitAST : public StatementAST
{
public:
    ExitAST() = default;
    ~ExitAST() = default;
    ExitAST(const ExitAST&) = delete;
    ExitAST(ExitAST&&) noexcept = default;
    ExitAST& operator=(const ExitAST&) = delete;
    ExitAST& operator=(ExitAST&&) noexcept = default;
    void accept(Visitor& v);
};

class ReturnAST : public StatementAST
{
private:
    std::shared_ptr<ExprAST> m_retExpr;
public:
    ReturnAST(std::shared_ptr<ExprAST> retExpr);
    ~ReturnAST() = default;
    ReturnAST(const ReturnAST&) = delete;
    ReturnAST(ReturnAST&&) noexcept = default;
    ReturnAST& operator=(const ReturnAST&) = delete;
    ReturnAST& operator=(ReturnAST&&) noexcept = default;

    std::shared_ptr<ExprAST> getRetExpr();
    void accept(Visitor& v);
};


class ProgramAST
{
private:
    std::shared_ptr<ScopeAST> m_scope;

public:
    ProgramAST(std::shared_ptr<ScopeAST> scope);

    ~ProgramAST() = default;
    ProgramAST(const ProgramAST&) = delete;
    ProgramAST(ProgramAST&&) noexcept = default;
    ProgramAST& operator=(const ProgramAST&) = delete;
    ProgramAST& operator=(ProgramAST&&) noexcept = default;

    std::shared_ptr<ScopeAST> getScope();
    void accept(Visitor& v);
};

class ParameterAST
{
private:
    std::shared_ptr<IdentifierAST> m_identifier;
    Type m_type;
  
public:
    ParameterAST(std::shared_ptr<IdentifierAST> identifier, Type type);
  
    ~ParameterAST() = default;
    ParameterAST(const ParameterAST&) = delete;
    ParameterAST(ParameterAST&&) noexcept = default;
    ParameterAST& operator=(const ParameterAST&) = delete;
    ParameterAST& operator=(ParameterAST&&) noexcept = default;
  
    std::shared_ptr<IdentifierAST> getIdentifier();
    Type getType() const;
    void accept(Visitor& v);
};

class ParametersAST
{
private:
    std::shared_ptr<ParameterAST> m_param;
    std::shared_ptr<ParametersAST> m_params;

public:
    ParametersAST(std::shared_ptr<ParameterAST> param,
                  std::shared_ptr<ParametersAST> params);
  
    ~ParametersAST() = default;
    ParametersAST(const ParametersAST&) = delete;
    ParametersAST(ParametersAST&&) noexcept = default;
    ParametersAST& operator=(const ParametersAST&) = delete;
    ParametersAST& operator=(ParametersAST&&) noexcept = default;
 
    std::shared_ptr<ParameterAST> getParam();
    std::shared_ptr<ParametersAST> getParams();
    void accept(Visitor& v);
};

class ProcDeclAST : public DeclAST
{
private:
    std::string m_procName;
    std::shared_ptr<ParametersAST> m_params;
    std::shared_ptr<ScopeAST> m_scope;

public:
    ProcDeclAST(std::string procName, std::shared_ptr<ParametersAST> params,
                std::shared_ptr<ScopeAST> scope);
  
    ~ProcDeclAST() = default;
    ProcDeclAST(const ProcDeclAST&) = delete;
    ProcDeclAST(ProcDeclAST&&) noexcept = default;
    ProcDeclAST& operator=(const ProcDeclAST&) = delete;
    ProcDeclAST& operator=(ProcDeclAST&&) noexcept = default;
  
    std::string getProcName() const;
    std::shared_ptr<ParametersAST> getParams();
    std::shared_ptr<ScopeAST> getScope();
    void accept(Visitor& v) override;
};

class FuncDeclAST : public DeclAST
{
private:
    std::string m_funcName;
    std::shared_ptr<ParametersAST> m_params;
    std::shared_ptr<ScopeAST> m_scope;
    Type m_type;

public:
    FuncDeclAST(std::string funcName, std::shared_ptr<ParametersAST> params,
                std::shared_ptr<ScopeAST> scope, Type type);
  
    ~FuncDeclAST() = default;
    FuncDeclAST(const FuncDeclAST&) = delete;
    FuncDeclAST(FuncDeclAST&&) noexcept = default;
    FuncDeclAST& operator=(const FuncDeclAST&) = delete;
    FuncDeclAST& operator=(FuncDeclAST&&) noexcept = default;
  
    std::string getFuncName() const;
    std::shared_ptr<ParametersAST> getParams();
    std::shared_ptr<ScopeAST> getScope();
    void accept(Visitor& v) override;
};
