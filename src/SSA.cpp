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
