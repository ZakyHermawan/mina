#include "Ast.hpp"
#include "InstIR.hpp"
#include "CodeGen.hpp"
#include "IRVisitor.hpp"
#include "SSA.hpp"

#include <memory>
#include <stdexcept>

IRVisitor::IRVisitor()
    : m_tempCounter(0),
      m_labelCounter(0),
      m_ssa{},
      m_cg{m_ssa}
{
    m_currentBB = m_ssa.getCFG();
}

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
    auto val = v.getVal();
    m_temp.push(std::to_string(val));
    auto inst = std::make_shared<IntConstInst>(val, m_currentBB);
    inst->setup_def_use();
    m_instStack.push(inst);
}

void IRVisitor::visit(BoolAST& v) {
    auto val = v.getVal();
    auto inst = std::make_shared<BoolConstInst>(val, m_currentBB);
    inst->setup_def_use();
    m_instStack.push(inst);

    if (val)
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
        std::string str = "\'\\n\'";
        auto inst = std::make_shared<StrConstInst>(str, m_currentBB);
        inst->setup_def_use();
        m_instStack.push(inst);
    }
    else
    {
        auto newVal = "\"" + val + "\"";
        m_temp.push(newVal);
        auto inst = std::make_shared<StrConstInst>(newVal, m_currentBB);
        inst->setup_def_use();
        m_instStack.push(inst);
    }
}

void IRVisitor::visit(VariableAST& v)
{
    auto& val = v.getName();
    auto valInst = m_ssa.readVariable(val, m_currentBB);
    m_temp.push(val);
    m_instStack.push(valInst);
}

void IRVisitor::visit(ProgramAST& v)
{
    v.getScope()->accept(*this);
    auto haltInst = std::make_shared<HaltInst>(m_currentBB);
    m_currentBB->pushInst(haltInst);
    
    m_ssa.sealBlock(m_currentBB);

    //std::cout << "Before renaming: \n";
    m_ssa.renameSSA();

    //std::cout << "\n\nAfter renaming: \n";
    //m_ssa.printCFG();

    SSA old = m_ssa;
    m_cg.setSSA(old);
    //m_cg.linearizeCFG();
    m_cg.generateMIR();

    for (auto const& [key, func]: m_funcBB)
    {
        SSA newSSA;
        newSSA.setCFG(func);

        // todo: correctly implement rename for function signature,
        // one of the solution:
        // make special case for parameters in function signature,
        // make every parameter declaration as definition a new variable
        // then just do phiweb
        newSSA.renameSSA();
        newSSA.printCFG();
    }
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
    auto decls = v.getDeclarations();
    auto stmts = v.getStatements();
    auto expr = v.getExpr();

    if (decls)
    {
        decls->accept(*this);
    }
    if (stmts)
    {
        stmts->accept(*this);
    }
    if (expr)
    {
        expr->accept(*this);
    }
}

void IRVisitor::visit(AssignmentAST& v)
{
    v.getExpr()->accept(*this);
    auto exprTemp = popTemp();
    auto exprInst = popInst();

    auto identifier = v.getIdentifier();    
    auto& targetStr = identifier->getName();

    if (identifier->getIdentType() == IdentType::ARRAY)
    {
        auto sourceSSAName = m_ssa.getCurrentSSAName(targetStr);
        auto targetSSAName = m_ssa.baseNameToSSA(targetStr);
        auto sourceInst = m_ssa.readVariable(targetStr, m_currentBB);
        auto targetInst = std::make_shared<IdentInst>(targetSSAName, m_currentBB);
        targetInst->setup_def_use();

        auto arrIdentifier =
            std::dynamic_pointer_cast<ArrAccessAST>(identifier);
        auto type = arrIdentifier->getType();
        auto subsExpr = arrIdentifier->getSubsExpr();
        subsExpr->accept(*this);
        auto subsInst = popInst();
        auto arrayUpdateInst = std::make_shared<ArrUpdateInst>(
            targetInst, sourceInst, subsInst, exprInst, m_currentBB, type);
        arrayUpdateInst->setup_def_use();
        m_ssa.writeVariable(targetStr, m_currentBB, arrayUpdateInst);
        m_currentBB->pushInst(arrayUpdateInst);
    }
    else if (identifier->getIdentType() == IdentType::VARIABLE)
    {
        auto targetSSAName = m_ssa.baseNameToSSA(targetStr);
        auto targetSSAInst =
          std::make_shared<IdentInst>(targetSSAName, m_currentBB);
        targetSSAInst->setup_def_use();
        auto assignInst = std::make_shared<AssignInst>(targetSSAInst, exprInst, m_currentBB);
        assignInst->setup_def_use();
        m_ssa.writeVariable(targetStr, m_currentBB, assignInst);
        m_currentBB->pushInst(std::move(assignInst));
    }
    else
    {
        throw std::runtime_error("Unknown identifier type!");
    }
}

