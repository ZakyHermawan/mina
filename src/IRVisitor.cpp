#include "IRVisitor.hpp"
#include "Ast.hpp"

IRVisitor::IRVisitor() : m_tempCounter(0), m_labelCounter(0) {}

void IRVisitor::visit(StatementsAST& v)
{
  auto statementAST = v.getStatement();
  auto statementsAST = v.getStatements();
  if (statementAST)
  {
      statementAST->accept(*this);
  }
  if (statementsAST)
  {
      statementsAST->accept(*this);
  }
}
void IRVisitor::visit(NumberAST& v)
{
    m_temp.push(std::to_string(v.getVal()));
}
void IRVisitor::visit(BoolAST& v) {
    if (v.getVal())
    {
        m_temp.push("true");
    }
    else
    {
        m_temp.push("false");
    }
}
void IRVisitor::visit(StringAST& v)
{
    auto val = v.getVal();
    if (val == "\n")
    {
        m_temp.push("\'\\n\'");
    }
    else
    {
        m_temp.push(val);
    }
}

void IRVisitor::visit(IdentifierAST& v)
{
    m_temp.push(v.getName());
}

void IRVisitor::visit(ProgramAST& v)
{
    v.getScope()->accept(*this);
}
void IRVisitor::visit(ScopeAST& v)
{
    auto decls = v.getDeclarations();
    auto stmts = v.getStatements();
    
    if (decls)
    {
        decls->accept(*this);
    }
    if (stmts)
    {
        stmts->accept(*this);
    }
}

void IRVisitor::visit(ScopedExprAST& v)
{
    std::cout << "Scoped Expression AST\n";
}

void IRVisitor::visit(AssignmentAST& v)
{
    v.getExpr()->accept(*this);
    auto exprTemp = popTemp();
    auto identifier = v.getIdentifier();
    identifier->accept(*this);

    std::cout << popTemp() << " <- " << exprTemp;
    std::cout << std::endl;
    pushCurrentTemp();
}

void IRVisitor::visit(OutputAST& v)
{
    auto output = v.getExpr();
    output->accept(*this);
}
void IRVisitor::visit(OutputsAST& v)
{
    auto output = v.getOutput();
    std::cout << "put(";
    output->accept(*this);
    std::cout << popTemp();
    std::cout << ")\n";
    auto outputs = v.getOutputs();
    if (outputs)
    {
        outputs->accept(*this);
    }
}
void IRVisitor::visit(InputAST& v)
{
    auto input = v.getExpr();
    input->accept(*this);
}
void IRVisitor::visit(InputsAST& v)
{
    auto input = v.getInput();
    std::cout << "get(";
    input->accept(*this);
    std::cout << popTemp();
    std::cout << ")\n";
    auto inputs = v.getInputs();
    if (inputs)
    {
        inputs->accept(*this);
    }
}
void IRVisitor::visit(IfAST& v)
{
    auto thenLabel = "thenBlock_" + std::to_string(m_labelCounter) + ":";
    auto elseLabel = "elseBlock_" + std::to_string(m_labelCounter) + ":";
    auto mergeLabel = "mergeBlock_" + std::to_string(m_labelCounter) + ":";

    auto expr = v.getCondition();
    auto thenArm = v.getThen();
    auto elseArm = v.getElse();

    std::cout << std::endl;
    expr->accept(*this);
    std::cout << "if(!" << popTemp();
    std::cout << ") goto ";
    if (elseArm)
    {
        std::cout << elseLabel << std::endl;
    }
    else
    {
        std::cout << mergeLabel << std::endl;
    }
    thenArm->accept(*this);
    std::cout << "goto " << mergeLabel << std::endl;
    if (elseArm)
    {
        std::cout << elseLabel << std::endl;
        elseArm->accept(*this);
    }
    std::cout << mergeLabel << std::endl;
    ++m_labelCounter;
}
void IRVisitor::visit(RepeatUntilAST& v)
{
    auto label = "repeatUntilBlock_" + std::to_string(m_labelCounter++) + ":";
    std::cout << '\n' << label << std::endl;
    auto statements = v.getStatements();
    auto expr = v.getExitCond();
    statements->accept(*this);
    expr->accept(*this);
    std::cout << "if(!" << popTemp();
    std::cout << ") goto " << label << std::endl << std::endl;
}
void IRVisitor::visit(LoopAST& v)
{
    auto label = "Loop_" + std::to_string(m_labelCounter++) + ":";
    std::cout << '\n' << label << std::endl;

    auto statements = v.getStatements();
    v.accept(*this);

    std::cout << "goto " << label << std::endl << std::endl;
}
void IRVisitor::visit(ExitAST& v) { std::cout << "Exit AST\n"; }
void IRVisitor::visit(ReturnAST& v)
{
    auto retExpr = v.getRetExpr();
    retExpr->accept(*this);
    std::cout << "return " << popTemp() << std::endl;
}
void IRVisitor::visit(ArrAccessAST& v)
{
    auto identifier = v.getIdentifier();
    auto subsExpr = v.getSubsExpr();
    subsExpr->accept(*this);
    auto st = identifier->getName() + "[" + popTemp() + "]";
    m_temp.push(st);
}
void IRVisitor::visit(ArgumentsAST& v)
{
    auto expr = v.getExpr();
    auto args = v.getArgs();
    expr->accept(*this);
    std::cout << "push " << popTemp() << std::endl;
    if (args)
    {
        args->accept(*this);
    }
}

