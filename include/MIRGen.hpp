#pragma once
#include "MachineIR.hpp"

#include <vector>
#include <memory>

class MIRGen
{
private:
	 std::vector<std::shared_ptr<MachineIR>> m_instructions;
public:
	MIRGen() = default;
	//void addBB(std::shared_ptr<BasicBlockMIR> bb);
};