void IRVisitor::visit(OutputAST& v)
{
    auto output = v.getExpr();
    output->accept(*this);
}

void IRVisitor::visit(OutputsAST& v)
{
    auto output = v.getOutput();
    output->accept(*this);
    auto temp = popTemp();

    auto inst = popInst();
    
    if (temp == "\'\\n\'")
    {
        auto operand = std::make_shared<StrConstInst>("\'\\n\'", m_currentBB);
        operand->setup_def_use();
        auto putInst =
            std::make_shared<PutInst>(std::move(operand), m_currentBB);
        putInst->setup_def_use();
        m_currentBB->pushInst(std::move(putInst));
    }
    else if (temp == "true" || temp == "false")
    {
        auto boolVal = (temp == "true") ? true : false;
        auto operand = std::make_shared<BoolConstInst>(boolVal, m_currentBB);
        operand->setup_def_use();
        auto putInst =
            std::make_shared<PutInst>(std::move(operand), m_currentBB);
        putInst->setup_def_use();
        m_currentBB->pushInst(std::move(putInst));
    }
    else
    {
        size_t pos = temp.find('[');
        if (pos != std::string::npos){
            //auto inst = popInst();
            std::string baseName = temp.substr(0, pos);
            auto targetInstName = inst->getString();
            auto targetInst = std::make_shared<IdentInst>(
                std::move(targetInstName), m_currentBB);
            targetInst->setup_def_use();
            auto putInst =
                std::make_shared<PutInst>(std::move(targetInst), m_currentBB);
            putInst->setup_def_use();
            m_currentBB->pushInst(std::move(putInst));
        } else {
            // The string is not in the array format.
            try {
                size_t chars_processed = 0;
                // Attempt to convert the string to an integer
                int integer_value = std::stoi(temp, &chars_processed);

                // This is the crucial check: was the ENTIRE string used for the conversion?
                if (chars_processed == temp.length()) {
                    // IF TRUE: The string is a valid integer.
                  auto operand = std::make_shared<IntConstInst>(integer_value,
                                                                m_currentBB);
                    operand->setup_def_use();
                  auto inst = std::make_shared<PutInst>(std::move(operand),
                                                        m_currentBB);
                    inst->setup_def_use();
                    m_currentBB->pushInst(inst);

                } else {
                    // IF FALSE: The string started with a number but contains other text (e.g., "543abc").
                    // We treat this as an identifier.
                    throw std::runtime_error("put operand should be either number or valid identifier");
                }

            } catch (const std::invalid_argument& e) {
                // CATCH: An exception was thrown because the string does not start with a number (e.g., "hello").
                // This is clearly an identifier.
                if (temp[0] == '\"')
                {
                    // this is a string const
                    auto operand =
                        std::make_shared<StrConstInst>(temp, m_currentBB);
                    operand->setup_def_use();
                    auto inst = std::make_shared<PutInst>(std::move(operand),
                                                          m_currentBB);
                    inst->setup_def_use();
                    m_currentBB->pushInst(inst);
                }
                else
                {
                    auto operand = std::make_shared<IdentInst>(
                        m_ssa.getCurrentSSAName(temp), m_currentBB);
                    operand->setup_def_use();
                    auto inst = std::make_shared<PutInst>(std::move(operand),
                                                          m_currentBB);
                    inst->setup_def_use();
                    m_currentBB->pushInst(inst);
                }
            } catch (const std::out_of_range& e) {
                throw std::runtime_error("index was out of range for an int");
            }
        }
    }

    auto outputs = v.getOutputs();
    if (outputs)
    {
        outputs->accept(*this);
    }
}

void IRVisitor::visit(InputAST& v)
{
    auto input = v.getInput();
    std::string name = input->getName();
    m_temp.push(name);
    auto target = m_ssa.baseNameToSSA(name);
    auto inst = std::make_shared<IdentInst>(target, m_currentBB);
    inst->setup_def_use();
    m_instStack.push(inst);
    m_ssa.writeVariable(name, m_currentBB, inst);
}

