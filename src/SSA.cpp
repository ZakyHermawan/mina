#include "SSA.hpp"
#include "InstIR.hpp"

#include <queue>
#include <set>
#include <iostream>

SSA::SSA()
    : m_cfg{std::make_shared<BasicBlock>("Entry_0")},
      m_currBBNameWithoutCtr {"Entry"},
      m_currBBCtr {0},
      m_currentBB{}
    {}

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
    // --- Step 0: Handle the case of an empty CFG ---
    if (!m_cfg) {
        std::cerr << "CFG is empty or not initialized." << std::endl;
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
        std::cout << "\n" << current_bb->getName() << ":" << std::endl;
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
                        same->push_user(inst);
                        break;
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
void SSA::sealBlock(std::shared_ptr<BasicBlock> block)
{
    for (const auto& [var, val] : m_incompletePhis[block])
    {
        addPhiOperands(var, m_incompletePhis[block][var]);
    }
    m_sealedBlocks.insert(block);
}
