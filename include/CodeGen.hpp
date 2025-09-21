#pragma once

#include "SSA.hpp"

#include <asmjit/x86.h>

#include <map>
#include <utility>
#include <string>

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

	std::vector<asmjit::FuncNode*> m_funcNodes;

	using funcNodePair = std::pair<asmjit::FuncNode*, asmjit::FuncSignature>;
	std::map<std::string, funcNodePair> m_funcMap;

public:
	CodeGen(SSA ssa);
	void setSSA(SSA& SSA);

	asmjit::x86::Gp CodeGen::getFirstArgumentRegister();
	asmjit::x86::Gp CodeGen::getSecondArgumentRegister();

	void syscallPutChar(char c);
    void syscallPrintInt(int val);
    void syscallPrintString(std::string& str);
	void syscallScanInt(asmjit::x86::Gp reg);

	void generateFuncNode(std::string& funcName, bool haveRet,
						  unsigned int numberOfArg);

	void generateX86(std::string functionName);
	void executeJIT();
};