void IRVisitor::visit(InputsAST& v)
{
    auto input = v.getInput();
    input->accept(*this);
    auto temp = popTemp();
    auto inst = popInst();

    auto getInst = std::make_shared<GetInst>(std::move(inst), m_currentBB);
    getInst->setup_def_use();
    m_currentBB->pushInst(std::move(getInst));
    auto inputs = v.getInputs();
    if (inputs)
    {
        inputs->accept(*this);
    }
}

void IRVisitor::visit(IfAST& v)
{
    auto ifExprLabel = "ifExprBlock_" + std::to_string(m_labelCounter);
    auto ifExprBB = std::make_shared<BasicBlock>(ifExprLabel);
    auto jumpInst = std::make_shared<JumpInst>(ifExprBB);
    jumpInst->setup_def_use();
    m_currentBB->pushInst(std::move(jumpInst));
    m_currentBB->pushSuccessor(ifExprBB);
    ifExprBB->pushPredecessor(m_currentBB);

    m_ssa.sealBlock(m_currentBB);
    m_currentBB = ifExprBB;

    auto expr = v.getCondition();
    auto thenArm = v.getThen();
    auto elseArm = v.getElse();

    expr->accept(*this);

    auto exprInst = popInst();

    auto thenBlockLabel = "thenBlock_" + std::to_string(m_labelCounter);
    auto thenBB = std::make_shared<BasicBlock>(thenBlockLabel);
    thenBB->pushPredecessor(m_currentBB);

    auto elseBlockLabel = "elseBlock_" + std::to_string(m_labelCounter);
    auto elseBB = std::make_shared<BasicBlock>(elseBlockLabel);
    elseBB->pushPredecessor(m_currentBB);

    auto mergeBlockLabel = "mergeBlock_" + std::to_string(m_labelCounter);
    auto mergeBB = std::make_shared<BasicBlock>(mergeBlockLabel);
    mergeBB->pushPredecessor(thenBB);
    mergeBB->pushPredecessor(elseBB);

    thenBB->pushSuccessor(mergeBB);
    elseBB->pushSuccessor(mergeBB);

    auto branchInst =
        std::make_shared<BRTInst>(exprInst, thenBB, elseBB, m_currentBB);
    branchInst->setup_def_use();
    m_currentBB->pushInst(branchInst);
    m_currentBB->pushSuccessor(thenBB);
    m_currentBB->pushSuccessor(elseBB);


    m_ssa.sealBlock(m_currentBB);
    m_currentBB = thenBB;

    thenArm->accept(*this);

    auto otherjumpInst = std::make_shared<JumpInst>(mergeBB);
    otherjumpInst->setup_def_use();
    m_currentBB->pushInst(otherjumpInst);
    m_currentBB->pushSuccessor(mergeBB);

    m_ssa.sealBlock(m_currentBB);
    m_currentBB = elseBB;

    if (elseArm)
    {
        elseArm->accept(*this);
    }
    auto moreJumpInst = std::make_shared<JumpInst>(mergeBB);
    moreJumpInst->setup_def_use();
    m_currentBB->pushInst(moreJumpInst);
    m_currentBB->pushSuccessor(mergeBB);

    m_ssa.sealBlock(m_currentBB);
    m_currentBB = mergeBB;

    ++m_labelCounter;
}

void IRVisitor::visit(RepeatUntilAST& v)
{
    auto label = "repeatUntilBlock_" + std::to_string(m_labelCounter++);
    auto repeatUntilBB = std::make_shared<BasicBlock>(label);
    auto jumpInst = std::make_shared<JumpInst>(repeatUntilBB);
    jumpInst->setup_def_use();
    m_currentBB->pushInst(std::move(jumpInst));

    m_currentBB->pushSuccessor(repeatUntilBB);
    repeatUntilBB->pushPredecessor(m_currentBB);
    repeatUntilBB->pushPredecessor(repeatUntilBB);
    repeatUntilBB->pushSuccessor(repeatUntilBB);
    m_ssa.sealBlock(m_currentBB);
    m_currentBB = repeatUntilBB;

    auto statements = v.getStatements();
    auto expr = v.getExitCond();
    statements->accept(*this);
    expr->accept(*this);

    auto cond = popInst();
    std::string newBBName =
        "repeatUntilBlock_" + std::to_string(m_labelCounter - 1) + "_exit";
    auto repeatUntilExitBB = std::make_shared<BasicBlock>(newBBName);
    auto newJumpInst = std::make_shared<BRFInst>(std::move(cond), repeatUntilBB, repeatUntilExitBB, m_currentBB);
    newJumpInst->setup_def_use();
    m_currentBB->pushInst(std::move(newJumpInst));
    m_currentBB->pushSuccessor(repeatUntilExitBB);
    repeatUntilExitBB->pushPredecessor(m_currentBB);
    repeatUntilExitBB->pushSuccessor(repeatUntilExitBB);
    m_ssa.sealBlock(m_currentBB);
    m_currentBB = repeatUntilExitBB;
}

