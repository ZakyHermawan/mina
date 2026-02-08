#pragma once

#include "SSA.hpp"
#include "BasicBlock.hpp"
#include "MachineIR.hpp"

#include <map>
#include <set>
#include <memory>
#include <string>
#include <vector>

namespace mina
{

class CodeGen
{
	SSA m_ssa;

	std::vector<std::shared_ptr<BasicBlock>> m_linearizedBlocks;
	std::vector<std::shared_ptr<BasicBlockMIR>> m_mirBlocks;

	std::set<std::string> m_stringLiterals;
	std::map<std::string, SSA> m_functionSSAMap;
	std::map<std::string, std::shared_ptr<Register>> m_vregMap;

public:
	CodeGen(SSA ssa);
	void setSSA(SSA& SSA);

	void linearizeCFG();
	void generateMIR(bool isMain = false);
	void addSSA(std::string funcName, SSA& ssa);
	void generateAllFunctionsMIR();
};


}  // namespace mina
