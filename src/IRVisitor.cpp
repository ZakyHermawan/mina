#include "IRVisitor.hpp"
#include "Ast.hpp"
#include "InstIR.hpp"
#include "DisjointSetUnion.hpp"

#include <queue>
#include <set>
#include <memory>

#include <asmjit/x86.h>

typedef int (*Func)(void);

IRVisitor::IRVisitor()
    : m_tempCounter(0),
      m_labelCounter(0),
      m_jitRuntime(),
      m_codeHolder(),
      m_assembler(&m_codeHolder),
      m_logger(stdout)
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
    m_ssa.sealBlock(m_currentBB);

    m_ssa.printCFG();

    DisjointSetUnion dsu;

    // BFS
    std::queue<std::shared_ptr<BasicBlock>> worklist;
    std::set<std::shared_ptr<BasicBlock>> visited;
    worklist.push(m_ssa.getCFG());
    visited.insert(m_ssa.getCFG());
    std::vector<std::string> variables;

    while (!worklist.empty())
    {
        std::shared_ptr<BasicBlock> current_bb = worklist.front();
        visited.insert(current_bb);
        worklist.pop();

        auto& instructions = current_bb->getInstructions();

        for (unsigned int i = 0; i < instructions.size(); ++i)
        {
            auto& currInst = instructions[i];
            auto& target = currInst->getTarget();
            auto& targetStr = target->getString();
            dsu.make_set(targetStr);
            if (currInst->isPhi())
            {
                auto& operands = currInst->getOperands();
                for (int i = 0; i < operands.size(); ++i)
                {
                    auto& opStr = operands[i]->getString();
                    dsu.unite(targetStr, opStr);
                }
            }
            variables.push_back(targetStr);
        }

        for (const auto& successor : current_bb->getSuccessors())
        {
            if (visited.find(successor) == visited.end())
            {
                visited.insert(successor);
                worklist.push(successor);
            }
        }
    }

    // 1. Create a map to hold the final name for each set's root.
    std::unordered_map<std::string, std::string> root_to_new_name;

    // 2. Choose a representative name for each set.
    //    A common convention is to use the original name without the SSA suffix.
    for (const auto& var : variables) {
        std::string root = dsu.find(var);
        if (root_to_new_name.find(root) == root_to_new_name.end()) {
            // Example: if root is "x_1", new name becomes "x".
            std::string base_name = root.substr(0, root.find('.')); 
            root_to_new_name[root] = base_name;
        }
    }

    // 3. Create the final, full renaming map for all variables.
    std::unordered_map<std::string, std::string> final_rename_map;
    for (const auto& var : variables) {
        std::string root = dsu.find(var);
        final_rename_map[var] = root_to_new_name[root];
    }

    // BFS
    worklist = {};
    visited = {};

    worklist.push(m_ssa.getCFG());

    while (!worklist.empty())
    {
        std::shared_ptr<BasicBlock> current_bb = worklist.front();
        visited.insert(current_bb);
        worklist.pop();

        auto& instructions = current_bb->getInstructions();
        if (instructions.size() == 0)
        {
            continue;
        }

        unsigned int instruction_idx = 0;
        while (true)
        {
            if (instruction_idx == instructions.size())
            {
                break;
            }

            auto& currInst = instructions[instruction_idx];
            auto& target = currInst->getTarget();
            auto& operands = currInst->getOperands();
            auto& targetStr = target->getString();

            if (currInst->isPhi())
            {
                instructions.erase(instructions.begin() + instruction_idx);
                continue;
            }
            else
            {
                if (root_to_new_name.find(targetStr) == root_to_new_name.end())
                {
                    throw std::runtime_error("can't find target str " + targetStr + "in the hashmap");
                }
                auto& new_target_str = root_to_new_name[targetStr];

                std::shared_ptr<IdentInst> newTargetInst = std::make_shared<IdentInst>(new_target_str, currInst->getBlock());
                auto& operands = currInst->getOperands();
                for (int i = 0; i < operands.size(); ++i)
                {
                    if (operands[i]->canBeRenamed() == false)
                    {
                        continue;
                    }
                    auto& operandTarget = operands[i]->getTarget();
                    auto& operandTargetStr = operandTarget->getString();
                    if (root_to_new_name.find(operandTargetStr) ==
                        root_to_new_name.end())
                    {
                        continue;
                    }

                    auto& new_operand_target_str = root_to_new_name[operandTargetStr];
                    std::shared_ptr<IdentInst> newOperandTargetInst =
                        std::make_shared<IdentInst>(new_operand_target_str,
                                                    currInst->getBlock());
                    operands[i] = newOperandTargetInst;
                }
                currInst->setTarget(newTargetInst);
            }
            ++instruction_idx;
        }

        for (const auto& successor : current_bb->getSuccessors())
        {
            if (visited.find(successor) == visited.end())
            {
                visited.insert(successor);
                worklist.push(successor);
            }
        }
    }
    generateX86();
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
    //sealBlock(m_currentBB);

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
    //sealBlock(m_currentBB);

    thenArm->accept(*this);

    auto otherjumpInst = std::make_shared<JumpInst>(mergeBB);
    otherjumpInst->setup_def_use();
    m_currentBB->pushInst(otherjumpInst);
    m_currentBB->pushSuccessor(mergeBB);

    m_ssa.sealBlock(m_currentBB);
    m_currentBB = elseBB;
    //sealBlock(m_currentBB);

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
    //sealBlock(m_currentBB);

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
    //sealBlock(m_currentBB);

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
    //sealBlock(m_currentBB);
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
    expr->accept(*this);

    m_arguments.push_back(popInst());
    m_argNames.push_back(popTemp());

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

    auto funcName = v.getFuncName();

    auto callInst = std::make_shared<CallInst>(funcName, m_arguments, m_currentBB);
    callInst->setup_def_use();
    m_currentBB->pushInst(callInst);
    m_currentBB->pushSuccessor(m_funcBB[funcName]);
    m_funcBB[funcName]->pushPredecessor(m_currentBB);

    m_ssa.incCurrBBCtr();
    auto newBBLabel =
        m_ssa.getCurrBBNameWithoutCtr() + "_" + std::to_string(m_ssa.getCurrBBCtr());
    auto newBB = std::make_shared<BasicBlock>(newBBLabel);
    newBB->pushPredecessor(m_funcBB[funcName]);
    m_funcBB[funcName]->pushSuccessor(newBB);
    m_currentBB = newBB;

    auto temp = getCurrentTemp();
    pushCurrentTemp();

    auto tempInst = std::make_shared<IdentInst>(temp, m_currentBB);
    tempInst->setup_def_use();
    auto popInst = std::make_shared<PopInst>(tempInst, m_currentBB);
    popInst->setup_def_use();
    m_currentBB->pushInst(popInst);
    m_instStack.push(tempInst);
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
    v.getTerm()->accept(*this);
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
    auto identName = ident->getName();
    auto identInst = std::make_shared<IdentInst>(m_ssa.baseNameToSSA(identName),
                                                 m_currentBB);
    m_ssa.writeVariable(identName, m_currentBB, identInst);

    // we should remove this and implement lowerParam()
    auto popInst = std::make_shared<PopInst>(identInst, m_currentBB);
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

    for (auto& param : m_parameters)
    {
        auto identType = param->getIdentType();
        if (identType == IdentType::VARIABLE)
        {
            std::cout << "var!\n";
        }
        else
        {
            std::runtime_error("array parameter is not implemented yet!\n");
        }
    }

    scope->accept(*this);
    auto funcSignature = std::make_shared<FuncSignature>(
        procName, FType::PROC, Type::UNDEFINED, m_parameters, m_currentBB);
    m_currentBB->pushInst(funcSignature);

    //generateX86(); try to generate the function here, but don't execute, save it so we can syscall later
    m_currentBB = oldBB;
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