void IRVisitor::visit(LoopAST& v)
{
    auto label = "Loop_" + std::to_string(m_labelCounter++) + ":";

    auto statements = v.getStatements();
    v.accept(*this);

}

void IRVisitor::visit(ExitAST& v) { }
void IRVisitor::visit(ReturnAST& v)
{
    auto retExpr = v.getRetExpr();
    retExpr->accept(*this);
    auto inst = popInst();
    auto pushInst = std::make_shared<PushInst>(inst, m_currentBB);
    pushInst->setup_def_use();
    m_currentBB->pushInst(pushInst);
    auto retInst = std::make_shared<ReturnInst>(m_currentBB);
    retInst->setup_def_use();
    m_currentBB->pushInst(retInst);
}

void IRVisitor::visit(ArrAccessAST& v)
{
    auto subsExpr = v.getSubsExpr();
    subsExpr->accept(*this);

    auto idxInst = popInst();
    auto& baseName = v.getName();
    auto readVal = m_ssa.readVariable(baseName, m_currentBB);
    auto idxStr = popTemp();

    auto st = baseName + "[" + idxStr + "]";

    auto sourceSSAName = m_ssa.getCurrentSSAName(baseName);
    auto targetSSAName = getCurrentTemp();
    pushCurrentTemp();
    auto sourceInst =
        std::make_shared<IdentInst>(std::move(sourceSSAName), m_currentBB);
    sourceInst->setup_def_use();
    auto targetInst =
        std::make_shared<IdentInst>(std::move(targetSSAName), m_currentBB);
    targetInst->setup_def_use();

    m_instStack.push(targetInst);
    auto arrAccInst = std::make_shared<ArrAccessInst>(
        std::move(targetInst), std::move(readVal), std::move(idxInst),
        m_currentBB, v.getType());
    arrAccInst->setup_def_use();
    m_temp.push(st);
    m_currentBB->pushInst(arrAccInst);
}

void IRVisitor::visit(ArgumentsAST& v)
{
    auto expr = v.getExpr();
    auto args = v.getArgs();
    if (expr)
    {
      expr->accept(*this);
      m_arguments.push_back(popInst());
      m_argNames.push_back(popTemp());
    }

    if (args)
    {
        args->accept(*this);
    }
}

void IRVisitor::visit(CallAST& v)
{
    m_arguments = {};
    m_argNames = {};

    auto funcName = v.getFuncName();
    auto args = v.getArgs();
    if (args)
    {
       args->accept(*this);
    }

    auto callInst = std::make_shared<CallInst>(funcName, m_arguments, m_currentBB);
    callInst->setup_def_use();
    m_currentBB->pushInst(callInst);
}

void IRVisitor::visit(FactorAST& v)
{
    auto op = v.getOp().getLexme();
    v.getFactor()->accept(*this);
    auto temp = popTemp();
    auto inst = popInst();

    auto currentTempStr = getCurrentTemp();
    auto currentTempInst =
        std::make_shared<IdentInst>(std::move(currentTempStr), m_currentBB);
    currentTempInst->setup_def_use();
    m_instStack.push(currentTempInst);

    pushCurrentTemp();


    if (op == "-")
    {
        auto newInst = std::make_shared<MulInst>(
            currentTempInst, std::make_shared<IntConstInst>(-1, m_currentBB), inst, m_currentBB);
        newInst->setup_def_use();
        m_currentBB->pushInst(newInst);
    }
    else if (op == "~")
    {
        auto newInst = std::make_shared<NotInst>(
            currentTempInst, inst, m_currentBB);
        newInst->setup_def_use();
        m_currentBB->pushInst(newInst);
    }
}

