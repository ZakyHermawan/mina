#include "BB.hpp"
#include "InstIR.hpp"

BB::BB(std::string name) : m_name(std::move(name)) {}

void BB::setInstructions(
    std::vector<std::shared_ptr<Inst>> instructions)
{
    m_instructions = std::move(instructions);
}

std::vector<std::shared_ptr<Inst>>& BB::getInstructions()
{
    return m_instructions;
}

void BB::pushInst(std::shared_ptr<Inst> inst)
{
    m_instructions.push_back(inst);
}

void BB::pushInstBegin(std::shared_ptr<Inst> inst)
{
    m_instructions.insert(m_instructions.begin(), inst);
}

void BB::popInst() { m_instructions.pop_back(); }

void BB::setPredecessors(
    std::vector<std::shared_ptr<BB>> predecessors)
{
    m_predecessors = std::move(predecessors);
}

void BB::setSuccessors(std::vector<std::shared_ptr<BB>> successors)
{
    m_successors = std::move(successors);
}

std::vector<std::shared_ptr<BB>> BB::getPredecessors()
{
    return m_predecessors;
}

std::vector<std::shared_ptr<BB>> BB::getSuccessors()
{
    return m_successors;
}

void BB::pushPredecessor(const std::shared_ptr<BB>& predecessor)
{
  m_predecessors.push_back(predecessor);
}

void BB::pushSuccessor(const std::shared_ptr<BB>& successor)
{
  m_successors.push_back(successor);
}

std::string BB::getName() { return m_name; }
