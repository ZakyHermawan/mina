#include "SSA.hpp"
#include "InstIR.hpp"
#include "BasicBlock.hpp"

#include <memory>
#include <string>
#include <vector>
#include <iostream>
#include <unordered_map>

namespace mina
{

SSA::SSA()
    : m_cfg{std::make_shared<BasicBlock>("Entry_0")},
      m_currBBNameWithoutCtr{"Entry"},
      m_currBBCtr{0},
      m_currentBB{}
{
}

void SSA::setCFG(std::shared_ptr<BasicBlock> cfg) { m_cfg = cfg; }

std::string SSA::baseNameToSSA(const std::string& name)
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

std::string SSA::getCurrentSSAName(const std::string& name)
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

void SSA::printCFG()
{
    auto rpo = getRPONodes(m_cfg);
    for (auto& node : rpo)
    {
        auto& currBlock = node;
        std::cout << currBlock->getName() << ":\n";
        auto& inst = currBlock->getInstructions();
        for (int j = 0; j < inst.size(); ++j)
        {
            // Don't print function signature
            //if (inst[j]->getInstType() == InstType::Func) continue;
            std::cout << inst[j]->getString() << std::endl;
        }
        std::cout << std::endl;
    }
}

std::string SSA::getBaseName(std::string name)
{
    auto pos = name.find_last_of(".");
    return name.substr(0, pos);
}

std::string& SSA::getCurrBBNameWithoutCtr() { return m_currBBNameWithoutCtr; }

void SSA::incCurrBBCtr() { ++m_currBBCtr; }

int SSA::getCurrBBCtr() const { return m_currBBCtr; }

void SSA::setCurrBB(std::shared_ptr<BasicBlock> bb) { m_currentBB = bb; }

std::shared_ptr<BasicBlock> SSA::getCurrBB() { return m_currentBB; }

std::shared_ptr<BasicBlock> SSA::getCFG() { return m_cfg; }

void SSA::writeVariable(std::string varName, std::shared_ptr<BasicBlock> block,
                        std::shared_ptr<Inst> value)
{
    m_currDef[block][varName] = value;
}
std::shared_ptr<Inst> SSA::readVariable(std::string varName,
                                        std::shared_ptr<BasicBlock> block)
{
    if (m_currDef[block].find(varName) != m_currDef[block].end())
    {
        return m_currDef[block][varName];
    }
    return readVariableRecursive(varName, block);
}
std::shared_ptr<Inst> SSA::readVariableRecursive(
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

std::shared_ptr<Inst> SSA::addPhiOperands(std::string varName,
                                          std::shared_ptr<PhiInst> phi)
{
    auto preds = phi->getBlock()->getPredecessors();
    for (auto& pred : preds)
    {
        auto val = readVariable(varName, pred);
        phi->appendOperand(val, pred);
    }
    
    return tryRemoveTrivialPhi(phi);
}

std::shared_ptr<Inst> SSA::tryRemoveTrivialPhi(std::shared_ptr<PhiInst> phi)
{
    std::shared_ptr<Inst> same = nullptr;
    auto& operandList = phi->getOperands();
    for (auto& op : operandList)
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
        const auto& block = user->getBlock();
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
                        same->push_user(inst);
                        break;
                    }
                }
            }
        }
    }
    
    // remove phi from the hash table
    const auto& block = phi->getBlock();
    for (auto& [varName, value] : m_currDef[block])
    {
      if (value == phi)
      {
        m_currDef[block][varName] = same;
      }
    }
    
    // remove phi from instructions
    for (auto it = block->getInstructions().begin();
         it != block->getInstructions().end();)
    {
        if (*it == phi)  // Compare shared_ptr directly for equality
        {
            // erase returns an iterator to the next element
            it = block->getInstructions().erase(it);
        }
        else
        {
            ++it;  // Move to the next element only if not erased
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

void SSA::sealBlock(std::shared_ptr<BasicBlock> block)
{
    for (const auto& [var, val] : m_incompletePhis[block])
    {
        addPhiOperands(var, m_incompletePhis[block][var]);
    }
    m_sealedBlocks.insert(block);
}

// Implement method I (Naive Translation) from paper
// Optimizing translation out of SSA using renaming constraints.
void SSA::renameSSA()
{
    auto rpo = getRPONodes(getCFG());
    //printCFG();
    for (auto& node : rpo)
    {
        std::shared_ptr<BasicBlock> current_bb = node;
        auto& instructions = current_bb->getInstructions();
        for (unsigned int inst_idx = 0; inst_idx < instructions.size();
             ++inst_idx)
        {
            auto& currInst = instructions[inst_idx];
            const auto& target = currInst->getTarget();
            const auto& targetStr = target->getString();
            
            if (currInst->isPhi())
            {
                auto phiInst = std::dynamic_pointer_cast<PhiInst>(currInst);
                auto& operands = phiInst->getOperands();
                for (int i = 0; i < operands.size(); ++i)
                {
                    const auto& opStr = operands[i]->getString();
                    auto predBlock = phiInst->getOperandBB(i);
                    auto newTargetInst =
                        std::make_shared<IdentInst>(targetStr + "'", predBlock);
                    // predBlock->pushInst(std::make_shared<AssignInst>(
                    //     newTargetInst, operands[i]->getTarget(), predBlock));
                    auto& lastInst =
                        predBlock
                            ->getInstructions()[predBlock->getInstructions().size() - 1];
                    if (lastInst->getInstType() == InstType::BRT ||
                        lastInst->getInstType() == InstType::BRF)
                    {
                        predBlock->insertInstAtIndex(
                            predBlock->getInstructions().size() - 2,
                            std::make_shared<AssignInst>(newTargetInst,
                                                        operands[i]->getTarget(),
                                                        predBlock));
                    }
                    else
                    {
                        predBlock->insertInstAtIndex(
                            predBlock->getInstructions().size() - 1,
                            std::make_shared<AssignInst>(newTargetInst,
                                                        operands[i]->getTarget(),
                                                        predBlock));
                    }
                }

                // Remove phi instruction and add assign instruction at the
                // beginning of the block
                current_bb->removeInstAtIndex(inst_idx);

                // reset index to the beginning of the block since we
                // just added a new instruction at the beginning
                inst_idx = -1;

                if (operands.size() == 0)
                {
                    continue;
                }

                auto newTargetInst =
                    std::make_shared<IdentInst>(targetStr, current_bb);
                auto whatever =
                    std::make_shared<IdentInst>(targetStr + "'", current_bb);
                current_bb->pushInstBegin(std::make_shared<AssignInst>(
                    newTargetInst, whatever, current_bb));
            }
        }
    }
    //printCFG();
}

}  // namespace mina