void IRVisitor::visit(CallAST& v)
{
    auto args = v.getArgs();
    if (args)
    {
       args->accept(*this);
    }
    std::cout << "call " << v.getFuncName() << '\n';
}
void IRVisitor::visit(FactorAST& v)
{
    v.getFactor()->accept(*this);
}
void IRVisitor::visit(FactorsAST& v)
{
    auto op = v.getOp().getLexme();
    auto factor = v.getFactor();
    factor->accept(*this);

    auto right = popTemp();
    auto left = popTemp();
    generateCurrentTemp();
    pushCurrentTemp();

    std::cout << " <- ";
    std::cout << left << " " << op << " " << right << std::endl;

    auto factors = v.getFactors();
    if (factors)
    {
        factors->accept(*this);
    }
}
void IRVisitor::visit(TermAST& v)
{
  v.getFactor()->accept(*this);
  auto factors = v.getFactors();
  if (factors) factors->accept(*this);
}
void IRVisitor::visit(TermsAST& v)
{
    auto op = v.getOp().getLexme();
    v.getTerm()->accept(*this);

    auto right = popTemp();
    auto left = popTemp();
    generateCurrentTemp();
    pushCurrentTemp();

    std::cout << " <- ";
    std::cout << left << " " << op << " " << right << std::endl;
    auto terms = v.getTerms();
    if (terms)
    {
      terms->accept(*this);
    }
}
void IRVisitor::visit(SimpleExprAST& v)
{
    v.getTerm()->accept(*this);
    auto terms = v.getTerms();
    if (terms) terms->accept(*this);
}
void IRVisitor::visit(OptRelationAST& v)
{
    auto op = v.getOp().getLexme();
    v.getTerms()->accept(*this);
    generateCurrentTemp();
    std::cout << " <- ";
    std::cout << popTemp() << " " << op << " " << popTemp() << std::endl;
    pushCurrentTemp();
}
void IRVisitor::visit(ExpressionAST& v)
{
    auto terms = v.getTerms();
    auto optRelation = v.getOptRelation();
    if (terms == nullptr)
    {
    }
    if (terms)
    {
        terms->accept(*this);
    }
    if (optRelation)
    {
        optRelation->accept(*this);
    }
}

void IRVisitor::visit(VarDeclAST& v)
{
    std::cout << v.getIdentifier()->getName() << " <- ";
    auto type = v.getIdentifier()->getType();
    if (type == Type::BOOLEAN)
    {
        std::cout << "false\n";
    }
    else if (type == Type::INTEGER)
    {
        std::cout << "0\n";
    }
    else
    {
        throw std::runtime_error("Unknown type");
    }
}

void IRVisitor::visit(ArrDeclAST& v)
{
    for (unsigned int i = 0; i < v.getSize(); ++i)
    {
        std::cout << v.getIdentifier()->getName() << "[" << i << "]"  " <- ";

        auto type = v.getIdentifier()->getType();
        if (type == Type::BOOLEAN)
        {
            std::cout << "false\n";
        }
        else if (type == Type::INTEGER)
        {
            std::cout << "0\n";
        }
        else
        {
            throw std::runtime_error("Unknown type");
        }
    }
}

void IRVisitor::visit(DeclarationsAST& v)
{
    auto decl = v.getDeclaration();
    decl->accept(*this);
    auto decls = v.getDeclarations();
    if (decls)
    {
        decls->accept(*this);
    }
}
void IRVisitor::visit(ParameterAST& v)
{
    std::cout << v.getIdentifier()->getName() << " : " << typeToStr(v.getType());
}
void IRVisitor::visit(ParametersAST& v)
{
    auto parameter = v.getParam();
    auto parameters = v.getParams();
    parameter->accept(*this);
    if (parameters)
    {
        std::cout << ", ";
        parameters->accept(*this);
    }
}
void IRVisitor::visit(ProcDeclAST& v)
{
    std::cout << std::endl << "proc_" << v.getProcName() << "(";
    auto params = v.getParams();
    auto scope = v.getScope();
    if (params)
    {
        params->accept(*this);
        
    }
    std::cout << ")\n";
    scope->accept(*this);
    std::cout << "endProc\n";
}
void IRVisitor::visit(FuncDeclAST& v)
{
    std::cout << std::endl << "func_" << v.getFuncName() << "(";
    auto params = v.getParams();
    auto scope = v.getScope();
    if (params)
    {
        params->accept(*this);
    }
    std::cout << ")\n";
    scope->accept(*this);
    std::cout << "endFunc\n";
}

void IRVisitor::generateCurrentTemp()
{
    std::cout << "t" << m_tempCounter;
}

void IRVisitor::pushCurrentTemp()
{
    auto currentTemp = "t" + std::to_string(m_tempCounter);
    m_temp.push(currentTemp);
    ++m_tempCounter;
}
std::string IRVisitor::popTemp()
{
  if (m_temp.size() == 0)
  {
    std::cout << "empty!\n";
    exit(1);
  }
    std::string val = m_temp.top();
    m_temp.pop();
    return val;
}
std::string IRVisitor::getLastTemp()
{
  return "t" + std::to_string(m_tempCounter - 1);
}
