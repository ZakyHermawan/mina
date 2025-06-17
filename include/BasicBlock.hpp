#pragma once

#include <string>
#include <vector>
#include <memory>

class Inst;

class BasicBlock : public std::enable_shared_from_this<BasicBlock>
{
private:
    std::string m_name;
    std::vector<std::shared_ptr<Inst>> m_instructions;
    std::vector<std::shared_ptr<BasicBlock>> m_predecessors,
        m_successors;  // predecessors and successors

public:
    BasicBlock() = default;
    BasicBlock(std::string name);
    void setInstructions(std::vector<std::shared_ptr<Inst>> instructions);
    std::vector<std::shared_ptr<Inst>>& getInstructions();

    void pushInst(std::shared_ptr<Inst> inst);
    void pushInstBegin(std::shared_ptr<Inst> inst);

    void popInst();
    void setPredecessors(std::vector<std::shared_ptr<BasicBlock>> predecessors);
    void setSuccessors(std::vector<std::shared_ptr<BasicBlock>> successors);
    std::vector<std::shared_ptr<BasicBlock>> getPredecessors();
    std::vector<std::shared_ptr<BasicBlock>> getSuccessors();
    void pushPredecessor(const std::shared_ptr<BasicBlock>& predecessor);
    void pushSuccessor(const std::shared_ptr<BasicBlock>& successor);

    virtual ~BasicBlock() = default;
    BasicBlock(const BasicBlock&) = delete;
    BasicBlock(BasicBlock&&) noexcept = default;
    BasicBlock& operator=(const BasicBlock&) = delete;
    BasicBlock& operator=(BasicBlock&&) noexcept = default;

    std::string getName();
};
