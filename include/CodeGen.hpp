#pragma once

#include "SSA.hpp"
#include "BasicBlock.hpp"
#include "MachineIR.hpp"

class CodeGen
{
	SSA m_ssa;

	std::vector<std::shared_ptr<BasicBlock>> m_linearizedBlocks;
	std::vector<std::shared_ptr<BasicBlockMIR>> m_mirBlocks;

public:
	CodeGen(SSA ssa);
	void setSSA(SSA& SSA);

	void linearizeCFG();
	void generateMIR();
};