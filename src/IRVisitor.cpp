#include "IRVisitor.hpp"
#include "Ast.hpp"
#include "Inst.hpp"
#include <queue>
#include <set>
#include <memory>

IRVisitor::IRVisitor() : m_tempCounter(0), m_labelCounter(0)
{
    m_cfg = std::make_shared<BasicBlock>("Entry_0");
    m_currBBNameWithoutCtr = "Entry";
    m_currBBCtr = 0;
    m_currentBB = m_cfg;
    //m_sealedBlocks.insert(m_currentBB);
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
    auto inst = std::make_shared<IntConstInst>(val);
    inst->setup_def_use();
    m_instStack.push(inst);
}

void IRVisitor::visit(BoolAST& v) {
    auto val = v.getVal();
    auto inst = std::make_shared<BoolConstInst>(val);
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
        auto inst = std::make_shared<StrConstInst>(str);
        inst->setup_def_use();
        m_instStack.push(inst);
    }
    else
    {
        auto newVal = "\"" + val + "\"";
        m_temp.push(newVal);
        auto inst = std::make_shared<StrConstInst>(newVal);
        inst->setup_def_use();
        m_instStack.push(inst);
    }
}

void IRVisitor::visit(VariableAST& v)
{
    auto val = v.getName();
    auto valInst = readVariable(val, m_currentBB);
    m_temp.push(val);
    m_instStack.push(valInst);
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

    std::cout << targetStr << " <- " << exprTemp;
    std::cout << std::endl;

    if (identifier->getIdentType() == IdentType::ARRAY)
    {
        auto sourceSSAName = getCurrentSSAName(targetStr);
        auto targetSSAName = baseNameToSSA(targetStr);
        auto sourceInst = readVariable(targetStr, m_currentBB);
        auto targetInst = std::make_shared<IdentInst>(targetSSAName);
        targetInst->setup_def_use();

        auto arrIdentifier =
            std::dynamic_pointer_cast<ArrAccessAST>(identifier);
        auto subsExpr = arrIdentifier->getSubsExpr();
        subsExpr->accept(*this);
        auto subsInst = popInst();
        auto arrayUpdateInst = std::make_shared<ArrUpdateInst>(
            targetInst, sourceInst, subsInst, exprInst);
        arrayUpdateInst->setup_def_use();
        writeVariable(targetStr, m_currentBB, arrayUpdateInst);
        m_currentBB->pushInst(arrayUpdateInst);
    }
    else if (identifier->getIdentType() == IdentType::VARIABLE)
    {
        auto targetSSAName = baseNameToSSA(targetStr);
        auto targetSSAInst = std::make_shared<IdentInst>(targetSSAName);
        targetSSAInst->setup_def_use();
        auto assignInst = std::make_shared<AssignInst>(targetSSAInst,
                                                       exprInst);
        assignInst->setup_def_use();
        writeVariable(targetStr, m_currentBB, assignInst);
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
    std::cout << "put(";
    output->accept(*this);
    auto temp = popTemp();

    auto inst = popInst();
    std::cout << temp;
    std::cout << ")\n";
    
    if (temp == "\'\\n\'")
    {
        auto operand = std::make_shared<StrConstInst>("\'\\n\'");
        operand->setup_def_use();
        auto putInst = std::make_shared<PutInst>(std::move(operand));
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
            auto targetInst =
                std::make_shared<IdentInst>(std::move(targetInstName));
            targetInst->setup_def_use();
            auto putInst = std::make_shared<PutInst>(std::move(targetInst));
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
                    auto operand = std::make_shared<IntConstInst>(integer_value);
                    operand->setup_def_use();
                    auto inst = std::make_shared<PutInst>(std::move(operand));
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
                    auto operand =
                        std::make_shared<StrConstInst>(temp);
                    operand->setup_def_use();
                    auto inst = std::make_shared<PutInst>(std::move(operand));
                    inst->setup_def_use();
                    m_currentBB->pushInst(inst);
                }
                else
                {
                    auto operand =
                        std::make_shared<IdentInst>(getCurrentSSAName(temp));
                    operand->setup_def_use();
                    auto inst = std::make_shared<PutInst>(std::move(operand));
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
    auto target = baseNameToSSA(name);
    auto inst = std::make_shared<IdentInst>(target);
    inst->setup_def_use();
    m_instStack.push(inst);
    writeVariable(name, m_currentBB, inst);
}

void IRVisitor::visit(InputsAST& v)
{
    auto input = v.getInput();
    std::cout << "get(";
    input->accept(*this);
    auto temp = popTemp();
    std::cout << temp;
    std::cout << ")\n";
    auto inst = popInst();

    auto getInst = std::make_shared<GetInst>(std::move(inst));
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
    std::cout << ifExprLabel << ":" << std::endl;   
    auto ifExprBB = std::make_shared<BasicBlock>(ifExprLabel);
    auto jumpInst = std::make_shared<JumpInst>(ifExprBB);
    jumpInst->setup_def_use();
    m_currentBB->pushInst(std::move(jumpInst));
    m_currentBB->pushSuccessor(ifExprBB);
    ifExprBB->pushPredecessor(m_currentBB);

    sealBlock(m_currentBB);
    m_currentBB = ifExprBB;
    //sealBlock(m_currentBB);

    auto expr = v.getCondition();
    auto thenArm = v.getThen();
    auto elseArm = v.getElse();

    expr->accept(*this);

    auto exprInst = popInst();

    auto thenBlockLabel = "thenBlock_" + std::to_string(m_labelCounter);
    auto thenBB = std::make_shared<BasicBlock>(thenBlockLabel);
    thenBB->pushPredecessor(m_currentBB);

    //auto elseLabel = "elseBlock_" + std::to_string(m_labelCounter) + ":";
    auto elseBlockLabel = "elseBlock_" + std::to_string(m_labelCounter);
    auto elseBB = std::make_shared<BasicBlock>(elseBlockLabel);
    elseBB->pushPredecessor(m_currentBB);

    auto mergeBlockLabel = "mergeBlock_" + std::to_string(m_labelCounter);
    auto mergeBB = std::make_shared<BasicBlock>(mergeBlockLabel);
    mergeBB->pushPredecessor(thenBB);
    mergeBB->pushPredecessor(elseBB);

    thenBB->pushSuccessor(mergeBB);
    elseBB->pushSuccessor(mergeBB);

    auto branchInst = std::make_shared<BRTInst>(exprInst, thenBB, elseBB);
    branchInst->setup_def_use();
    m_currentBB->pushInst(branchInst);
    m_currentBB->pushSuccessor(thenBB);
    m_currentBB->pushSuccessor(elseBB);

    std::cout << "if(!" << popTemp();
    std::cout << ") goto ";
    if (elseArm)
    {
        std::cout << elseBlockLabel << std::endl;
    }
    else
    {
        std::cout << mergeBlockLabel << std::endl;
    }

    sealBlock(m_currentBB);
    m_currentBB = thenBB;
    //sealBlock(m_currentBB);

    thenArm->accept(*this);

    std::cout << "goto " << mergeBlockLabel << std::endl;

    auto otherjumpInst = std::make_shared<JumpInst>(mergeBB);
    otherjumpInst->setup_def_use();
    m_currentBB->pushInst(otherjumpInst);
    m_currentBB->pushSuccessor(mergeBB);

    sealBlock(m_currentBB);
    m_currentBB = elseBB;
    //sealBlock(m_currentBB);

    if (elseArm)
    {
        std::cout << elseBlockLabel << std::endl;
        elseArm->accept(*this);
    }
    auto moreJumpInst = std::make_shared<JumpInst>(mergeBB);
    moreJumpInst->setup_def_use();
    m_currentBB->pushInst(moreJumpInst);
    m_currentBB->pushSuccessor(mergeBB);

    sealBlock(m_currentBB);
    m_currentBB = mergeBB;
    //sealBlock(m_currentBB);

    std::cout << mergeBlockLabel << std::endl;
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
    sealBlock(m_currentBB);
    m_currentBB = repeatUntilBB;
    //sealBlock(m_currentBB);

    std::cout << '\n' << label << std::endl;
    auto statements = v.getStatements();
    auto expr = v.getExitCond();
    statements->accept(*this);
    expr->accept(*this);
    std::cout << "if(!" << popTemp();
    std::cout << ") goto " << label << std::endl << std::endl;

    auto cond = popInst();
    std::string newBBName =
        "repeatUntilBlock_" + std::to_string(m_labelCounter - 1) + "_exit";
    auto repeatUntilExitBB = std::make_shared<BasicBlock>(newBBName);
    auto newJumpInst = std::make_shared<BRFInst>(std::move(cond), repeatUntilBB,
                                                 repeatUntilExitBB);
    newJumpInst->setup_def_use();
    m_currentBB->pushInst(std::move(newJumpInst));
    m_currentBB->pushSuccessor(repeatUntilExitBB);
    repeatUntilExitBB->pushPredecessor(m_currentBB);
    repeatUntilExitBB->pushSuccessor(repeatUntilExitBB);
    sealBlock(m_currentBB);
    m_currentBB = repeatUntilExitBB;
    //sealBlock(m_currentBB);
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
    auto inst = popInst();
    auto pushInst = std::make_shared<PushInst>(inst);
    pushInst->setup_def_use();
    m_currentBB->pushInst(pushInst);
    auto retInst = std::make_shared<ReturnInst>();
    retInst->setup_def_use();
    m_currentBB->pushInst(retInst);

    std::cout << "return " << popTemp() << std::endl;
}

void IRVisitor::visit(ArrAccessAST& v)
{
    auto subsExpr = v.getSubsExpr();
    subsExpr->accept(*this);

    auto idxInst = popInst();
    auto& baseName = v.getName();
    auto readVal = readVariable(baseName, m_currentBB);
    auto idxStr = popTemp();

    auto st = baseName + "[" + idxStr + "]";

    auto sourceSSAName = getCurrentSSAName(baseName);
    auto targetSSAName = getCurrentTemp();
    pushCurrentTemp();
    auto sourceInst = std::make_shared<IdentInst>(std::move(sourceSSAName));
    sourceInst->setup_def_use();
    auto targetInst = std::make_shared<IdentInst>(std::move(targetSSAName));
    targetInst->setup_def_use();

    m_instStack.push(targetInst);
    auto arrAccInst = std::make_shared<ArrAccessInst>(
        std::move(targetInst), std::move(readVal), std::move(idxInst));
    arrAccInst->setup_def_use();
    m_temp.push(st);
    m_currentBB->pushInst(arrAccInst);
}

void IRVisitor::visit(ArgumentsAST& v)
{
    auto expr = v.getExpr();
    auto args = v.getArgs();
    expr->accept(*this);
    auto inst = popInst();

    auto pushInst = std::make_shared<PushInst>(inst);
    pushInst->setup_def_use();

    m_argNames.push_back(popTemp());
    m_arguments.push_back(pushInst);

    if (args)
    {
        args->accept(*this);
    }
}

void IRVisitor::visit(CallAST& v)
{
    m_arguments = std::vector<std::shared_ptr<Inst>>();
    m_argNames = std::vector<std::string>();

    auto args = v.getArgs();
    if (args)
    {
       args->accept(*this);
    }

    std::reverse(m_arguments.begin(), m_arguments.end());
    std::reverse(m_argNames.begin(), m_argNames.end());

    for (auto& arg : m_arguments)
    {
        m_currentBB->pushInst(arg);
    }

    for (auto& argName : m_argNames)
    {
        std::cout << " push " << argName << std::endl;
    }

    auto funcName = v.getFuncName();

    auto callInst = std::make_shared<CallInst>(funcName);
    callInst->setup_def_use();
    m_currentBB->pushInst(callInst);
    m_currentBB->pushSuccessor(m_funcBB[funcName]);
    m_funcBB[funcName]->pushPredecessor(m_currentBB);

     auto newBBLabel =
         m_currBBNameWithoutCtr + "_" + std::to_string(++m_currBBCtr);
     auto newBB = std::make_shared<BasicBlock>(newBBLabel);
     newBB->pushPredecessor(m_funcBB[funcName]);
     m_funcBB[funcName]->pushSuccessor(newBB);
     m_currentBB = newBB;

     auto temp = getCurrentTemp();
     pushCurrentTemp();
     std::cout << temp << " <- " << "call " << funcName << std::endl;

     auto tempInst = std::make_shared<IdentInst>(temp);
     tempInst->setup_def_use();
     auto popInst = std::make_shared<PopInst>(tempInst);
     popInst->setup_def_use();
     m_currentBB->pushInst(popInst);
     m_instStack.push(tempInst);
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

    auto rightInst = popInst();
    auto leftInst = popInst();
    auto currentTempStr = getCurrentTemp();
    auto currentTempInst =
        std::make_shared<IdentInst>(std::move(currentTempStr));
    currentTempInst->setup_def_use();
    m_instStack.push(currentTempInst);

    generateCurrentTemp();
    pushCurrentTemp();

    std::cout << " <- ";
    std::cout << left << " " << op << " " << right << std::endl;

    if (op == "*")
    {

        auto inst =
            std::make_shared<MulInst>(std::move(currentTempInst),
                                    std::move(leftInst), std::move(rightInst));
          inst->setup_def_use();
          m_currentBB->pushInst(inst);
    }
    else if (op == "/")
    {
        auto inst =
            std::make_shared<DivInst>(std::move(currentTempInst),
                                    std::move(leftInst), std::move(rightInst));
        inst->setup_def_use();
        m_currentBB->pushInst(inst);
    }
    else
    {
        auto inst =
            std::make_shared<AndInst>(std::move(currentTempInst),
                                    std::move(leftInst), std::move(rightInst));
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

    auto rightInst = popInst();
    auto leftInst = popInst();
    auto currentTempStr = getCurrentTemp();
    auto currentTempInst =
        std::make_shared<IdentInst>(currentTempStr);
    currentTempInst->setup_def_use();
    m_instStack.push(currentTempInst);

    generateCurrentTemp();
    pushCurrentTemp();

    std::cout << " <- ";
    std::cout << left << " " << op << " " << right << std::endl;

    if (op == "+")
    {
        auto inst =
            std::make_shared<AddInst>(currentTempInst,
                                      leftInst, rightInst);
        inst->setup_def_use();
        writeVariable(currentTempStr, m_currentBB, inst);
        m_currentBB->pushInst(inst);
    }
    else if (op == "-")
    {
        auto inst =
            std::make_shared<SubInst>(currentTempInst,
                                      leftInst, rightInst);
        inst->setup_def_use();
        writeVariable(currentTempStr, m_currentBB, inst);
        m_currentBB->pushInst(inst);
    }
    else
    {
        auto inst =
            std::make_shared<OrInst>(currentTempInst,
                                     leftInst, rightInst);
        inst->setup_def_use();
        writeVariable(currentTempStr, m_currentBB, inst);
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
    v.getTerm()->accept(*this);
    auto terms = v.getTerms();
    if (terms) terms->accept(*this);
}

void IRVisitor::visit(OptRelationAST& v)
{
    auto op = v.getOp().getLexme();
    v.getTerms()->accept(*this);
    generateCurrentTemp();
    auto currentTempStr = getCurrentTemp();
    auto targetInst = std::make_shared<IdentInst>(currentTempStr);
    targetInst->setup_def_use();

    auto rightInst = popInst();
    auto leftInst = popInst();
    m_instStack.push(targetInst);
    std::cout << " <- ";
    auto rightTemp = popTemp();
    auto leftTemp = popTemp();
    std::cout << leftTemp << " " << op << " " << rightTemp << std::endl;
    if (op == "=")
    {
        auto compInst = std::make_shared<CmpEQInst>(
            std::move(targetInst), std::move(leftInst), std::move(rightInst));
      compInst->setup_def_use();
        writeVariable(currentTempStr, m_currentBB, compInst);
        m_currentBB->pushInst(compInst);
    }
    else if (op == "!=")
    {
        auto compInst = std::make_shared<CmpNEInst>(
            std::move(targetInst), std::move(leftInst), std::move(rightInst));
        compInst->setup_def_use();
        writeVariable(currentTempStr, m_currentBB, compInst);
        m_currentBB->pushInst(compInst);
    }
    else if (op == "<")
    {
        auto compInst = std::make_shared<CmpLTInst>(
            std::move(targetInst), std::move(leftInst), std::move(rightInst));
        compInst->setup_def_use();
        writeVariable(currentTempStr, m_currentBB, compInst);
        m_currentBB->pushInst(compInst);
    }
    else if (op == "<=")
    {
        auto compInst = std::make_shared<CmpLTEInst>(
            std::move(targetInst), std::move(leftInst), std::move(rightInst));
        compInst->setup_def_use();
        writeVariable(currentTempStr, m_currentBB, compInst);
        m_currentBB->pushInst(compInst);

    }
    else if (op == ">")
    {
        auto compInst = std::make_shared<CmpGTInst>(
            std::move(targetInst), std::move(leftInst), std::move(rightInst));
        compInst->setup_def_use();
        writeVariable(currentTempStr, m_currentBB, compInst);
        m_currentBB->pushInst(compInst);

    }
    else if (op == ">=")
    {
        auto compInst = std::make_shared<CmpGTEInst>(
            std::move(targetInst), std::move(leftInst), std::move(rightInst));
        compInst->setup_def_use();
        writeVariable(currentTempStr, m_currentBB, compInst);
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
    std::cout << baseName << " <- ";
    
    auto ssaName = baseNameToSSA(std::move(baseName));
    auto targetIdentInst = std::make_shared<IdentInst>(ssaName);
    targetIdentInst->setup_def_use();

    auto type = v.getIdentifier()->getType();
    if (type == Type::BOOLEAN)
    {
        auto boolConstInst = std::make_shared<BoolConstInst>(false);
        boolConstInst->setup_def_use();
        auto inst =
            std::make_shared<AssignInst>(std::move(targetIdentInst), std::move(boolConstInst));
        inst->setup_def_use();
        writeVariable(baseName, m_currentBB, inst);
        std::cout << "false\n";
        m_currentBB->pushInst(inst);
    }
    else if (type == Type::INTEGER)
    {
        auto intConstInst = std::make_shared<IntConstInst>(0);
        intConstInst->setup_def_use();

        auto inst = std::make_shared<AssignInst>(std::move(targetIdentInst),
                                                 std::move(intConstInst));
        inst->setup_def_use();

        writeVariable(baseName, m_currentBB, inst);
        std::cout << "0\n";
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
    auto ssaName = getCurrentSSAName(baseName);
    auto allocaIdentInst = std::make_shared<IdentInst>(ssaName);
    allocaIdentInst->setup_def_use();

    auto type = v.getIdentifier()->getType();
    auto size = v.getSize();
    auto allocaInst =
        std::make_shared<AllocaInst>(std::move(allocaIdentInst), type, size);
    allocaInst->setup_def_use();
    writeVariable(baseName, m_currentBB, allocaInst);

    m_currentBB->pushInst(allocaInst);

    for (unsigned int i = 0; i < v.getSize(); ++i)
    {
        std::string& baseName = v.getIdentifier()->getName();
        auto ssaName = getCurrentSSAName(baseName);
        auto sourceIdentInst = std::make_shared<IdentInst>(ssaName);
        sourceIdentInst->setup_def_use();

        auto indexInst = std::make_shared<IntConstInst>((int)i);

        auto targetSsaName = baseNameToSSA(baseName);
        auto targetIdentInst = std::make_shared<IdentInst>(targetSsaName);
        targetIdentInst->setup_def_use();

        std::cout << v.getIdentifier()->getName() << "[" << i << "]"  " <- ";

        auto type = v.getIdentifier()->getType();
        if (type == Type::BOOLEAN)
        {
            auto boolConstInst = std::make_shared<BoolConstInst>(false);
            boolConstInst->setup_def_use();
            auto inst = std::make_shared<ArrUpdateInst>(
                std::move(targetIdentInst), std::move(sourceIdentInst),
                std::move(indexInst), std::move(boolConstInst));
            inst->setup_def_use();
            writeVariable(baseName, m_currentBB, inst);

            m_currentBB->pushInst(inst);

            std::cout << "false\n";
        }
        else if (type == Type::INTEGER)
        {
            auto intConstInst = std::make_shared<IntConstInst>(0);
            intConstInst->setup_def_use();

            auto inst = std::make_shared<ArrUpdateInst>(
                std::move(targetIdentInst), std::move(sourceIdentInst),
                std::move(indexInst), std::move(intConstInst));
            inst->setup_def_use();
            writeVariable(baseName, m_currentBB, inst);

            m_currentBB->pushInst(inst);

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
    auto ident = v.getIdentifier();
    auto identName = ident->getName();
    auto identInst = std::make_shared<IdentInst>(baseNameToSSA(identName));
    std::cout << identName << " : " << typeToStr(v.getType());
    writeVariable(identName, m_currentBB, identInst);

    auto popInst = std::make_shared<PopInst>(identInst);
    popInst->setup_def_use();
    m_currentBB->pushInst(popInst);
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
    auto procName = v.getProcName();
    std::string bbName = procName;
    std::cout << std::endl << "proc_" << v.getProcName() << "(";

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
    std::cout << ")\n";
    scope->accept(*this);

    // make a push(0) instruction because proc have no return statement,
    // so we return 0 as default
    auto inst = std::make_shared<IntConstInst>(0);
    inst->setup_def_use();
    auto pushInst = std::make_shared<PushInst>(inst);
    pushInst->setup_def_use();
    m_currentBB->pushInst(pushInst);

    auto retInst = std::make_shared<ReturnInst>();
    retInst->setup_def_use();
    m_currentBB->pushInst(retInst);
    std::cout << "return\n";
    std::cout << "endProc\n";
    m_currentBB = oldBB;
}

void IRVisitor::visit(FuncDeclAST& v)
{
    auto funcName = v.getFuncName();
    std::string bbName = funcName;
    std::cout << std::endl << bbName << "(";
    
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
    std::cout << ")\n";
    scope->accept(*this);
    std::cout << "endFunc\n";
    m_currentBB = oldBB;
}

std::string IRVisitor::getCurrentTemp()
{
  return "t" + std::to_string(m_tempCounter);
}

void IRVisitor::generateCurrentTemp() {
    std::cout << getCurrentTemp();
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

std::shared_ptr<Inst> IRVisitor::popInst()
{
    if (m_instStack.size() == 0)
    {
        std::cout << "instruction stack is empty!\n";
        printCFG();
        exit(1);
    }
    std::shared_ptr<Inst> inst = m_instStack.top();
    m_instStack.pop();
    return inst;
}

std::string IRVisitor::baseNameToSSA(const std::string& name)
{
    auto it = m_nameCtr.find(name);
    if (it == m_nameCtr.end())
    {
        m_nameCtr[name] = 0;
        return name + "." + std::to_string(0);
    }
    else
    {
        ++m_nameCtr[name];
        return name + "." + std::to_string(m_nameCtr[name]);
    }
}

std::string IRVisitor::getCurrentSSAName(const std::string& name)
{
    auto it = m_nameCtr.find(name);
    if (it == m_nameCtr.end())
    {
        m_nameCtr[name] = 0;
        return name + "." + std::to_string(0);
    }
    else
    {
        return name + "." + std::to_string(m_nameCtr[name]);
    }
}

void IRVisitor::printCFG()
{
    // --- Step 0: Handle the case of an empty CFG ---
    if (!m_cfg) {
        std::cout << "CFG is empty or not initialized." << std::endl;
        return;
    }

    // --- Step 1: Initialize BFS data structures ---
    std::queue<std::shared_ptr<BasicBlock>> worklist;
    std::set<std::shared_ptr<BasicBlock>> visited;

    // --- Step 2: Start the traversal from the entry block ---
    worklist.push(m_cfg);
    visited.insert(m_cfg);

    std::cout << "--- Control Flow Graph (BFS Traversal) ---" << std::endl;

    // --- Step 3: Loop as long as there are blocks to process ---
    while (!worklist.empty())
    {
        // Get the next block from the front of the queue
        std::shared_ptr<BasicBlock> current_bb = worklist.front();
        worklist.pop();

        // --- Process the current block (print its contents) ---
        std::cout << "\nBasic Block Name: " << current_bb->getName() << std::endl;
        auto instructions = current_bb->getInstructions();
        if (instructions.empty()) {
            std::cout << "  (no instructions)" << std::endl;
        } else {
            for (const auto& inst : instructions) {
                // Indenting instructions makes the output much clearer
                std::cout << "  " << inst->getString() << std::endl;
            }
        }
        
        // --- Step 4: Add its unvisited successors to the queue ---
        for (const auto& successor : current_bb->getSuccessors())
        {
            // The 'find' method on a set is an efficient way to check for existence.
            // If the successor is NOT in our visited set...
            if (visited.find(successor) == visited.end())
            {
                // ...then mark it as visited and add it to the queue to be processed later.
                visited.insert(successor);
                worklist.push(successor);
            }
        }
    }
     std::cout << "\n--- End of CFG ---\n" << std::endl;
}

std::string IRVisitor::getBaseName(std::string name)
{
    auto pos = name.find_last_of(".");
    return name.substr(0, pos);
}

void IRVisitor::writeVariable(std::string varName,
    std::shared_ptr<BasicBlock> block,
    std::shared_ptr<Inst> value)
{
    m_currDef[block][varName] = value;
}

std::shared_ptr<Inst> IRVisitor::readVariable(std::string varName,
    std::shared_ptr<BasicBlock> block)
{
    if (m_currDef[block].find(varName) != m_currDef[block].end())
    {
        return m_currDef[block][varName];
    }
    return readVariableRecursive(varName, block);
}

std::shared_ptr<Inst> IRVisitor::readVariableRecursive(
    std::string varName, std::shared_ptr<BasicBlock> block)
{
  if (m_sealedBlocks.find(block) == m_sealedBlocks.end())
    {
        auto baseName = getBaseName(varName);
        auto phiName = baseNameToSSA(baseName);
        auto val = std::make_shared<PhiInst>(phiName, block);
        val->setup_def_use();
        block->pushInstBegin(val);

        m_incompletePhis[block][varName] = val;
        writeVariable(varName, block, val);
        return val;

    }
    else if (block->getPredecessors().size() == 1)
    {
        auto val = readVariable(varName, block->getPredecessors()[0]);
        writeVariable(varName, block, val);
        return val;
    }
    else
    {
        auto baseName = getBaseName(varName);
        auto phiName = baseNameToSSA(baseName);
        auto val = std::make_shared<PhiInst>(phiName, block);
        val->setup_def_use();

        writeVariable(varName, block, val);
        block->pushInstBegin(val);
        auto aval = addPhiOperands(baseName, val);
        writeVariable(varName, block, aval);
        return aval;
    }
}

std::shared_ptr<Inst> IRVisitor::addPhiOperands(std::string varName,
    std::shared_ptr<PhiInst> phi)
{
    auto preds = phi->getBlock()->getPredecessors();
    for (auto pred : preds)
    {
        auto val = readVariable(varName, pred);
        phi->appendOperand(val);
    }

    return tryRemoveTrivialPhi(phi);
}

void IRVisitor::sealBlock(std::shared_ptr<BasicBlock> block)
{
    for (const auto& [var, val] : m_incompletePhis[block])
    {
        addPhiOperands(var, m_incompletePhis[block][var]);
    }
    m_sealedBlocks.insert(block);
}

std::shared_ptr<Inst> IRVisitor::tryRemoveTrivialPhi(std::shared_ptr<PhiInst> phi)
{
    std::shared_ptr<Inst> same = nullptr;
    auto& operan = phi->getOperands();
    for (auto& op : operan)
    {
        if (op == same || op == phi)
        {
            continue;
        }
        if (same != nullptr)
        {
            return phi;
        }
        same = op;
        std::cout << same->getString() << std::endl;
    }

    if (same == nullptr)
    {
        same = std::shared_ptr<UndefInst>();
    }

    auto& users = phi->get_users();
    auto users_without_phi = std::vector<std::shared_ptr<Inst>>();

    // get every users of phi except phi itself
    for (const auto& user : users)
    {
        if (user != phi)
        {
            users_without_phi.push_back(user);
        }
    }

    // replace all uses of phi with same
    for (auto& user : users_without_phi)
    {
        auto& block = user->getBlock();
        auto& instructions = block->getInstructions();
        for (int i = 0; i < instructions.size(); ++i)
        {
            if (instructions[i] == user)
            {
                auto& inst = instructions[i];
                auto& operands = inst->getOperands();

                for (int i = 0; i < operands.size(); ++i)
                {
                    if (operands[i] == phi)
                    {
                        operands[i] = same;
                    }
                }
            }
        }
    }


    // remove phi from the hash table
    auto& block = phi->getBlock();
    for (auto& [varName, value] : m_currDef[block])
    {
        if (value == phi)
        {
          m_currDef[block][varName] = same;
        }
    }

    // remove phi from instructions
    for (auto it = block->getInstructions().begin(); it != block->getInstructions().end(); )
    {
        if (*it == phi) // Compare shared_ptr directly for equality
        {
            // erase returns an iterator to the next element
            it = block->getInstructions().erase(it);
        }
        else
        {
            ++it; // Move to the next element only if not erased
        }
    }

    for (auto& user : users)
    {
        if (user && user->isPhi())
        {
            tryRemoveTrivialPhi(std::dynamic_pointer_cast<PhiInst>(user));
        }
    }
    return same;
}

/*
* before tryRemoveTrivialPhi optimizations
--- Control Flow Graph (BFS Traversal) ---

Basic Block Name: Entry_0
  a.0 <- 0
  b.0 <- 0
  c.0 <- false
  a.1 <- Get()
  b.1 <- 0
  Jump ifExprBlock_0

Basic Block Name: ifExprBlock_0
  b.2 <- Phi(b.1)
  a.2 <- Phi(a.1)
  t0 <- Cmp_GT(a.2, b.2)
  BRT(t0, thenBlock_0, elseBlock_0)

Basic Block Name: thenBlock_0
  c.1 <- true
  Jump mergeBlock_0

Basic Block Name: elseBlock_0
  c.2 <- false
  Jump mergeBlock_0

Basic Block Name: mergeBlock_0
  b.5 <- Phi(b.2, b.2)
  c.4 <- Phi(c.1, c.2)
  a.3 <- 0
  a.4 <- 5
  Jump ifExprBlock_1

Basic Block Name: ifExprBlock_1
  c.3 <- Phi(c.4)
  BRT(c.3, thenBlock_1, elseBlock_1)

Basic Block Name: thenBlock_1
  Jump repeatUntilBlock_1

Basic Block Name: elseBlock_1
  Jump mergeBlock_1

Basic Block Name: mergeBlock_1
  t3 <- Add(1, 1)
  a.6 <- t3
  Put(a.6)
  Put('\n')

Basic Block Name: repeatUntilBlock_1
  a.5 <- Phi(a.4, a.5)
  b.3 <- Phi(b.5, b.4)
  t1 <- Add(b.3, 1)
  b.4 <- t1
  Put(b.4)
  Put('\n')
  t2 <- Cmp_GTE(b.4, a.5)
  BRF(t2, repeatUntilBlock_1, repeatUntilBlock_1_exit)

Basic Block Name: repeatUntilBlock_1_exit
  Jump mergeBlock_1

--- End of CFG ---
*/

/*
* after tryRemoveTrivialPhi optimizations
--- Control Flow Graph (BFS Traversal) ---

Basic Block Name: Entry_0
  a.0 <- 0
  b.0 <- 0
  c.0 <- false
  a.1 <- Get()
  b.1 <- 0
  Jump ifExprBlock_0

Basic Block Name: ifExprBlock_0
  t0 <- Cmp_GT(a.1, b.1)
  BRT(t0, thenBlock_0, elseBlock_0)

Basic Block Name: thenBlock_0
  c.1 <- true
  Jump mergeBlock_0

Basic Block Name: elseBlock_0
  c.2 <- false
  Jump mergeBlock_0

Basic Block Name: mergeBlock_0
  c.4 <- Phi(c.1, c.2)
  a.3 <- 0
  a.4 <- 5
  Jump ifExprBlock_1

Basic Block Name: ifExprBlock_1
  BRT(c.3, thenBlock_1, elseBlock_1)

Basic Block Name: thenBlock_1
  Jump repeatUntilBlock_1

Basic Block Name: elseBlock_1
  Jump mergeBlock_1

Basic Block Name: mergeBlock_1
  t3 <- Add(1, 1)
  a.6 <- t3
  Put(a.6)
  Put('\n')

Basic Block Name: repeatUntilBlock_1
  b.3 <- Phi(b.1, b.4)
  t1 <- Add(b.3, 1)
  b.4 <- t1
  Put(b.4)
  Put('\n')
  t2 <- Cmp_GTE(b.4, a.4)
  BRF(t2, repeatUntilBlock_1, repeatUntilBlock_1_exit)

Basic Block Name: repeatUntilBlock_1_exit
  Jump mergeBlock_1

--- End of CFG ---
*/