void IRVisitor::visit(FactorsAST& v)
{
    auto op = v.getOp().getLexme();
    auto factor = v.getFactor();
    factor->accept(*this);

    auto right = popTemp();
    auto left = popTemp();

    auto rightInst = popInst();
    auto leftInst = popInst();
    auto currentTempStr = getCurrentTemp();
    auto currentTempInst =
        std::make_shared<IdentInst>(std::move(currentTempStr), m_currentBB);
    currentTempInst->setup_def_use();
    m_instStack.push(currentTempInst);

    pushCurrentTemp();

    if (op == "*")
    {

        auto inst =
            std::make_shared<MulInst>(std::move(currentTempInst),
                                            std::move(leftInst),
                                            std::move(rightInst), m_currentBB);
          inst->setup_def_use();
          m_currentBB->pushInst(inst);
    }
    else if (op == "/")
    {
        auto inst =
            std::make_shared<DivInst>(std::move(currentTempInst),
                                            std::move(leftInst),
                                            std::move(rightInst), m_currentBB);
        inst->setup_def_use();
        m_currentBB->pushInst(inst);
    }
    else
    {
        auto inst =
            std::make_shared<AndInst>(std::move(currentTempInst),
                                            std::move(leftInst),
                                            std::move(rightInst), m_currentBB);
        inst->setup_def_use();
        m_currentBB->pushInst(inst);
    }

    auto factors = v.getFactors();
    if (factors)
    {
        factors->accept(*this);
    }
}

void IRVisitor::visit(TermAST& v)
{
    auto term = v.getFactor();
    if (term)
    {
        term->accept(*this);
    }
    else
    {
        return;
    }
    auto factors = v.getFactors();
    if (factors)
    {
        factors->accept(*this);
    }
}

void IRVisitor::visit(TermsAST& v)
{
    auto op = v.getOp().getLexme();
    v.getTerm()->accept(*this);

    auto right = popTemp();
    auto left = popTemp();

    auto rightInst = popInst();
    auto leftInst = popInst();
    auto currentTempStr = getCurrentTemp();
    auto currentTempInst =
        std::make_shared<IdentInst>(currentTempStr, m_currentBB);
    currentTempInst->setup_def_use();
    m_instStack.push(currentTempInst);

    pushCurrentTemp();

    if (op == "+")
    {
        auto inst =
            std::make_shared<AddInst>(currentTempInst, leftInst,
                                            rightInst, m_currentBB);
        inst->setup_def_use();
        m_ssa.writeVariable(currentTempStr, m_currentBB, inst);
        m_currentBB->pushInst(inst);
    }
    else if (op == "-")
    {
        auto inst =
            std::make_shared<SubInst>(currentTempInst, leftInst,
                                            rightInst, m_currentBB);
        inst->setup_def_use();
        m_ssa.writeVariable(currentTempStr, m_currentBB, inst);
        m_currentBB->pushInst(inst);
    }
    else
    {
        auto inst =
            std::make_shared<OrInst>(currentTempInst, leftInst, rightInst,
                                           m_currentBB);
        inst->setup_def_use();
        m_ssa.writeVariable(currentTempStr, m_currentBB, inst);
        m_currentBB->pushInst(inst);
    }

    auto terms = v.getTerms();
    if (terms)
    {
        terms->accept(*this);
    }

}

void IRVisitor::visit(SimpleExprAST& v)
{
    if (v.getTerm()) v.getTerm()->accept(*this);
    auto terms = v.getTerms();
    if (terms) terms->accept(*this);
}