void IRVisitor::generateX86()
{
    // BFS
    std::queue<std::shared_ptr<BasicBlock>> worklist;
    std::set<std::shared_ptr<BasicBlock>> visited;
    worklist.push(m_ssa.getCFG());
    visited.insert(m_ssa.getCFG());
    
    std::vector<std::string> variables;

    //asmjit::FileLogger logger(stdout);  // Logger should always survive CodeHolder.
    asmjit::StringLogger logger;
    // Runtime designed for JIT - it holds relocated functions and controls
    // their lifetime.
    asmjit::JitRuntime rt;

    // Holds code and relocation information during code generation.
    asmjit::CodeHolder code;

    // Initialize CodeHolder. It will be configured to match the host CPU
    // architecture. If you run this on an x86/x86_64 machine, it will generate
    // x86 code.
    code.init(rt.environment(), rt.cpuFeatures());
    code.setLogger(&logger);  // attach logger

    // Use x86::Assembler to emit x86/x86_64 code.
    asmjit::x86::Compiler cc(&code);
    cc.addFunc(asmjit::FuncSignature::build<void>());  // Begin a function of `int
                                                       // fn(void)` signature.

    std::unordered_map<std::string, asmjit::x86::Gp> registerMap;
    std::unordered_map<std::string, asmjit::Label> labelMap;

    while (!worklist.empty())
    {
        std::shared_ptr<BasicBlock> current_bb = worklist.front();
        auto bbName = current_bb->getName();
        if (labelMap.find(bbName) == labelMap.end())
        {
            labelMap[bbName] = cc.newNamedLabel(bbName.c_str());
        }
        auto& label = labelMap[bbName];
        cc.bind(label);

        visited.insert(current_bb);

        worklist.pop();

        auto& instructions = current_bb->getInstructions();

        enum class CmpType
        {
            NONE,
            EQ,
            NE,
            LT,
            LTE,
            GT,
            GTE
        };

        struct CodegenState
        {
            CmpType last_comparison_type = CmpType::NONE;
        } state;

        bool is_prev_cmp = false;
        for (unsigned int i = 0; i < instructions.size(); ++i)
        {
            auto& currInst = instructions[i];
            auto instType = currInst->getInstType();
            switch (instType)
            {
                case InstType::Add:
                {
                    auto& target = currInst->getTarget();
                    auto& operands = currInst->getOperands();
                    auto& operand1 = operands[0]->getTarget();
                    auto& operand2 = operands[1]->getTarget();

                    auto& targetName = target->getString();
                    if (registerMap.find(targetName) == registerMap.end())
                    {
                        auto newReg = cc.newGp32(targetName.c_str());
                        registerMap[targetName] = newReg;
                    }

                    auto& targetRegister = registerMap[targetName];
                    auto operandInstType = operand1->getInstType();
                    switch (operandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand1);
                            auto err = cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand1);
                            cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        default:
                        {
                            cc.mov(
                                targetRegister,
                                registerMap[operand1->getTarget()->getString()]);
                            break;
                        }
                    }

                    auto otherOperandInstType = operand2->getInstType();

                    switch (otherOperandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand2);
                            cc.add(targetRegister, casted->getVal());
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand2);
                            cc.add(targetRegister, casted->getVal());
                            break;
                        }
                        default:
                        {
                            cc.add(
                                targetRegister,
                                registerMap[operand2->getTarget()->getString()]);
                            break;
                        }
                    }
                    break;
                }
                case InstType::Sub:
                {
                    auto& target = currInst->getTarget();
                    auto& operands = currInst->getOperands();
                    auto& operand1 = operands[0]->getTarget();
                    auto& operand2 = operands[1]->getTarget();

                    auto& targetName = target->getString();
                    if (registerMap.find(targetName) == registerMap.end())
                    {
                        auto newReg = cc.newGp32(targetName.c_str());
                        registerMap[targetName] = newReg;
                    }

                    auto& targetRegister = registerMap[targetName];
                    auto operandInstType = operand1->getInstType();
                    switch (operandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand1);
                            cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand1);
                            cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        default:
                        {
                            cc.mov(
                                targetRegister,
                                registerMap[operand1->getTarget()->getString()]);
                            break;
                        }
                    }

                    auto otherOperandInstType = operand2->getInstType();
                    switch (otherOperandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand2);
                            cc.sub(targetRegister, casted->getVal());
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand2);
                            cc.sub(targetRegister, casted->getVal());
                            break;
                        }
                        default:
                        {
                            cc.sub(
                                targetRegister,
                                registerMap[operand2->getTarget()->getString()]);
                            break;
                        }
                    }
                    break;
                }
                case InstType::Mul:
                {
                    auto& target = currInst->getTarget();
                    auto& operands = currInst->getOperands();
                    auto& operand1 = operands[0]->getTarget();
                    auto& operand2 = operands[1]->getTarget();

                    auto& targetName = target->getString();
                    if (registerMap.find(targetName) == registerMap.end())
                    {
                        auto newReg = cc.newGp32(targetName.c_str());
                        registerMap[targetName] = newReg;
                    }

                    auto& targetRegister = registerMap[targetName];
                    auto operandInstType = operand1->getInstType();
                    switch (operandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand1);
                            cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand1);
                            cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        default:
                        {
                            cc.mov(
                                targetRegister,
                                registerMap[operand1->getTarget()->getString()]);
                            break;
                        }
                    }

                    auto otherOperandInstType = operand2->getInstType();
                    switch (otherOperandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand2);
                            auto tempReg = cc.newGp32();
                            cc.mov(tempReg, casted->getVal());
                            cc.imul(targetRegister, tempReg);
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand2);
                            auto tempReg = cc.newGp32();
                            cc.mov(tempReg, casted->getVal());
                            cc.imul(targetRegister, tempReg);
                            break;
                        }
                        default:
                        {
                            cc.imul(
                                targetRegister,
                                registerMap[operand2->getTarget()->getString()]);
                            break;
                        }
                    }
                    break;
                }
                case InstType::Div:
                {
                    auto& target = currInst->getTarget();
                    auto& operands = currInst->getOperands();
                    auto& operand1 = operands[0]->getTarget();
                    auto& operand2 = operands[1]->getTarget();

                    auto& targetName = target->getString();
                    if (registerMap.find(targetName) == registerMap.end())
                    {
                        auto newReg = cc.newGp32(targetName.c_str());
                        registerMap[targetName] = newReg;
                    }

                    auto& targetRegister = registerMap[targetName];
                    auto operandInstType = operand1->getInstType();
                    switch (operandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand1);
                            cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand1);
                            cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        default:
                        {
                            cc.mov(
                                targetRegister,
                                registerMap[operand1->getTarget()->getString()]);
                            break;
                        }
                    }

                    auto otherOperandInstType = operand2->getInstType();
                    switch (otherOperandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand2);

                            auto a = cc.newInt32("a");
                            auto b = cc.newInt32("b");
                            auto dummy = cc.newInt32("dummy");

                            cc.xor_(dummy, dummy);
                            cc.mov(a, targetRegister);
                            cc.mov(b, casted->getVal());
                            cc.idiv(dummy, a, b);
                            cc.mov(targetRegister, a);

                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand2);

                            auto a = cc.newInt32("a");
                            auto b = cc.newInt32("b");
                            auto dummy = cc.newInt32("dummy");

                            cc.xor_(dummy, dummy);
                            cc.mov(a, targetRegister);
                            cc.mov(b, casted->getVal());
                            cc.idiv(dummy, a, b);
                            cc.mov(targetRegister, a);

                            break;
                        }
                        default:
                        {
                            auto a = cc.newInt32("a");
                            auto b = cc.newInt32("b");
                            auto dummy = cc.newInt32("dummy");

                            cc.xor_(dummy, dummy);
                            cc.mov(a, targetRegister);
                            cc.mov(b, registerMap[operand2->getTarget()->getString()]);
                            cc.idiv(dummy, a, b);
                            cc.mov(targetRegister, a);

                            break;
                        }
                    }
                    break;
                }
                case InstType::Not:
                {
                    auto& target = currInst->getTarget();
                    auto& operands = currInst->getOperands();
                    auto& operand1 = operands[0]->getTarget();

                    auto& targetName = target->getString();
                    if (registerMap.find(targetName) == registerMap.end())
                    {
                        auto newReg = cc.newGp32(targetName.c_str());
                        registerMap[targetName] = newReg;
                    }

                    auto& targetRegister = registerMap[targetName];
                    auto operandInstType = operand1->getInstType();
                    switch (operandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand1);
                            cc.mov(targetRegister, casted->getVal());
                            cc.neg(targetRegister);
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand1);
                            cc.mov(targetRegister, casted->getVal());
                            cc.neg(targetRegister);
                            break;
                        }
                        default:
                        {
                            cc.mov(
                                targetRegister,
                                registerMap[operand1->getTarget()->getString()]);
                            cc.neg(targetRegister);
                            break;
                        }
                    }
                    break;
                }
                case InstType::And:
                {
                    auto& target = currInst->getTarget();
                    auto& operands = currInst->getOperands();
                    auto& operand1 = operands[0]->getTarget();
                    auto& operand2 = operands[1]->getTarget();

                    auto& targetName = target->getString();
                    if (registerMap.find(targetName) == registerMap.end())
                    {
                        auto newReg = cc.newGp32(targetName.c_str());
                        registerMap[targetName] = newReg;
                    }

                    auto& targetRegister = registerMap[targetName];
                    auto operandInstType = operand1->getInstType();
                    switch (operandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand1);
                            cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand1);
                            cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        default:
                        {
                            cc.mov(
                                targetRegister,
                                registerMap[operand1->getTarget()->getString()]);
                            break;
                        }
                    }

                    auto otherOperandInstType = operand2->getInstType();
                    switch (otherOperandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand2);
                            auto tempReg = cc.newGp32();
                            cc.mov(tempReg, casted->getVal());
                            cc.and_(targetRegister, tempReg);
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand2);
                            auto tempReg = cc.newGp32();
                            cc.mov(tempReg, casted->getVal());
                            cc.and_(targetRegister, tempReg);
                            break;
                        }
                        default:
                        {
                            cc.and_(
                                targetRegister,
                                registerMap[operand2->getTarget()->getString()]);
                            break;
                        }
                    }
                    break;
                }
                case InstType::Or:
                {
                    auto& target = currInst->getTarget();
                    auto& operands = currInst->getOperands();
                    auto& operand1 = operands[0]->getTarget();
                    auto& operand2 = operands[1]->getTarget();

                    auto& targetName = target->getString();
                    if (registerMap.find(targetName) == registerMap.end())
                    {
                        auto newReg = cc.newGp32(targetName.c_str());
                        registerMap[targetName] = newReg;
                    }

                    auto& targetRegister = registerMap[targetName];
                    auto operandInstType = operand1->getInstType();
                    switch (operandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand1);
                            cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand1);
                            cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        default:
                        {
                            cc.mov(
                                targetRegister,
                                registerMap[operand1->getTarget()->getString()]);
                            break;
                        }
                    }

                    auto otherOperandInstType = operand2->getInstType();
                    switch (otherOperandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand2);
                            auto tempReg = cc.newGp32();
                            cc.mov(tempReg, casted->getVal());
                            cc.or_(targetRegister, tempReg);
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand2);
                            auto tempReg = cc.newGp32();
                            cc.mov(tempReg, casted->getVal());
                            cc.or_(targetRegister, tempReg);
                            break;
                        }
                        default:
                        {
                            cc.or_(
                                targetRegister,
                                registerMap[operand2->getTarget()->getString()]);
                            break;
                        }
                    }
                    break;
                }
                case InstType::Alloca:
                {
                    auto& target = currInst->getTarget();
                    auto allocaConverted =
                        std::dynamic_pointer_cast<AllocaInst>(currInst);
                    auto size = allocaConverted->getSize();
                    auto type = allocaConverted->getType();

                    auto& targetName = target->getString();

                    if (registerMap.find(targetName) == registerMap.end())
                    {
                        auto newReg = cc.newGp32(targetName.c_str());
                        registerMap[targetName] = newReg;
                    }
                    m_tmp = registerMap[targetName];
                    auto& targetRegister = registerMap[targetName];
                    if (type == Type::BOOLEAN)
                    {
                        cc.sub(asmjit::x86::esp, size);
                        cc.mov(targetRegister, asmjit::x86::esp);
                    }
                    else if (type == Type::INTEGER)
                    {
                        cc.sub(asmjit::x86::esp, size * 4);
                        cc.mov(targetRegister, asmjit::x86::esp);
                    }
                    else
                    {
                        throw std::runtime_error("Unknown type");
                    }

                    break;
                }
                case InstType::ArrAccess:
                {
                    auto castedInst =
                          std::dynamic_pointer_cast<ArrAccessInst>(currInst);
                    auto& target = castedInst->getTarget();
                    auto& source = castedInst->getSource()->getTarget();
                    auto& index = castedInst->getIndex()->getTarget();

                    auto& sourceReg = registerMap[source->getString()];
                    auto targetReg = cc.newGp32();
                    auto& targetName = target->getString();
                    registerMap[targetName] = targetReg;

                    auto indexType = castedInst->getType();
                    asmjit::x86::Gp indexTempReg;

                    switch (indexType)
                    {
                        case Type::INTEGER:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(index);
                            indexTempReg = cc.newGp32();
                            cc.mov(indexTempReg, casted->getVal() * 4);
                            break;
                        }
                        case Type::BOOLEAN:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(index);
                            indexTempReg = cc.newGp32();
                            cc.mov(indexTempReg, casted->getVal());
                            break;
                        }
                        default:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(index);
                            indexTempReg = registerMap[index->getTarget()->getString()];
                            cc.mov(indexTempReg, casted->getVal());
                            break;
                        }
                    }

                    cc.mov(targetReg, asmjit::x86::dword_ptr(sourceReg, indexTempReg));
                    break;
                }
                case InstType::ArrUpdate:
                {
                    auto castedInst =
                          std::dynamic_pointer_cast<ArrUpdateInst>(currInst);
                    auto& target = castedInst->getTarget();
                    auto& source = castedInst->getSource()->getTarget();
                    auto& index = castedInst->getIndex()->getTarget();
                    auto& val = castedInst->getVal()->getTarget();
                    auto& targetName = target->getString();
                    if (registerMap.find(targetName) == registerMap.end())
                    {
                        throw std::runtime_error(
                            "unknown target register name: " + targetName);
                    }
                    auto& targetRegister = registerMap[targetName];
                    auto& sourceRegister = registerMap[targetName];

                    auto indexType = castedInst->getType();
                    asmjit::x86::Gp indexTempReg;

                    switch (indexType)
                    {
                        case Type::INTEGER:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(index);
                            if (casted == nullptr)
                            {
                                throw std::runtime_error("error when casting pointer!\n");
                            }
                            indexTempReg = cc.newGp32();
                            cc.mov(indexTempReg, casted->getVal() * 4);
                            break;
                        }
                        case Type::BOOLEAN:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(index);
                            if (casted == nullptr)
                            {
                                throw std::runtime_error("error when casting pointer!\n");
                            }
                            indexTempReg = cc.newGp32();
                            cc.mov(indexTempReg, casted->getVal());
                            break;
                        }
                        default:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(index);
                            if (casted == nullptr)
                            {
                                throw std::runtime_error("error when casting pointer!\n");
                            }
                            indexTempReg = registerMap[index->getTarget()->getString()];
                            cc.mov(indexTempReg, casted->getVal());
                            break;
                        }
                    }

                    auto valueType = val->getInstType();
                    asmjit::x86::Gp valueTempReg;
                    switch (valueType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(val);
                            valueTempReg = cc.newGp32();
                            cc.mov(valueTempReg, casted->getVal());
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(val);
                            valueTempReg = cc.newGp32();
                            cc.mov(valueTempReg, casted->getVal());
                            break;
                        }
                        default:
                        {
                            valueTempReg =
                                registerMap[index->getTarget()->getString()];
                            break;
                        }
                    }

                    cc.mov(asmjit::x86::dword_ptr(sourceRegister, indexTempReg), valueTempReg);
                    break;
                }
                case InstType::Assign:
                {
                    if (is_prev_cmp == true)
                    {
                        break;
                    }
                    auto& target = currInst->getTarget();
                    auto& operands = currInst->getOperands();
                    auto& operand1 = operands[0]->getTarget();

                    auto& targetName = target->getString();
                    if (registerMap.find(targetName) == registerMap.end())
                    {
                        auto newReg = cc.newGp32(targetName.c_str());
                        registerMap[targetName] = newReg;
                    }
                    auto& targetRegister = registerMap[targetName];
                    auto operandInstType = operand1->getInstType();
                    switch (operandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand1);
                            cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand1);
                            cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        default:
                        {
                            cc.mov(
                                targetRegister,
                                registerMap[operand1->getString()]);
                            break;
                        }
                    }
                    break;
                }
                case InstType::CmpEq:
                {
                    auto cmpEqInst = std::dynamic_pointer_cast<CmpEQInst>(currInst);
                    auto& target = cmpEqInst->getTarget();
                    auto& operands = cmpEqInst->getOperands();
                    auto& operand1 = operands[0]->getTarget();
                    auto& operand2 = operands[1]->getTarget();

                    auto& targetName = target->getString();
                    if (registerMap.find(targetName) == registerMap.end())
                    {
                        auto newReg = cc.newGp32(targetName.c_str());
                        registerMap[targetName] = newReg;
                    }

                    auto& targetRegister = registerMap[targetName];
                    auto operandInstType = operand1->getInstType();
                    switch (operandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand1);
                            cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand1);
                            cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        default:
                        {
                            cc.mov(
                                targetRegister,
                                registerMap[operand1->getTarget()->getString()]);
                            break;
                        }
                    }

                    auto otherOperandInstType = operand2->getInstType();
                    switch (otherOperandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand2);
                            auto tempReg = cc.newGp32();
                            cc.mov(tempReg, casted->getVal());
                            cc.cmp(targetRegister, tempReg);
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand2);
                            auto tempReg = cc.newGp32();
                            cc.mov(tempReg, casted->getVal());
                            cc.cmp(targetRegister, tempReg);
                            break;
                        }
                        default:
                        {
                            cc.cmp(
                                targetRegister,
                                registerMap[operand2->getTarget()->getString()]);
                            break;
                        }
                    }
                    is_prev_cmp = true;
                    state.last_comparison_type = CmpType::EQ;
                    break;
                }
                case InstType::CmpNE:
                {
                    auto cmpNeInst = std::dynamic_pointer_cast<CmpNEInst>(currInst);
                    auto& target = cmpNeInst->getTarget();
                    auto& operands = cmpNeInst->getOperands();
                    auto& operand1 = operands[0]->getTarget();
                    auto& operand2 = operands[1]->getTarget();

                    auto& targetName = target->getString();
                    if (registerMap.find(targetName) == registerMap.end())
                    {
                        auto newReg = cc.newGp32(targetName.c_str());
                        registerMap[targetName] = newReg;
                    }

                    auto& targetRegister = registerMap[targetName];
                    auto operandInstType = operand1->getInstType();
                    switch (operandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand1);
                            cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand1);
                            cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        default:
                        {
                            cc.mov(
                                targetRegister,
                                registerMap[operand1->getTarget()->getString()]);
                            break;
                        }
                    }

                    auto otherOperandInstType = operand2->getInstType();
                    switch (otherOperandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand2);
                            auto tempReg = cc.newGp32();
                            cc.mov(tempReg, casted->getVal());
                            cc.cmp(targetRegister, tempReg);
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand2);
                            auto tempReg = cc.newGp32();
                            cc.mov(tempReg, casted->getVal());
                            cc.cmp(targetRegister, tempReg);
                            break;
                        }
                        default:
                        {
                            cc.cmp(
                                targetRegister,
                                registerMap[operand2->getTarget()->getString()]);
                            break;
                        }
                    }
                    is_prev_cmp = true;
                    state.last_comparison_type = CmpType::NE;
                    break;
                }
                case InstType::CmpLT:
                {
                    auto cmpLtInstr = std::dynamic_pointer_cast<CmpLTInst>(currInst);
                    auto& target = cmpLtInstr->getTarget();
                    auto& operands = cmpLtInstr->getOperands();
                    auto& operand1 = operands[0]->getTarget();
                    auto& operand2 = operands[1]->getTarget();

                    auto& targetName = target->getString();
                    if (registerMap.find(targetName) == registerMap.end())
                    {
                        auto newReg = cc.newGp32(targetName.c_str());
                        registerMap[targetName] = newReg;
                    }

                    auto& targetRegister = registerMap[targetName];
                    auto operandInstType = operand1->getInstType();
                    switch (operandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand1);
                            cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand1);
                            cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        default:
                        {
                            cc.mov(
                                targetRegister,
                                registerMap[operand1->getTarget()->getString()]);
                            break;
                        }
                    }

                    auto otherOperandInstType = operand2->getInstType();
                    switch (otherOperandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand2);
                            auto tempReg = cc.newGp32();
                            cc.mov(tempReg, casted->getVal());
                            cc.cmp(targetRegister, tempReg);
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand2);
                            auto tempReg = cc.newGp32();
                            cc.mov(tempReg, casted->getVal());
                            cc.cmp(targetRegister, tempReg);
                            break;
                        }
                        default:
                        {
                            cc.cmp(
                                targetRegister,
                                registerMap[operand2->getTarget()->getString()]);
                            break;
                        }
                    }
                    is_prev_cmp = true;
                    state.last_comparison_type = CmpType::LT;
                    break;
                }
                case InstType::CmpLTE:
                {
                    auto cmpLteInst = std::dynamic_pointer_cast<CmpLTEInst>(currInst);
                    auto& target = cmpLteInst->getTarget();
                    auto& operands = cmpLteInst->getOperands();
                    auto& operand1 = operands[0]->getTarget();
                    auto& operand2 = operands[1]->getTarget();

                    auto& targetName = target->getString();
                    if (registerMap.find(targetName) == registerMap.end())
                    {
                        auto newReg = cc.newGp32(targetName.c_str());
                        registerMap[targetName] = newReg;
                    }

                    auto& targetRegister = registerMap[targetName];
                    auto operandInstType = operand1->getInstType();
                    switch (operandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand1);
                            cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand1);
                            cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        default:
                        {
                            cc.mov(
                                targetRegister,
                                registerMap[operand1->getTarget()->getString()]);
                            break;
                        }
                    }

                    auto otherOperandInstType = operand2->getInstType();
                    switch (otherOperandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand2);
                            auto tempReg = cc.newGp32();
                            cc.mov(tempReg, casted->getVal());
                            cc.cmp(targetRegister, tempReg);
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand2);
                            auto tempReg = cc.newGp32();
                            cc.mov(tempReg, casted->getVal());
                            cc.cmp(targetRegister, tempReg);
                            break;
                        }
                        default:
                        {
                            cc.cmp(
                                targetRegister,
                                registerMap[operand2->getTarget()->getString()]);
                            break;
                        }
                    }
                    is_prev_cmp = true;
                    state.last_comparison_type = CmpType::LTE;
                    break;
                }
                case InstType::CmpGT:
                {
                    auto cmpGtInst = std::dynamic_pointer_cast<CmpGTInst>(currInst);
                    auto& target = cmpGtInst->getTarget();
                    auto& operands = cmpGtInst->getOperands();
                    auto& operand1 = operands[0]->getTarget();
                    auto& operand2 = operands[1]->getTarget();

                    auto& targetName = target->getString();
                    if (registerMap.find(targetName) == registerMap.end())
                    {
                        auto newReg = cc.newGp32(targetName.c_str());
                        registerMap[targetName] = newReg;
                    }

                    auto& targetRegister = registerMap[targetName];
                    auto operandInstType = operand1->getInstType();
                    switch (operandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand1);
                            cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand1);
                            cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        default:
                        {
                            cc.mov(
                                targetRegister,
                                registerMap[operand1->getTarget()->getString()]);
                            break;
                        }
                    }

                    auto otherOperandInstType = operand2->getInstType();
                    switch (otherOperandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand2);
                            auto tempReg = cc.newGp32();
                            cc.mov(tempReg, casted->getVal());
                            cc.cmp(targetRegister, tempReg);
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand2);
                            auto tempReg = cc.newGp32();
                            cc.mov(tempReg, casted->getVal());
                            cc.cmp(targetRegister, tempReg);
                            break;
                        }
                        default:
                        {
                            cc.cmp(
                                targetRegister,
                                registerMap[operand2->getTarget()->getString()]);
                            break;
                        }
                    }
                    is_prev_cmp = true;
                    state.last_comparison_type = CmpType::GT;
                    break;
                }
                case InstType::CmpGTE:
                {
                    auto cmpGteInst = std::dynamic_pointer_cast<CmpGTEInst>(currInst);
                    auto& target = cmpGteInst->getTarget();
                    auto& operands = cmpGteInst->getOperands();
                    auto& operand1 = operands[0]->getTarget();
                    auto& operand2 = operands[1]->getTarget();

                    auto& targetName = target->getString();
                    if (registerMap.find(targetName) == registerMap.end())
                    {
                        auto newReg = cc.newGp32(targetName.c_str());
                        registerMap[targetName] = newReg;
                    }

                    auto& targetRegister = registerMap[targetName];
                    auto operandInstType = operand1->getInstType();
                    switch (operandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand1);
                            cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand1);
                            cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        default:
                        {
                            cc.mov(
                                targetRegister,
                                registerMap[operand1->getTarget()->getString()]);
                            break;
                        }
                    }

                    auto otherOperandInstType = operand2->getInstType();
                    switch (otherOperandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand2);
                            auto tempReg = cc.newGp32();
                            cc.mov(tempReg, casted->getVal());
                            cc.cmp(targetRegister, tempReg);
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand2);
                            auto tempReg = cc.newGp32();
                            cc.mov(tempReg, casted->getVal());
                            cc.cmp(targetRegister, tempReg);
                            break;
                        }
                        default:
                        {
                            cc.cmp(
                                targetRegister,
                                registerMap[operand2->getTarget()->getString()]);
                            break;
                        }
                    }
                    is_prev_cmp = true;
                    state.last_comparison_type = CmpType::GTE;
                    break;
                }
                case InstType::Jump:
                {
                    auto jumpInst = std::dynamic_pointer_cast<JumpInst>(currInst);
                    auto& targetLabel = jumpInst->getJumpTarget();
                    auto& targetLabelName = targetLabel->getName();
                    if (labelMap.find(targetLabelName) == labelMap.end())
                    {
                        labelMap[targetLabelName] =
                            cc.newNamedLabel(targetLabelName.c_str());
                    }
                    auto& label = labelMap[targetLabelName];
                    cc.jmp(label);
                    is_prev_cmp = false;
                    break;
                }
                case InstType::BRT:
                {
                    auto brtInst = std::dynamic_pointer_cast<BRTInst>(currInst);
                    auto& failLabel = brtInst->getTargetFailed();
                    auto& failLabelName = failLabel->getName();
                    if (labelMap.find(failLabelName) == labelMap.end())
                    {
                        labelMap[failLabelName] =
                            cc.newNamedLabel(failLabelName.c_str());
                    }
                    auto& label = labelMap[failLabelName];
                    switch (state.last_comparison_type)
                    {
                        case CmpType::EQ:
                            cc.jne(label);
                            break;
                        case CmpType::NE:
                            cc.je(label);
                            break;
                        case CmpType::LT:
                            cc.jge(label);
                            break;
                        case CmpType::LTE:
                            cc.jg(label);
                            break;
                        case CmpType::GT:
                            cc.jle(label);
                            break;
                        case CmpType::GTE:
                            cc.jl(label);
                            break;

                    }
                    is_prev_cmp = false;
                    break;
                }
                case InstType::BRF:
                {
                    auto brtInst = std::dynamic_pointer_cast<BRFInst>(currInst);
                    auto& failLabel = brtInst->getTargetFailed();
                    auto& failLabelName = failLabel->getName();
                    if (labelMap.find(failLabelName) == labelMap.end())
                    {
                        labelMap[failLabelName] =
                            cc.newNamedLabel(failLabelName.c_str());
                    }
                    auto& label = labelMap[failLabelName];
                    switch (state.last_comparison_type)
                    {
                        case CmpType::EQ:
                            cc.je(label);
                            break;
                        case CmpType::NE:
                            cc.jne(label);
                            break;
                        case CmpType::LT:
                            cc.jl(label);
                            break;
                        case CmpType::LTE:
                            cc.jle(label);
                            break;
                        case CmpType::GT:
                            cc.jg(label);
                            break;
                        case CmpType::GTE:
                            cc.jge(label);
                            break;
                    }
                    is_prev_cmp = false;
                    break;
                }

                case InstType::Put:
                {
                    auto putInst = std::dynamic_pointer_cast<PutInst>(currInst);
                    auto& operands = putInst->getOperands();
                    for (int i = 0; i < operands.size(); ++i)
                    {
                        auto& operand = operands[i]->getTarget();
                        auto operandType = operand->getInstType();
                        switch (operandType)
                        {
                            case InstType::IntConst:
                            {
                                auto intConstInst = std::dynamic_pointer_cast<IntConstInst>(operand);
                                auto val = intConstInst->getVal();
                                syscallPrintInt(cc, val);
                                break;
                            }
                            case InstType::BoolConst:
                            {
                                auto boolConstInst = std::dynamic_pointer_cast<BoolConstInst>(operand);
                                auto val = boolConstInst->getVal();
                                if (val != 0 && val != 1)
                                {
                                    throw std::runtime_error("Boolean value must be 0 or 1!");
                                }
                                
                                if (val)
                                {
                                    syscallPrintString(cc, std::string("true"));
                                }
                                else
                                {
                                    syscallPrintString(cc, std::string("false"));
                                }
                                break;
                            }
                            case InstType::StrConst:
                            {
                                auto strConstInst =
                                    std::dynamic_pointer_cast<StrConstInst>(
                                        operand);
                                auto& val = strConstInst->getString();
                                if (val == "\'\\n\'")
                                {
                                    syscallPutChar(cc, '\n');
                                    break;
                                }
                                for (int i = 0; i < val.length(); ++i)
                                {
                                    if(val[i] == '\"' || val[i] == '\'') continue;
                                    syscallPutChar(cc, val[i]);
                                }
                                break;
                            }
                            default:
                            {
                                // todo: check data type, and print true/false or integer based on data type
                                // assume integer
                                auto identifierInst =
                                    std::dynamic_pointer_cast<IdentInst>(
                                        operand);
                                std::string result;
                                std::string st = identifierInst->getString();
                                auto dot_pos = st.find(".");
                                if (dot_pos != std::string::npos)
                                {
                                    result = st.substr(0, dot_pos);
                                }
                                else
                                {
                                    result = st;
                                }

                                auto& reg =
                                    registerMap[result];
                                cc.mov(asmjit::x86::ecx, reg);
                                cc.add(asmjit::x86::ecx, '0');
                                asmjit::Imm printWithParamAddr =
                                    asmjit::Imm((void*)putchar);
                                cc.sub(asmjit::x86::rsp, 32);
                                cc.call(printWithParamAddr);
                                cc.add(asmjit::x86::rsp, 32);
                                break;
                            }
                        }
                    }
                    break;
                }
                case InstType::Get:
                {
                    auto getInst = std::dynamic_pointer_cast<GetInst>(currInst);
                    auto target = getInst->getTarget();

                    auto& targetName = target->getString();
                    if (registerMap.find(targetName) == registerMap.end())
                    {
                        auto newReg = cc.newGp32(targetName.c_str());
                        registerMap[targetName] = newReg;
                    }
                    auto& targetRegister = registerMap[targetName];
                    syscallScanInt(cc, targetRegister);
                    cc.mov(targetRegister, asmjit::x86::eax);
                    break;
                }
                case InstType::Push:
                {
                    auto pushInst = std::dynamic_pointer_cast<GetInst>(currInst);
                    auto operand = pushInst->getOperands()[0]->getTarget();
                    auto operandType = operand->getInstType();
                    switch (operandType)
                    {
                        case InstType::IntConst:
                        {
                            auto intConstInst = std::dynamic_pointer_cast<IntConstInst>(operand);
                            auto val = intConstInst->getVal();
                            cc.push(val);
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto boolConstInst = std::dynamic_pointer_cast<BoolConstInst>(operand);
                            auto val = boolConstInst->getVal();
                            cc.push(val);
                            break;
                        }
                        case InstType::StrConst:
                        {
                            throw std::runtime_error("Cannot push StrConst!");
                            break;
                        }
                        default:
                        {
                          break;
                        }
                    }
                    break;
                }
                case InstType::Pop:
                {
                    break;
                }
                case InstType::Return:
                {
                    break;
                }
                case InstType::Call:
                {
                    break;
                }

                case InstType::FuncSignature:
                {
                    std::cout << "func signature!\n";
                }

                default:
                    break;
            }
        }


        for (const auto& successor : current_bb->getSuccessors())
        {
            if (visited.find(successor) == visited.end())
            {
              auto newSuccessor =
                  std::make_shared<BasicBlock>(successor->getName());
                visited.insert(successor);
                worklist.push(successor);
            }
        }
    }

    cc.ret();
    cc.endFunc();   // End of the function body.
    cc.finalize();  // Translate and assemble the whole 'cc' content.    

    std::cout << "==================================" << std::endl;
    Func fn;
    asmjit::Error err = rt.add(&fn, &code);
    std::cout << logger.content().data() << std::endl;

    if (err)
    {
        printf("AsmJit failed: %s\n", asmjit::DebugUtils::errorAsString(err));
        return;
    }

    fn();
    std::cout << "JIT executed!\n";

    rt.release(fn);
}

