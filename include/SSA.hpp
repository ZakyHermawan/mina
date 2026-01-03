#pragma once

#include "BasicBlock.hpp"
#include "InstIR.hpp"

#include <string>
#include <memory>
#include <unordered_map>
#include <unordered_set>

using subMap = std::unordered_map<std::string, std::shared_ptr<Inst>>;
using subPhi = std::unordered_map<std::string, std::shared_ptr<PhiInst>>;

class SSA
{
public:
	SSA();
    void setCFG(std::shared_ptr<BasicBlock>);

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

    void writeVariable(std::string varName, std::shared_ptr<BasicBlock> block,
                       std::shared_ptr<Inst> value);
    std::shared_ptr<Inst> readVariable(std::string varName,
                                       std::shared_ptr<BasicBlock> block);
    std::shared_ptr<Inst> readVariableRecursive(
        std::string varName, std::shared_ptr<BasicBlock> block);
    std::shared_ptr<Inst> addPhiOperands(std::string varName,
                                         std::shared_ptr<PhiInst> phi);
    std::shared_ptr<Inst> tryRemoveTrivialPhi(std::shared_ptr<PhiInst> phi);
    void sealBlock(std::shared_ptr<BasicBlock> block);
    
    void renameSSA();

private:
    std::string m_currBBNameWithoutCtr;
    int m_currBBCtr;
    std::shared_ptr<BasicBlock> m_cfg, m_currentBB;
    std::unordered_map<std::string, int> m_nameCtr;

    std::unordered_set<std::shared_ptr<BasicBlock>> m_sealedBlocks;
    std::unordered_map<std::shared_ptr<BasicBlock>, subMap> m_currDef;
    std::unordered_map<std::shared_ptr<BasicBlock>, subPhi> m_incompletePhis;
};
