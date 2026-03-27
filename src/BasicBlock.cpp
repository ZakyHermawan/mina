#include "BasicBlock.hpp"
#include "InstIR.hpp"

#include <set>
#include <vector>
#include <string>
#include <utility>
#include <memory>
#include <algorithm>
#include <functional>

namespace mina
{

BasicBlock::BasicBlock(std::string name)
    : m_name(std::move(name))
{
}

void BasicBlock::setInstructions(
    std::vector<std::shared_ptr<Inst>> instructions)
{
    m_instructions = std::move(instructions);
}

std::vector<std::shared_ptr<Inst>>& BasicBlock::getInstructions()
{
    return m_instructions;
}

void BasicBlock::pushInst(std::shared_ptr<Inst> inst)
{
    m_instructions.push_back(inst);
}

void BasicBlock::pushInstBegin(std::shared_ptr<Inst> inst)
{
    m_instructions.insert(m_instructions.begin(), inst);
}

void BasicBlock::popInst() { m_instructions.pop_back(); }

void BasicBlock::setPredecessors(
    std::vector<std::shared_ptr<BasicBlock>> predecessors)
{
    m_predecessors = std::move(predecessors);
}

void BasicBlock::setSuccessors(std::vector<std::shared_ptr<BasicBlock>> successors)
{
    m_successors = std::move(successors);
}

std::vector<std::shared_ptr<BasicBlock>> BasicBlock::getPredecessors()
{
    return m_predecessors;
}

std::vector<std::shared_ptr<BasicBlock>> BasicBlock::getSuccessors()
{
    return m_successors;
}

void BasicBlock::pushPredecessor(const std::shared_ptr<BasicBlock>& predecessor)
{
  m_predecessors.push_back(predecessor);
}

void BasicBlock::pushSuccessor(const std::shared_ptr<BasicBlock>& successor)
{
  m_successors.push_back(successor);
}

std::string BasicBlock::getName() { return m_name; }

std::vector<std::shared_ptr<BasicBlock>> getRPONodes(
    std::shared_ptr<BasicBlock> root)
{
    std::vector<std::shared_ptr<BasicBlock>> rpo;
    std::set<std::shared_ptr<BasicBlock>> rpo_visited;
    std::function<void(std::shared_ptr<BasicBlock>)> dfs =
        [&](std::shared_ptr<BasicBlock> bb)
    {
        rpo_visited.insert(bb);
        const auto& successors = bb->getSuccessors();

        // Traverse successors in reverse order so the first element that is
        // being inserted
        // into the successors vector is in front of successors that is added
        // later, since we will reverse the linearizedBlocks at the end.
        for (int i = successors.size() - 1; i >= 0; --i)
        {
            auto& succ = successors[i];
            if (rpo_visited.find(succ) == rpo_visited.end())
            {
                dfs(succ);
            }
        }
        rpo.push_back(bb);
    };

    dfs(root);
    std::reverse(rpo.begin(), rpo.end());

    return rpo;
}

}  // namespace mina