asmjit::x86::Gp IRVisitor::getFirstArgumentRegister(asmjit::x86::Compiler& cc)
{
    asmjit::x86::Gp arg1_reg = (cc.environment().isPlatformWindows())
        ? asmjit::x86::rcx   // 1st argument on
                             // Windows
        : asmjit::x86::rdi;  // 1st argument on
                             // Linux/macOS
    return arg1_reg;
}
asmjit::x86::Gp IRVisitor::getSecondArgumentRegister(asmjit::x86::Compiler& cc)
{
    asmjit::x86::Gp arg2_reg = (cc.environment().isPlatformWindows())
        ? asmjit::x86::rdx  // 2nd argument on
                            // Windows
        : asmjit::x86::rsi; // 2nd argument on
                            // Linux/macOS
    return arg2_reg;
}


void IRVisitor::syscallPutChar(asmjit::x86::Compiler& cc, char c)
{
    asmjit::x86::Gp arg1_reg = getFirstArgumentRegister(cc);
    cc.mov(arg1_reg, c);
    cc.sub(asmjit::x86::rsp, 32);
    cc.call(putchar);
    cc.add(asmjit::x86::rsp, 32);
}

// implement printf("%d", val);
void IRVisitor::syscallPrintInt(asmjit::x86::Compiler& cc, int val)
{
    asmjit::x86::Gp arg1_reg = getFirstArgumentRegister(cc);
    asmjit::x86::Gp arg2_reg = getSecondArgumentRegister(cc);
    
    // Allocate 16 bytes on the stack for the format string and the integer.
    cc.sub(asmjit::x86::rsp, 16);
    
    // Write "%d\0" as a string to [rsp]. "%d" is 0x6425, null terminator is 0.
    cc.mov(asmjit::x86::word_ptr(asmjit::x86::rsp), 0x6425);  // "%d"
    cc.mov(asmjit::x86::byte_ptr(asmjit::x86::rsp, 2), 0);    // null terminator
    
    cc.lea(arg1_reg, asmjit::x86::ptr(asmjit::x86::rsp));
    cc.mov(arg2_reg, val);

    cc.sub(asmjit::x86::rsp, 32);
    cc.call(printf);
    cc.add(asmjit::x86::rsp, 32);
    
    // Restore stack
    cc.add(asmjit::x86::rsp, 16);
}

void IRVisitor::syscallPrintString(asmjit::x86::Compiler& cc, std::string& str)
{
  for (int i = 0; i < str.size(); ++i)
  {
      syscallPutChar(cc, str[i]);
  }
}

void IRVisitor::syscallScanInt(asmjit::x86::Compiler& cc, asmjit::x86::Gp reg)
{
    // currently, can only scan one digit integer
    asmjit::x86::Gp arg1_reg = getFirstArgumentRegister(cc);

    cc.sub(asmjit::x86::rsp, 32);
    cc.call(asmjit::Imm(getchar));
    cc.add(asmjit::x86::rsp, 32);
    cc.sub(asmjit::x86::eax, '0');
    cc.mov(reg, asmjit::x86::eax);
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