void IRVisitor::visit(OptRelationAST& v)
{
    auto op = v.getOp().getLexme();
    v.getTerms()->accept(*this);
    auto currentTempStr = getCurrentTemp();
    auto targetInst = std::make_shared<IdentInst>(currentTempStr, m_currentBB);
    targetInst->setup_def_use();

    auto rightInst = popInst();
    auto leftInst = popInst();
    m_instStack.push(targetInst);
    auto rightTemp = popTemp();
    auto leftTemp = popTemp();

    if (op == "=")
    {
        auto compInst = std::make_shared<CmpEQInst>(
            std::move(targetInst), std::move(leftInst), std::move(rightInst),
            m_currentBB);
        compInst->setup_def_use();
        m_ssa.writeVariable(currentTempStr, m_currentBB, compInst);
        m_currentBB->pushInst(compInst);
    }
    else if (op == "!=")
    {
        auto compInst = std::make_shared<CmpNEInst>(
            std::move(targetInst), std::move(leftInst), std::move(rightInst),
            m_currentBB);
        compInst->setup_def_use();
        m_ssa.writeVariable(currentTempStr, m_currentBB, compInst);
        m_currentBB->pushInst(compInst);
    }
    else if (op == "<")
    {
        auto compInst = std::make_shared<CmpLTInst>(
            std::move(targetInst), std::move(leftInst), std::move(rightInst),
            m_currentBB);
        compInst->setup_def_use();
        m_ssa.writeVariable(currentTempStr, m_currentBB, compInst);
        m_currentBB->pushInst(compInst);
    }
    else if (op == "<=")
    {
        auto compInst = std::make_shared<CmpLTEInst>(
            std::move(targetInst), std::move(leftInst), std::move(rightInst),
            m_currentBB);
        compInst->setup_def_use();
        m_ssa.writeVariable(currentTempStr, m_currentBB, compInst);
        m_currentBB->pushInst(compInst);

    }
    else if (op == ">")
    {
        auto compInst = std::make_shared<CmpGTInst>(
            std::move(targetInst), std::move(leftInst), std::move(rightInst),
            m_currentBB);
        compInst->setup_def_use();
        m_ssa.writeVariable(currentTempStr, m_currentBB, compInst);
        m_currentBB->pushInst(compInst);

    }
    else if (op == ">=")
    {
        auto compInst = std::make_shared<CmpGTEInst>(
            std::move(targetInst), std::move(leftInst), std::move(rightInst),
            m_currentBB);
        compInst->setup_def_use();
        m_ssa.writeVariable(currentTempStr, m_currentBB, compInst);
        m_currentBB->pushInst(compInst);
    }
    else
    {
        throw std::runtime_error("Unknown relational operator!");
    }
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
    auto baseName = v.getIdentifier()->getName();
    
    auto ssaName = m_ssa.baseNameToSSA(baseName);
    auto targetIdentInst = std::make_shared<IdentInst>(ssaName, m_currentBB);
    targetIdentInst->setup_def_use();

    auto type = v.getIdentifier()->getType();
    if (type == Type::BOOLEAN)
    {
        auto boolConstInst = std::make_shared<BoolConstInst>(false, m_currentBB);
        boolConstInst->setup_def_use();
        auto inst = std::make_shared<AssignInst>(
            std::move(targetIdentInst), std::move(boolConstInst), m_currentBB);
        inst->setup_def_use();
        m_ssa.writeVariable(baseName, m_currentBB, inst);
        m_currentBB->pushInst(inst);
    }
    else if (type == Type::INTEGER)
    {
        auto intConstInst = std::make_shared<IntConstInst>(0, m_currentBB);
        intConstInst->setup_def_use();

        auto inst = std::make_shared<AssignInst>(std::move(targetIdentInst), std::move(intConstInst), m_currentBB);
        inst->setup_def_use();

        m_ssa.writeVariable(baseName, m_currentBB, inst);
        m_currentBB->pushInst(inst);
    }
    else
    {
        throw std::runtime_error("Unknown type");
    }
}

