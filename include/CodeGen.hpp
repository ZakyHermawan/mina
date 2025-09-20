#pragma once

#include "SSA.hpp"

#include <asmjit/x86.h>

class CodeGen
{
	SSA m_ssa;

	// Logger should always survive CodeHolder.
    asmjit::StringLogger m_logger;

    // Runtime designed for JIT - it holds relocated functions and controls
    // their lifetime.
    asmjit::JitRuntime m_rt;

    // Holds code and relocation information during code generation.
    asmjit::CodeHolder m_code;

	// compiler is used to generate code
	std::shared_ptr<asmjit::x86::Compiler> m_cc;

public:
	CodeGen(SSA ssa);

	asmjit::x86::Gp CodeGen::getFirstArgumentRegister(
        std::shared_ptr<asmjit::x86::Compiler> cc);
	asmjit::x86::Gp CodeGen::getSecondArgumentRegister(
		std::shared_ptr<asmjit::x86::Compiler> cc);

	void syscallPutChar(std::shared_ptr<asmjit::x86::Compiler> cc, char c);
    void syscallPrintInt(std::shared_ptr<asmjit::x86::Compiler> cc, int val); // implement printf("%d", val);
    void syscallPrintString(std::shared_ptr<asmjit::x86::Compiler> cc, std::string& str);
	void syscallScanInt(std::shared_ptr<asmjit::x86::Compiler> cc, asmjit::x86::Gp reg);

	void generateFunc(bool haveRet, unsigned int numberOfArg);
	void generateX86();
};