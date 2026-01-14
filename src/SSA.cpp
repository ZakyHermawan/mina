#include "SSA.hpp"
#include "InstIR.hpp"
#include "BasicBlock.hpp"
#include "DisjointSetUnion.hpp"

#include <set>
#include <queue>
#include <memory>
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <functional>
#include <unordered_map>

SSA::SSA()
    : m_cfg{std::make_shared<BasicBlock>("Entry_0")},
      m_currBBNameWithoutCtr {"Entry"},
      m_currBBCtr {0},
      m_currentBB{}
    {}

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
    // Implement Reverse Post-Order Traversal to linearize the CFG
    std::set<std::shared_ptr<BasicBlock>> visited;
    std::vector<std::shared_ptr<BasicBlock>> linearizedBlocks;
    std::function<void(std::shared_ptr<BasicBlock>)> dfs =
        [&](std::shared_ptr<BasicBlock> bb)
        {
          visited.insert(bb);
          const auto& successors = bb->getSuccessors();
          // Traverse successors in reverse order so the first element that is being inserted 
          // into the successors vector is in front of successors that is added later,
          // since we will reverse the linearizedBlocks at the end.
          for (int i = successors.size() - 1; i >= 0; --i)
          {
              auto& succ = successors[i];
              if (visited.find(succ) == visited.end())
              {
                  dfs(succ);
              }
          }
          linearizedBlocks.push_back(bb);
    };
    dfs(m_cfg);
    std::reverse(linearizedBlocks.begin(), linearizedBlocks.end());

    for (int i = 0; i < linearizedBlocks.size(); ++i)
    {
        auto& currBlock = linearizedBlocks[i];
        std::cout << currBlock->getName() << ":\n";
        auto& inst = currBlock->getInstructions();
        for (int j = 0; j < inst.size(); ++j)
        {
            // Don't print function signature
            if (inst[j]->getInstType() == InstType::Func) continue;
            std::cout << inst[j]->getString() << std::endl;
        }
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

void SSA::writeVariable(
    std::string varName,
    std::shared_ptr<BasicBlock> block,
    std::shared_ptr<Inst> value)
{
    m_currDef[block][varName] = value;
}
std::shared_ptr<Inst> SSA::readVariable(
    std::string varName,
    std::shared_ptr<BasicBlock> block)
{
    if (m_currDef[block].find(varName) != m_currDef[block].end())
    {
        return m_currDef[block][varName];
    }
    return readVariableRecursive(varName, block);
}
std::shared_ptr<Inst> SSA::readVariableRecursive(
    std::string varName,
    std::shared_ptr<BasicBlock> block)
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
std::shared_ptr<Inst> SSA::addPhiOperands(
    std::string varName,
    std::shared_ptr<PhiInst> phi)
{
    auto preds = phi->getBlock()->getPredecessors();
    for (auto& pred : preds)
    {
        auto val = readVariable(varName, pred);
        phi->appendOperand(val);
    }

    return tryRemoveTrivialPhi(phi);
}

std::shared_ptr<Inst> SSA::tryRemoveTrivialPhi(std::shared_ptr<PhiInst> phi)
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

void SSA::sealBlock(std::shared_ptr<BasicBlock> block)
{
    for (const auto& [var, val] : m_incompletePhis[block])
    {
        addPhiOperands(var, m_incompletePhis[block][var]);
    }
    m_sealedBlocks.insert(block);
}

// Use DSU data structure to collect all related variable name
// for example: x.0 and x.1 are related, so we group them together in one set
// later, we rename all of these related variable

// Please improve this algorithm using better out of SSA algorithm before implementing optimizations,
// some optimization like coalescing can make this algorithm wrong.
void SSA::renameSSA()
{
    DisjointSetUnion dsu;

    // BFS
    std::queue<std::shared_ptr<BasicBlock>> worklist;
    std::set<std::shared_ptr<BasicBlock>> visited;
    worklist.push(getCFG());
    visited.insert(getCFG());
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
            const auto& target = currInst->getTarget();
            const auto& targetStr = target->getString();
            dsu.make_set(targetStr);

            if (currInst->isPhi())
            {
                auto& operands = currInst->getOperands();
                for (int i = 0; i < operands.size(); ++i)
                {
                    const auto& opStr = operands[i]->getString();

                    // Merges two sets that targetStr and opStr belong to.
                    // Create phiweb.
                    dsu.unite(targetStr, opStr);
                }
            }
            else if (currInst->getInstType() == InstType::Put)
            {
                const auto& operandStr = currInst->getOperands()[0]->getTarget()->getString();
                variables.push_back(operandStr);
                dsu.make_set(operandStr);
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

    worklist.push(getCFG());

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
            const auto& target = currInst->getTarget();
            const auto& operands = currInst->getOperands();
            const auto& targetStr = target->getString();

            if (currInst->isPhi())
            {
                instructions.erase(instructions.begin() + instruction_idx);
                continue;
            }
            else if (currInst->getInstType() == InstType::ProcCall)
            {
                auto& operands = currInst->getOperands();
                for (int i = 0; i < operands.size(); ++i)
                {
                    if (operands[i]->canBeRenamed() == false)
                    {
                        continue;
                    }
                    const auto& operandTarget = operands[i]->getTarget();
                    const auto& operandTargetStr = operandTarget->getString();
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
            }
            else
            {
                if (root_to_new_name.find(targetStr) == root_to_new_name.end())
                {
                    throw std::runtime_error("can't find target str " + targetStr +
                                             "in the hashmap");
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
                    const auto& operandTarget = operands[i]->getTarget();
                    const auto& operandTargetStr = operandTarget->getString();
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
}
