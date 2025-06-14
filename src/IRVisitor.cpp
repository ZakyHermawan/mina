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
    m_instStack.push(inst);
}

void IRVisitor::visit(BoolAST& v) {
    auto val = v.getVal();
    auto inst = std::make_shared<BoolConstInst>(val);
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
        m_instStack.push(inst);
    }
    else
    {
        auto newVal = "\"" + val + "\"";
        m_temp.push(newVal);
        auto inst = std::make_shared<StrConstInst>(newVal);
        m_instStack.push(inst);
    }
}

void IRVisitor::visit(IdentifierAST& v)
{
    auto val = v.getName();
    auto inst = std::make_shared<IdentInst>(getCurrentSSAName(val));
    m_instStack.push(inst);
    m_identAccType.push(IdentType::VARIABLE);
    m_temp.push(val);
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
    identifier->accept(*this);
    //auto topInst = m_instStack.top();
    //std::cout << topInst->getString() << std::endl;
    
    auto targetStr = popTemp();
    std::cout << targetStr << " <- " << exprTemp;
    std::cout << std::endl;

    auto identType = m_identAccType.top();
    m_identAccType.pop();

    if (identType == IdentType::ARRAY)
    {
        // undo ArrAccAST, since the array is being stored instead of being loaded
        popInst();
        popTemp();
        --m_tempCounter;
        m_currentBB->popInst();        
        std::string variable_name;
        std::string index_value;
        
        // Find the position of the opening and closing brackets
        size_t start_pos = targetStr.find('[');
        size_t end_pos = targetStr.find(']');
        
        // Check if both brackets were found in the correct order
        if (start_pos != std::string::npos && end_pos != std::string::npos &&
            start_pos < end_pos)
        {
          // 1. Get the part before the '['
          variable_name = targetStr.substr(0, start_pos);
        
          // 2. Get the part between '[' and ']'
          // The starting position of the index is one character after '['
          size_t index_start = start_pos + 1;
          // The length of the index string is the difference between the
          // positions
          size_t index_length = end_pos - index_start;
          index_value = targetStr.substr(index_start, index_length);
        }
        else
        {
          // Handle cases where the string is not in the expected format
          // For example, if there are no brackets, you might want to assign the
          // whole string to variable_name
          variable_name = targetStr;
          // index_value remains empty
        }
        
        //std::cout << "Variable Name: " << variable_name << std::endl;
        //std::cout << "Index Value: " << index_value << std::endl;
        auto sourceSSAName = getCurrentSSAName(variable_name);
        auto sourceSSAInst = std::make_shared<IdentInst>(sourceSSAName);
        auto targetSSAName = baseNameToSSA(variable_name);
        auto targetSSAInst = std::make_shared<IdentInst>(targetSSAName);

        std::string input = index_value;

        try {
            size_t chars_processed = 0;
            // Attempt to convert the string to an integer
            int integer_value = std::stoi(input, &chars_processed);

            // This is the crucial check: was the ENTIRE string used for the conversion?
            if (chars_processed == input.length()) {
                // IF TRUE: The string is a valid integer.
                auto indexInst = std::make_shared<IntConstInst>(integer_value);
                auto inst = std::make_shared<ArrUpdateInst>(
                    std::move(targetSSAInst), std::move(sourceSSAInst),
                    std::move(indexInst), std::move(exprInst));
                m_currentBB->pushInst(inst);

            } else {
                // IF FALSE: The string started with a number but contains other text (e.g., "543abc").
                // We treat this as an identifier.
                throw std::runtime_error("index value should be either number of valid identifier");
            }

        } catch (const std::invalid_argument& e) {
            // CATCH: An exception was thrown because the string does not start with a number (e.g., "hello").
            // This is clearly an identifier.
            auto indexInst = std::make_shared<IdentInst>(getCurrentSSAName(input));
            auto inst = std::make_shared<ArrUpdateInst>(
                (targetSSAInst), (sourceSSAInst),
                (indexInst), (exprInst));
            m_currentBB->pushInst(inst);

        } catch (const std::out_of_range& e) {
            throw std::runtime_error("index was out of range for an int");
        }
    }
    else if (identType == IdentType::VARIABLE)
    {
        //popInst();
        auto targetSSAName = baseNameToSSA(targetStr);
        auto targetSSAInst = std::make_shared<IdentInst>(targetSSAName);
        auto assignInst = std::make_shared<AssignInst>(std::move(targetSSAInst),
                                                       std::move(exprInst));
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
        auto putInst = std::make_shared<PutInst>(std::move(operand));
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
            auto putInst = std::make_shared<PutInst>(std::move(targetInst));
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
                    auto inst = std::make_shared<PutInst>(std::move(operand));
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
                    auto inst = std::make_shared<PutInst>(std::move(operand));
                    m_currentBB->pushInst(inst);
                }
                else
                {
                    auto operand =
                        std::make_shared<IdentInst>(getCurrentSSAName(temp));
                    auto inst = std::make_shared<PutInst>(std::move(operand));
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
    auto input = v.getExpr();
    input->accept(*this);
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
    auto targetInstName = baseNameToSSA(temp);
    auto targetInst = std::make_shared<IdentInst>(std::move(targetInstName));
    auto getInst =
        std::make_shared<GetInst>(std::move(targetInst));
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
    m_currentBB->pushInst(std::move(jumpInst));
    m_currentBB->pushSuccessor(ifExprBB);
    ifExprBB->pushPredecessor(m_currentBB);
    m_currentBB = ifExprBB;

    std::cout << ifExprLabel << ":" << std::endl;
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

    auto branchInst = std::make_shared<BRTInst>(exprInst, thenBB, elseBB);
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

    m_currentBB = thenBB;
    thenArm->accept(*this);

    std::cout << "goto " << mergeBlockLabel << std::endl;

    auto otherjumpInst = std::make_shared<JumpInst>(mergeBB);
    m_currentBB->pushInst(otherjumpInst);
    m_currentBB->pushSuccessor(mergeBB);

    m_currentBB = elseBB;
    if (elseArm)
    {
        std::cout << elseBlockLabel << std::endl;
        elseArm->accept(*this);
    }
    auto moreJumpInst = std::make_shared<JumpInst>(mergeBB);
    m_currentBB->pushInst(moreJumpInst);
    m_currentBB->pushSuccessor(mergeBB);

    m_currentBB = mergeBB;
    std::cout << mergeBlockLabel << std::endl;
    ++m_labelCounter;
}

void IRVisitor::visit(RepeatUntilAST& v)
{
    auto label = "repeatUntilBlock_" + std::to_string(m_labelCounter++);
    auto repeatUntilBB = std::make_shared<BasicBlock>(label);
    auto jumpInst = std::make_shared<JumpInst>(repeatUntilBB);
    m_currentBB->pushInst(std::move(jumpInst));

    m_currentBB->pushSuccessor(repeatUntilBB);
    repeatUntilBB->pushPredecessor(m_currentBB);
    repeatUntilBB->pushSuccessor(repeatUntilBB);
    m_currentBB = repeatUntilBB;

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
    m_currentBB->pushInst(std::move(newJumpInst));
    m_currentBB->pushSuccessor(repeatUntilExitBB);
    repeatUntilExitBB->pushPredecessor(m_currentBB);
    repeatUntilExitBB->pushSuccessor(repeatUntilExitBB);
    m_currentBB = repeatUntilExitBB;
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
    m_currentBB->pushInst(pushInst);
    auto retInst = std::make_shared<ReturnInst>();
    m_currentBB->pushInst(retInst);

    // Assume that return always at the end of function, and a function only have one return
    //auto newBBLabel =
    //    m_currBBNameWithoutCtr + "_" + std::to_string(++m_currBBCtr);
    //auto newBB = std::make_shared<BasicBlock>(newBBLabel);
    //newBB->pushPredecessor(m_currentBB);
    //m_currentBB->pushSuccessor(newBB);
    //m_currentBB = newBB;

    std::cout << "return " << popTemp() << std::endl;
}

void IRVisitor::visit(ArrAccessAST& v)
{
    auto identifier = v.getIdentifier();
    auto subsExpr = v.getSubsExpr();
    subsExpr->accept(*this);

    auto idxInst = popInst();
    auto baseName = identifier->getName();
    auto idxStr = popTemp();

    auto st = baseName + "[" + idxStr + "]";

    auto sourceSSAName = getCurrentSSAName(baseName);
    auto targetSSAName = getCurrentTemp();
    pushCurrentTemp();
    auto sourceInst = std::make_shared<IdentInst>(std::move(sourceSSAName));
    auto targetInst = std::make_shared<IdentInst>(std::move(targetSSAName));

    m_instStack.push(targetInst);
    auto arrAccInst = std::make_shared<ArrAccessInst>(
        std::move(targetInst), std::move(sourceInst), std::move(idxInst));
    m_temp.push(st);
    m_currentBB->pushInst(arrAccInst);
    m_identAccType.push(IdentType::ARRAY);
}

void IRVisitor::visit(ArgumentsAST& v)
{
    auto expr = v.getExpr();
    auto args = v.getArgs();
    expr->accept(*this);
    auto inst = popInst();
    std::cout << "push " << popTemp() << std::endl;
    auto pushInst = std::make_shared<PushInst>(inst);
    m_currentBB->pushInst(pushInst);
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
    auto funcName = v.getFuncName();
    //std::cout << "call " << funcName << '\n';
    auto callInst = std::make_shared<CallInst>(funcName);
    m_currentBB->pushInst(callInst);
    m_currentBB->pushSuccessor(m_funcBB[funcName]);
    //m_funcBB[funcName]->pushPredecessor(m_currentBB);

     auto newBBLabel =
         m_currBBNameWithoutCtr + "_" + std::to_string(++m_currBBCtr);
     auto newBB = std::make_shared<BasicBlock>(newBBLabel);
     newBB->pushPredecessor(m_currentBB);
     m_currentBB->pushSuccessor(newBB);
     m_currentBB = newBB;

     auto temp = getCurrentTemp();
     pushCurrentTemp();
     auto tempInst = std::make_shared<IdentInst>(temp);
     auto popInst = std::make_shared<PopInst>(tempInst);
     m_currentBB->pushInst(popInst);
     m_instStack.push(tempInst);
     std::cout << temp << " <- " << "call " << funcName << std::endl;
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
        m_currentBB->pushInst(inst);
    }
    else if (op == "/")
    {
      auto inst =
          std::make_shared<DivInst>(std::move(currentTempInst),
                                    std::move(leftInst), std::move(rightInst));
      m_currentBB->pushInst(inst);
    }
    else
    {
      auto inst =
          std::make_shared<AndInst>(std::move(currentTempInst),
                                    std::move(leftInst), std::move(rightInst));
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
        std::make_shared<IdentInst>(std::move(currentTempStr));
    m_instStack.push(currentTempInst);

    generateCurrentTemp();
    pushCurrentTemp();

    std::cout << " <- ";
    std::cout << left << " " << op << " " << right << std::endl;

    if (op == "+")
    {
        auto inst =
            std::make_shared<AddInst>(std::move(currentTempInst),
                                    std::move(leftInst), std::move(rightInst));
        m_currentBB->pushInst(inst);
    }
    else if (op == "-")
    {
        auto inst =
          std::make_shared<SubInst>(std::move(currentTempInst),
                                    std::move(leftInst), std::move(rightInst));
      m_currentBB->pushInst(inst);
    }
    else
    {
      auto inst =
          std::make_shared<OrInst>(std::move(currentTempInst),
                                    std::move(leftInst), std::move(rightInst));
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
    auto targetInst = std::make_shared<IdentInst>(std::move(currentTempStr));
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
        m_currentBB->pushInst(compInst);
    }
    else if (op == "!=")
    {
        auto compInst = std::make_shared<CmpNEInst>(
            std::move(targetInst), std::move(leftInst), std::move(rightInst));
        m_currentBB->pushInst(compInst);
    }
    else if (op == "<")
    {
        auto compInst = std::make_shared<CmpLTInst>(
            std::move(targetInst), std::move(leftInst), std::move(rightInst));
        m_currentBB->pushInst(compInst);
    }
    else if (op == "<=")
    {
        auto compInst = std::make_shared<CmpLTEInst>(
            std::move(targetInst), std::move(leftInst), std::move(rightInst));
        m_currentBB->pushInst(compInst);

    }
    else if (op == ">")
    {
        auto compInst = std::make_shared<CmpGTInst>(
            std::move(targetInst), std::move(leftInst), std::move(rightInst));
        m_currentBB->pushInst(compInst);

    }
    else if (op == ">=")
    {
        auto compInst = std::make_shared<CmpGTEInst>(
            std::move(targetInst), std::move(leftInst), std::move(rightInst));
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

    auto type = v.getIdentifier()->getType();
    if (type == Type::BOOLEAN)
    {
        auto boolConstInst = std::make_shared<BoolConstInst>(false);
        auto inst =
            std::make_shared<AssignInst>(std::move(targetIdentInst), std::move(boolConstInst));
        std::cout << "false\n";
        m_currentBB->pushInst(inst);
    }
    else if (type == Type::INTEGER)
    {
        auto intConstInst = std::make_shared<IntConstInst>(0);
        auto inst = std::make_shared<AssignInst>(std::move(targetIdentInst),
                                                 std::move(intConstInst));
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

    auto type = v.getIdentifier()->getType();
    auto size = v.getSize();
    auto allocaInst =
        std::make_shared<AllocaInst>(std::move(allocaIdentInst), type, size);

    m_currentBB->pushInst(allocaInst);

    for (unsigned int i = 0; i < v.getSize(); ++i)
    {
        std::string& baseName = v.getIdentifier()->getName();
        auto ssaName = getCurrentSSAName(baseName);
        auto sourceIdentInst = std::make_shared<IdentInst>(ssaName);

        auto indexInst = std::make_shared<IntConstInst>(i);

        auto targetSsaName = baseNameToSSA(baseName);
        auto targetIdentInst = std::make_shared<IdentInst>(targetSsaName);

        std::cout << v.getIdentifier()->getName() << "[" << i << "]"  " <- ";

        auto type = v.getIdentifier()->getType();
        if (type == Type::BOOLEAN)
        {
            auto boolConstInst = std::make_shared<BoolConstInst>(false);
            auto inst = std::make_shared<ArrUpdateInst>(
                std::move(targetIdentInst), std::move(sourceIdentInst),
                std::move(indexInst), std::move(boolConstInst));
            m_currentBB->pushInst(inst);

            std::cout << "false\n";
        }
        else if (type == Type::INTEGER)
        {
            auto intConstInst = std::make_shared<IntConstInst>(0);

            auto inst = std::make_shared<ArrUpdateInst>(
                std::move(targetIdentInst), std::move(sourceIdentInst),
                std::move(indexInst), std::move(intConstInst));
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
    ident->accept(*this);
    auto identName = popTemp();
    auto identInst = popInst();
    std::cout << identName << " : " << typeToStr(v.getType());
    //auto paramName = identName;
    //auto paramSSAName = baseNameToSSA(paramName);
    auto targetInst = identInst;
    auto popInst = std::make_shared<PopInst>(targetInst);
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

    auto inst = std::make_shared<IntConstInst>(0);
    auto pushInst = std::make_shared<PushInst>(inst);
    m_currentBB->pushInst(pushInst);

    auto retInst = std::make_shared<ReturnInst>();
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
      //std::cout << "untuk name: " << name << " counter: " << m_nameCtr[name]
      //          << std::endl; 
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
