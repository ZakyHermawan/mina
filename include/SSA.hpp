#pragma once

#include "BasicBlock.hpp"

#include <string>
#include <memory>
#include <unordered_map>

class SSA
{
public:
	SSA();

    std::string baseNameToSSA(const std::string& name);
    std::string getCurrentSSAName(const std::string& name);
    void printCFG();
    std::string getBaseName(std::string name);

    std::string& getCurrBBNameWithoutCtr();
    void incCurrBBCtr();
    int getCurrBBCtr() const;

    std::shared_ptr<BasicBlock> getCurrBB();
    void setCurrBB(std::shared_ptr<BasicBlock> bb);

    std::shared_ptr<BasicBlock> getCFG();

private:
    std::string m_currBBNameWithoutCtr;
    int m_currBBCtr;
    std::shared_ptr<BasicBlock> m_cfg, m_currentBB;
    std::unordered_map<std::string, int> m_nameCtr;
};
