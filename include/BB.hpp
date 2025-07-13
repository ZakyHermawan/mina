#pragma once

#include <string>
#include <vector>
#include <memory>

class Inst;

class BB : public std::enable_shared_from_this<BB>
{
private:
    std::string m_name;
    std::vector<std::shared_ptr<Inst>> m_instructions;
    std::vector<std::shared_ptr<BB>> m_predecessors,
        m_successors;  // predecessors and successors

public:
    BB() = default;
    BB(std::string name);
    void setInstructions(std::vector<std::shared_ptr<Inst>> instructions);
    std::vector<std::shared_ptr<Inst>>& getInstructions();

    void pushInst(std::shared_ptr<Inst> inst);
    void pushInstBegin(std::shared_ptr<Inst> inst);

    void popInst();
    void setPredecessors(std::vector<std::shared_ptr<BB>> predecessors);
    void setSuccessors(std::vector<std::shared_ptr<BB>> successors);
    std::vector<std::shared_ptr<BB>> getPredecessors();
    std::vector<std::shared_ptr<BB>> getSuccessors();
    void pushPredecessor(const std::shared_ptr<BB>& predecessor);
    void pushSuccessor(const std::shared_ptr<BB>& successor);

    virtual ~BB() = default;
    BB(const BB&) = delete;
    BB(BB&&) noexcept = default;
    BB& operator=(const BB&) = delete;
    BB& operator=(BB&&) noexcept = default;

    std::string getName();
};