void IRVisitor::visit(ArrDeclAST& v)
{
    std::string& baseName = v.getIdentifier()->getName();
    auto ssaName = m_ssa.getCurrentSSAName(baseName);
    auto allocaIdentInst = std::make_shared<IdentInst>(ssaName, m_currentBB);
    allocaIdentInst->setup_def_use();

    auto type = v.getIdentifier()->getType();
    auto size = v.getSize();
    auto allocaInst = std::make_shared<AllocaInst>(std::move(allocaIdentInst),
                                                   type, size, m_currentBB);
    allocaInst->setup_def_use();
    m_ssa.writeVariable(baseName, m_currentBB, allocaInst);

    m_currentBB->pushInst(allocaInst);

    for (unsigned int i = 0; i < v.getSize(); ++i)
    {
        std::string& baseName = v.getIdentifier()->getName();
        auto ssaName = m_ssa.getCurrentSSAName(baseName);
        auto sourceIdentInst =
            std::make_shared<IdentInst>(ssaName, m_currentBB);
        sourceIdentInst->setup_def_use();

        auto indexInst = std::make_shared<IntConstInst>((int)i, m_currentBB);

        auto targetSsaName = m_ssa.baseNameToSSA(baseName);
        auto targetIdentInst =
            std::make_shared<IdentInst>(targetSsaName, m_currentBB);
        targetIdentInst->setup_def_use();

        auto type = v.getIdentifier()->getType();
        if (type == Type::BOOLEAN)
        {
          auto boolConstInst =
              std::make_shared<BoolConstInst>(false, m_currentBB);
            boolConstInst->setup_def_use();
            auto inst = std::make_shared<ArrUpdateInst>(
                std::move(targetIdentInst), std::move(sourceIdentInst),
                std::move(indexInst), std::move(boolConstInst), m_currentBB, Type::BOOLEAN);
            inst->setup_def_use();
            m_ssa.writeVariable(baseName, m_currentBB, inst);

            m_currentBB->pushInst(inst);
        }
        else if (type == Type::INTEGER)
        {
            auto intConstInst = std::make_shared<IntConstInst>(0, m_currentBB);
            intConstInst->setup_def_use();

            auto inst = std::make_shared<ArrUpdateInst>(
                std::move(targetIdentInst), std::move(sourceIdentInst),
                std::move(indexInst), std::move(intConstInst), m_currentBB, Type::INTEGER);
            inst->setup_def_use();
            m_ssa.writeVariable(baseName, m_currentBB, inst);

            m_currentBB->pushInst(inst);
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
    auto ident = v.getIdentifier();
    auto identType = ident->getType();
    m_parameters.push_back(ident);

    // curerntBB here is the basic block for the function declaration
    std::string& identName = ident->getName();
    auto identInst = std::make_shared<IdentInst>(m_ssa.baseNameToSSA(identName),
                                                 m_currentBB);
    // Each parameter is a definition.
    m_ssa.writeVariable(m_ssa.baseNameToSSA(identName), m_currentBB,
                        identInst);
}

void IRVisitor::visit(ParametersAST& v)
{
    auto parameter = v.getParam();
    auto parameters = v.getParams();
    parameter->accept(*this);
    if (parameters)
    {
        parameters->accept(*this);
    }
}

void IRVisitor::visit(ProcDeclAST& v)
{
    auto procName = v.getProcName();
    std::string bbName = procName;

    auto basicBlock = std::make_shared<BasicBlock>(bbName);
    m_funcBB[bbName] = basicBlock;
    std::shared_ptr<BasicBlock> oldBB = m_currentBB;
    m_currentBB = basicBlock;


    auto params = v.getParams();
    auto scope = v.getScope();

    m_parameters = {};
    if (params)
    {
        params->accept(*this);
    }

    auto lowerFunc = std::make_shared<LowerFunc>(
        procName, FType::PROC, Type::UNDEFINED, m_parameters, m_currentBB);
    lowerFunc->setup_def_use();
    m_currentBB->pushInst(lowerFunc);

    // lower param
    for (int i=0; i < m_parameters.size(); ++i)
    {
        auto& param = m_parameters[i];
        auto identType = param->getIdentType();
        if (identType == IdentType::VARIABLE)
        {
            auto variable = std::dynamic_pointer_cast<VariableAST>(param);
        }
        else
        {
            throw std::runtime_error("array parameter is not implemented yet!\n");
        }
    }

    scope->accept(*this);
    m_currentBB = oldBB;

    auto funcSignature = std::make_shared<FuncSignature>(
        procName, FType::PROC, Type::UNDEFINED, m_parameters, m_currentBB);
    m_currentBB->pushInst(funcSignature);
    //m_cg.generateFuncNode(procName, false, m_parameters.size());
    return;
}

void IRVisitor::visit(FuncDeclAST& v)
{
    auto funcName = v.getFuncName();
    std::string bbName = funcName;
    
    auto basicBlock = std::make_shared<BasicBlock>(bbName);
    m_funcBB[bbName] = basicBlock;
    auto oldBB = m_currentBB;
    m_currentBB = basicBlock;
    auto params = v.getParams();
    auto scope = v.getScope();
    if (params)
    {
        params->accept(*this);
    }
    scope->accept(*this);
    m_currentBB = oldBB;
}

std::string IRVisitor::getCurrentTemp()
{
  return "t" + std::to_string(m_tempCounter);
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
        std::cerr << "empty!\n";
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

std::shared_ptr<Inst> IRVisitor::popInst()
{
    if (m_instStack.size() == 0)
    {
        std::cerr << "instruction stack is empty!\n";
        exit(1);
    }
    std::shared_ptr<Inst> inst = m_instStack.top();
    m_instStack.pop();
    return inst;
}
