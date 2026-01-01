#include "CodeGen.hpp"
#include "BasicBlock.hpp"
#include "MachineIR.hpp"
#include "InstIR.hpp"

#include <asmjit/x86.h>
#include <asmjit/core/globals.h>

#include <functional>
#include <iostream>
#include <vector>
#include <memory>
#include <algorithm>
#include <set>
#include <map>

typedef int (*Func)(void);

CodeGen::CodeGen(SSA ssa) : m_ssa{ssa}, m_logger{}, m_rt{}, m_code{}
{
    m_code.init(m_rt.environment(), m_rt.cpu_features());
    m_code.set_logger(&m_logger);  // attach logger
    m_cc = std::make_shared<asmjit::x86::Compiler>(&m_code);
}

void CodeGen::setSSA(SSA& ssa) { m_ssa = ssa; }

asmjit::x86::Gp CodeGen::getFirstArgumentRegister()
{
    asmjit::x86::Gp arg1_reg = (m_cc->environment().is_platform_windows())
        ? asmjit::x86::rcx   // 1st argument on
                             // Windows
        : asmjit::x86::rdi;  // 1st argument on
                             // Linux/macOS
    return arg1_reg;
}
asmjit::x86::Gp CodeGen::getSecondArgumentRegister()
{
    asmjit::x86::Gp arg2_reg = (m_cc->environment().is_platform_windows())
        ? asmjit::x86::rdx  // 2nd argument on
                            // Windows
        : asmjit::x86::rsi; // 2nd argument on
                            // Linux/macOS
    return arg2_reg;
}

void CodeGen::syscallPutChar(char c)
{
    asmjit::x86::Gp arg1_reg = getFirstArgumentRegister();

    // spill for caller saved register
    m_cc->sub(asmjit::x86::rsp, 8);
    m_cc->mov(asmjit::x86::dword_ptr(asmjit::x86::rsp), arg1_reg);

    m_cc->mov(arg1_reg, c);
    m_cc->sub(asmjit::x86::rsp, 32);
    m_cc->call(putchar);
    m_cc->add(asmjit::x86::rsp, 32);

    // restore spilled register
    m_cc->mov(arg1_reg, asmjit::x86::dword_ptr(asmjit::x86::rsp));
    m_cc->add(asmjit::x86::rsp, 8);
}

// implement printf("%d", val);
void CodeGen::syscallPrintInt(int val)
{
    asmjit::x86::Gp arg1_reg = getFirstArgumentRegister();
    asmjit::x86::Gp arg2_reg = getSecondArgumentRegister();
    
    // spill for caller saved register
    m_cc->sub(asmjit::x86::rsp, 16);
    m_cc->mov(asmjit::x86::dword_ptr(asmjit::x86::rsp), arg1_reg);
    m_cc->mov(asmjit::x86::dword_ptr(asmjit::x86::rsp, 8), arg2_reg);

    //// Allocate 16 bytes on the stack for the format string and the integer.
    m_cc->sub(asmjit::x86::rsp, 16);
    
    //// Write "%d\0" as a string to [rsp]. "%d" is 0x6425, null terminator is 0.
    m_cc->mov(asmjit::x86::word_ptr(asmjit::x86::rsp), 0x6425);  // "%d"
    m_cc->mov(asmjit::x86::byte_ptr(asmjit::x86::rsp, 2), 0);    // null terminator
    
    m_cc->lea(arg1_reg, asmjit::x86::ptr(asmjit::x86::rsp));
    m_cc->mov(arg2_reg, val);

    m_cc->sub(asmjit::x86::rsp, 32);
    m_cc->call(printf);
    m_cc->add(asmjit::x86::rsp, 32);
    
    //// Restore stack
    m_cc->add(asmjit::x86::rsp, 16);

    // restore spilled register
    m_cc->mov(arg1_reg, asmjit::x86::dword_ptr(asmjit::x86::rsp));
    m_cc->mov(arg2_reg, asmjit::x86::dword_ptr(asmjit::x86::rsp, 8));
    m_cc->add(asmjit::x86::rsp, 16);
}

void CodeGen::syscallPrintString(std::string& str)
{
  for (int i = 0; i < str.size(); ++i)
  {
      syscallPutChar(str[i]);
  }
}

void CodeGen::syscallScanInt(asmjit::x86::Gp reg)
{
    // currently, can only scan one digit integer
    asmjit::x86::Gp arg1_reg = getFirstArgumentRegister();

    m_cc->sub(asmjit::x86::rsp, 32);
    m_cc->call(asmjit::Imm(getchar));
    m_cc->add(asmjit::x86::rsp, 32);
    m_cc->sub(asmjit::x86::rax, '0');
    m_cc->mov(reg, asmjit::x86::rax);
}

void CodeGen::generateFuncNode(std::string& funcName, bool haveRet, unsigned int numberOfArg)
{
    if (numberOfArg > 4)
    {
        throw std::runtime_error("Only support procedure up to 4 parameter");
    }

    asmjit::FuncNode* funcNode = nullptr;
    asmjit::FuncSignature funcSignature;

    switch (numberOfArg)
    {
        case 0:
            if (haveRet)
            {
                funcSignature = asmjit::FuncSignature::build<int>();
                funcNode = m_cc->new_func(funcSignature);
            }
            else
            {
                funcSignature = asmjit::FuncSignature::build<void>();
                funcNode = m_cc->new_func(asmjit::FuncSignature::build<void>());
            }
            break;
        case 1:
            if (haveRet)
            {
                funcSignature = asmjit::FuncSignature::build<int, int>();
                funcNode = m_cc->new_func(funcSignature);
            }
            else
            {
                funcSignature = asmjit::FuncSignature::build<void, int>();
                funcNode = m_cc->new_func(funcSignature);
            }
            break;
        case 2:
            if (haveRet)
            {
                funcSignature = asmjit::FuncSignature::build<int, int, int>();
                funcNode = m_cc->new_func(funcSignature);
            }
            else
            {
                funcSignature = asmjit::FuncSignature::build<void, int, int>();
                funcNode = m_cc->new_func(funcSignature);
            }
            break;
        case 3:
            if (haveRet)
            {
                funcSignature = asmjit::FuncSignature::build<int, int, int, int>();
                funcNode = m_cc->new_func(funcSignature);
            }
            else
            {
                funcSignature = asmjit::FuncSignature::build<void, int, int, int>();
                funcNode = m_cc->new_func(funcSignature);
            }
            break;
        case 4:
            if (haveRet)
            {
                funcSignature = asmjit::FuncSignature::build<int, int, int, int, int>();
                funcNode = m_cc->new_func(funcSignature);
            }
            else
            {
                funcSignature = asmjit::FuncSignature::build<void, int, int, int, int>();
                funcNode = m_cc->new_func(funcSignature);
            }
            break;
    }

    auto& it = m_funcMap.find(funcName);
    if (funcNode == nullptr)
    {
        throw std::runtime_error("func node nullptr");
    }

    if (it != m_funcMap.end())
    {
        throw std::runtime_error("function name in the IR already defined!");
    }
    m_funcMap[funcName] = std::make_pair(funcNode, funcSignature);
}

void CodeGen::linearizeCFG()
{
    // Implement Reverse Post-Order Traversal to linearize the CFG
    std::set<std::shared_ptr<BasicBlock>> visited;
    std::function<void(std::shared_ptr<BasicBlock>)> dfs =
        [&](std::shared_ptr<BasicBlock> bb)
        {
          visited.insert(bb);
          for (auto& succ : bb->getSuccessors())
          {
              if (visited.find(succ) == visited.end())
              {
                  dfs(succ);
              }
          }
          m_linearizedBlocks.push_back(bb);
    };
    dfs(m_ssa.getCFG());
    std::reverse(m_linearizedBlocks.begin(), m_linearizedBlocks.end());
}

void CodeGen::generateMIR()
{
    linearizeCFG();
    std::cout << "Instructions in Reverse Post-Order:\n";
    
    // Print instructions in linearized order
    for (unsigned int i = 0; i < m_linearizedBlocks.size(); ++i)
    {
      std::cout << m_linearizedBlocks[i]->getName() << ":\n";
      for (unsigned int j = 0;
           j < m_linearizedBlocks[i]->getInstructions().size(); ++j)
      {
        std::cout << "    "
                  << m_linearizedBlocks[i]->getInstructions()[j]->getString()
                  << std::endl;
      }
    }
    std::cout << std::endl;
    
    std::shared_ptr<Register> rbp{new Register{0, "rbp"}};
    std::shared_ptr<Register> rsp{new Register{1, "rsp"}};
    std::shared_ptr<Register> rax{new Register{2, "rax"}};
    std::shared_ptr<Register> rbx{new Register{3, "rbx"}};
    std::shared_ptr<Register> rcx{new Register{4, "rcx"}};
    std::shared_ptr<Register> rdx{new Register{5, "rdx"}};
    std::shared_ptr<Register> rip{new Register{6, "rip"}};

    std::map<std::string, unsigned int> vRegToOffset;
    std::vector<std::string> strLiterals;
    unsigned int strConstCtr = 0;

    auto assignVRegToOffsetIfDoesNotExist = [&](std::string targetStr)
    {
        if (vRegToOffset.find(targetStr) == vRegToOffset.end())
        {
            vRegToOffset[targetStr] = (vRegToOffset.size() + 1) * 8;
        }
    };

    auto leaToLabel = [&](std::string format_str)
    {
        auto literalMIR = std::make_shared<LiteralMIR>(format_str);
        auto memMIR = std::make_shared<MemoryMIR>(rip, literalMIR);
        auto leaMIR = std::make_shared<LeaMIR>(
            std::vector<std::shared_ptr<MachineIR>>{rcx, memMIR});
        return leaMIR;
    };
    
    auto memoryLocationForVReg = [&](std::string vReg)
    {
        unsigned int offset = vRegToOffset[vReg];
        auto offsetMIR = std::make_shared<ConstMIR>(-static_cast<int>(offset));
        auto memoryMIR = std::make_shared<MemoryMIR>(rbp, offsetMIR);
        return memoryMIR;
    };

    for (int i = 0; i < m_linearizedBlocks.size(); ++i)
    {
        auto& currBlock = m_linearizedBlocks[i];
        std::shared_ptr<BasicBlockMIR> bbMIR{
            new BasicBlockMIR{currBlock->getName()}};

        auto& inst = currBlock->getInstructions();

        for (int j = 0; j < inst.size(); ++j)
        {
            auto instType = inst[j]->getInstType();
            if (instType == InstType::Assign)
            {
                auto assignInst = std::dynamic_pointer_cast<AssignInst>(inst[j]);
                auto& operands = assignInst->getOperands();
                auto& targetStr = assignInst->getTarget()->getString();
                auto source = assignInst->getSource();

                assignVRegToOffsetIfDoesNotExist(targetStr);
              
                auto mirTarget = memoryLocationForVReg(targetStr);              
                auto sourceType = source->getInstType();

                if (sourceType == InstType::IntConst)
                {
                    auto intConstInst = std::dynamic_pointer_cast<IntConstInst>(source);
                    std::shared_ptr<MachineIR> mirSource{
                        new ConstMIR{intConstInst->getVal()}};

                    // mov QWORD PTR [target], constant
                    auto movMIR = std::make_shared<MovMIR>(
                        std::vector<std::shared_ptr<MachineIR>>{mirTarget,
                                                                mirSource});
                    bbMIR->addInstruction(movMIR);
                }
                else if (sourceType == InstType::Ident)
                {
                    auto identInst = std::dynamic_pointer_cast<IdentInst>(source);
                    std::shared_ptr<MachineIR> mirSource{memoryLocationForVReg(
                        identInst->getTarget()->getString())};

                    // Intermediate move to load from memory to register
                    std::shared_ptr<MovMIR> intermediateMovMIR{
                        new MovMIR{std::vector<std::shared_ptr<MachineIR>>{
                            rax, mirSource}}};
                    bbMIR->addInstruction(intermediateMovMIR);
                    
                    // mov QWORD PTR [target], rax
                    std::shared_ptr<MovMIR> finalMovMIR{
                        new MovMIR{std::vector<std::shared_ptr<MachineIR>>{
                            mirTarget, rax}}};
                    bbMIR->addInstruction(finalMovMIR);
                }
            }
            else if (instType == InstType::Put)
            {
                std::vector<std::shared_ptr<MachineIR>> mirOperands;
                auto putInst = std::dynamic_pointer_cast<PutInst>(inst[j]);
                auto& operands = putInst->getOperands();
                auto outputType = operands[0]->getTarget()->getInstType();
                if (outputType == InstType::IntConst)
                {
                    bbMIR->addInstruction(leaToLabel("fmt_str"));

                    auto intConstInst =
                        std::dynamic_pointer_cast<IntConstInst>(
                            operands[0]->getTarget());
                    auto constMIR =
                        std::make_shared<ConstMIR>(intConstInst->getVal());

                    // mov rdx, constant
                    auto movMIR = std::make_shared<MovMIR>(
                        std::vector<std::shared_ptr<MachineIR>>{rdx, constMIR});
                    bbMIR->addInstruction(movMIR);
                }
                else if (outputType == InstType::StrConst)
                {
                    auto outputInstr = std::dynamic_pointer_cast<StrConstInst>(
                        operands[0]->getTarget());
                    auto outputStr = outputInstr->getString();
                    if (outputStr == "'\\n'")
                    {
                        bbMIR->addInstruction(leaToLabel("newline_str"));
                    }
                    else
                    {
                        std::string sectionName =
                            "literal" + std::to_string(strConstCtr++);
                        bbMIR->addInstruction(leaToLabel(sectionName));

                        std::string literal = outputInstr->getString();
                        strLiterals.push_back(outputStr);
                    }
                }
                else if (outputType == InstType::Ident)
                {
                    bbMIR->addInstruction(leaToLabel("fmt_str"));

                    auto identInst = std::dynamic_pointer_cast<IdentInst>(
                        operands[0]->getTarget());

                    // mov rdx, QWORD PTR [identifier]
                    auto movMIR = std::make_shared<MovMIR>(
                        std::vector<std::shared_ptr<MachineIR>>{
                            rdx,
                            memoryLocationForVReg(identInst->getString())});
                    bbMIR->addInstruction(movMIR);
                }
                auto callMIR = std::make_shared<CallMIR>("printf");
                bbMIR->addInstruction(callMIR);
            }
            else if (instType == InstType::Get)
            {
                std::vector<std::shared_ptr<MachineIR>> mirOperands;
                auto getInst = std::dynamic_pointer_cast<GetInst>(inst[j]);
                auto& target = getInst->getTarget();

                bbMIR->addInstruction(leaToLabel("fmt_str"));

                // lea rdx, [target]
                auto targetStr = target->getString();
                auto argLeaMIR = std::make_shared<LeaMIR>(
                    std::vector<std::shared_ptr<MachineIR>>{
                        rdx, memoryLocationForVReg(targetStr)});
                bbMIR->addInstruction(argLeaMIR);

                auto callMIR = std::make_shared<CallMIR>("scanf");
                bbMIR->addInstruction(callMIR);
            }
            else if (instType == InstType::Add)
            {
                auto addInst = std::dynamic_pointer_cast<AddInst>(inst[j]);
                auto targetStr = addInst->getTarget()->getString(); // Temporary variable

                auto& operands = addInst->getOperands();
                auto& operand1 = operands[0]->getTarget();
                auto& operand2 = operands[1]->getTarget();

                assignVRegToOffsetIfDoesNotExist(targetStr);

                auto op1MemMIR = memoryLocationForVReg(operand1->getString()); // rax
                auto op2MemMIR = memoryLocationForVReg(operand2->getString()); // rbx
                auto targetMemMIR = memoryLocationForVReg(targetStr);

                // mov rax, op1
                auto movMIR1 = std::make_shared<MovMIR>(
                    std::vector<std::shared_ptr<MachineIR>>{rax,
                                                            op1MemMIR});
                bbMIR->addInstruction(movMIR1);

                // mov rbx, op2
                auto movMIR2 = std::make_shared<MovMIR>(
                    std::vector<std::shared_ptr<MachineIR>>{rbx,
                                                            op2MemMIR});
                bbMIR->addInstruction(movMIR2);

                // add rax, rbx
                auto addMIR = std::make_shared<AddMIR>(
                    std::vector<std::shared_ptr<MachineIR>>{rax, rbx});
                bbMIR->addInstruction(addMIR);

                // mov QOWRD PTR [target], rax
                auto movMIR3 = std::make_shared<MovMIR>(
                    std::vector<std::shared_ptr<MachineIR>>{targetMemMIR, rax});
                bbMIR->addInstruction(movMIR3);
            }
            else if (instType == InstType::Sub)
            {
                auto subInst = std::dynamic_pointer_cast<SubInst>(inst[j]);
                auto targetStr = subInst->getTarget()->getString(); // Temporary variable

                auto& operands = subInst->getOperands();
                auto& operand1 = operands[0]->getTarget();
                auto& operand2 = operands[1]->getTarget();

                assignVRegToOffsetIfDoesNotExist(targetStr);

                auto op1MemMIR = memoryLocationForVReg(operand1->getString()); // rax
                auto op2MemMIR = memoryLocationForVReg(operand2->getString()); // rbx
                auto targetMemMIR = memoryLocationForVReg(targetStr);

                // mov rax, op1
                auto movMIR1 = std::make_shared<MovMIR>(
                    std::vector<std::shared_ptr<MachineIR>>{rax,
                                                            op1MemMIR});
                bbMIR->addInstruction(movMIR1);

                // mov rbx, op2
                auto movMIR2 = std::make_shared<MovMIR>(
                    std::vector<std::shared_ptr<MachineIR>>{rbx,
                                                            op2MemMIR});
                bbMIR->addInstruction(movMIR2);

                // add rax, rbx
                auto subMIR = std::make_shared<SubMIR>(
                    std::vector<std::shared_ptr<MachineIR>>{rax, rbx});
                bbMIR->addInstruction(subMIR);

                // mov QOWRD PTR [target], rax
                auto movMIR3 = std::make_shared<MovMIR>(
                    std::vector<std::shared_ptr<MachineIR>>{targetMemMIR, rax});
                bbMIR->addInstruction(movMIR3);
            }
            else if (instType == InstType::Mul)
            {
                auto mulInst = std::dynamic_pointer_cast<MulInst>(inst[j]);
                auto targetStr = mulInst->getTarget()->getString(); // Temporary variable

                auto& operands = mulInst->getOperands();
                auto& operand1 = operands[0]->getTarget();
                auto& operand2 = operands[1]->getTarget();

                assignVRegToOffsetIfDoesNotExist(targetStr);

                auto op1MemMIR = memoryLocationForVReg(operand1->getString()); // rax
                auto op2MemMIR = memoryLocationForVReg(operand2->getString()); // rbx
                auto targetMemMIR = memoryLocationForVReg(targetStr);

                // mov rax, op1
                auto movMIR1 = std::make_shared<MovMIR>(
                    std::vector<std::shared_ptr<MachineIR>>{rax,
                                                            op1MemMIR});
                bbMIR->addInstruction(movMIR1);

                // mov rbx, op2
                auto movMIR2 = std::make_shared<MovMIR>(
                    std::vector<std::shared_ptr<MachineIR>>{rbx,
                                                            op2MemMIR});
                bbMIR->addInstruction(movMIR2);

                // imul rax, rbx
                auto mulMIR = std::make_shared<MulMIR>(
                    std::vector<std::shared_ptr<MachineIR>>{rax, rbx});
                bbMIR->addInstruction(mulMIR);

                // mov QOWRD PTR [target], rax
                auto movMIR3 = std::make_shared<MovMIR>(
                    std::vector<std::shared_ptr<MachineIR>>{targetMemMIR, rax});
                bbMIR->addInstruction(movMIR3);
            }
            else if (instType == InstType::Div)
            {
                auto divInst = std::dynamic_pointer_cast<DivInst>(inst[j]);
                auto targetStr = divInst->getTarget()->getString(); // Temporary variable

                auto& operands = divInst->getOperands();
                auto& operand1 = operands[0]->getTarget();
                auto& operand2 = operands[1]->getTarget();

                assignVRegToOffsetIfDoesNotExist(targetStr);

                auto op1MemMIR = memoryLocationForVReg(operand1->getString()); // rax
                auto op2MemMIR = memoryLocationForVReg(operand2->getString()); // rbx
                auto targetMemMIR = memoryLocationForVReg(targetStr);

                // mov rax, op1
                auto movMIR1 = std::make_shared<MovMIR>(
                    std::vector<std::shared_ptr<MachineIR>>{rax,
                                                            op1MemMIR});
                bbMIR->addInstruction(movMIR1);

                // cqo
                auto cqoMIR = std::make_shared<CqoMIR>();
                bbMIR->addInstruction(cqoMIR);

                // idiv QWORD PTR [op2]
                auto divMIR = std::make_shared<DivMIR>(op2MemMIR);
                bbMIR->addInstruction(divMIR);

                // mov QWORD PTR [target], rax
                auto movMIR2 = std::make_shared<MovMIR>(
                    std::vector<std::shared_ptr<MachineIR>>{targetMemMIR,
                                                            rax});
                bbMIR->addInstruction(movMIR2);
            }
        }
        m_mirBlocks.push_back(std::move(bbMIR));
    }

    std::cout << ".intel_syntax noprefix\n.globl main\nfmt_str: .string \"%d\"\n";
    for (unsigned int i = 0; i < strLiterals.size(); ++i)
    {
        std::cout << "literal" << std::to_string(i) << ": .string ";
        std::cout << strLiterals[i] << std::endl;
    }
    std::cout << ".section .text\nmain: \n";
    for (const auto& mirBlock : m_mirBlocks)
    {
        // Shadow space 32 byte and 4 byte for each variable
        unsigned int offset = 32 + vRegToOffset.size() * 4;
        unsigned int aligned_offset = (offset + 15) & ~15; // 16-byte alignment
        std::cout << mirBlock->getName() << ": \n";
        std::cout << "    push rbp\n    mov rbp, rsp\n";
        std::cout << "    sub rsp, " << aligned_offset << std::endl;
        mirBlock->printInstructions();
        std::cout << "    add rsp, 48\n";
        std::cout << "    mov rsp, rbp\n    pop rbp\n    ret\n";
    }
    std::cout << "\nnewline_str: .string \"\\n\"\n";
    std::cout << "\n";
}

/*
void CodeGen::generateX86(std::string funcName)
{
    // BFS
    std::queue<std::shared_ptr<BasicBlock>> worklist;
    std::set<std::shared_ptr<BasicBlock>> visited;
    //linearizeCFG();
    worklist.push(m_ssa.getCFG());
    visited.insert(m_ssa.getCFG());
    
    std::vector<std::string> variables;
    auto it = m_funcMap.find(funcName);
    if (it == m_funcMap.end())
    {
        m_cc->add_func(asmjit::FuncSignature::build<void>());
    }
    else
    {
        auto node = m_funcMap[funcName].first;
        m_cc->add_func(node);
    }

    std::unordered_map<std::string, asmjit::x86::Gp> registerMap;
    std::unordered_map<std::string, asmjit::Label> labelMap;

    while (!worklist.empty())
    {
        std::shared_ptr<BasicBlock> current_bb = worklist.front();
        auto bbName = current_bb->getName();

        if (labelMap.find(bbName) == labelMap.end())
        {
            labelMap[bbName] = m_cc->new_named_label(bbName.c_str());
        }
        auto& label = labelMap[bbName];
        m_cc->bind(label); // generate label and start writing code adter this label

        visited.insert(current_bb);
        worklist.pop();

        auto& instructions = current_bb->getInstructions();

        enum class CmpType
        {
            NONE,
            EQ,
            NE,
            LT,
            LTE,
            GT,
            GTE
        };

        struct CodegenState
        {
            CmpType last_comparison_type = CmpType::NONE;
        } state;

        bool is_prev_cmp = false;
        for (unsigned int i = 0; i < instructions.size(); ++i)
        {
            auto& currInst = instructions[i];
            auto instType = currInst->getInstType();
            switch (instType)
            {
                case InstType::Add:
                {
                    auto& target = currInst->getTarget();
                    auto& operands = currInst->getOperands();
                    auto& operand1 = operands[0]->getTarget();
                    auto& operand2 = operands[1]->getTarget();

                    auto& targetName = target->getString();
                    if (registerMap.find(targetName) == registerMap.end())
                    {
                        auto newReg = m_cc->new_gp64(targetName.c_str());
                        registerMap[targetName] = newReg;
                    }

                    auto& targetRegister = registerMap[targetName];
                    auto operandInstType = operand1->getInstType();
                    switch (operandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand1);
                            auto err = m_cc->mov(targetRegister, casted->getVal());
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand1);
                            m_cc->mov(targetRegister, casted->getVal());
                            break;
                        }
                        default:
                        {
                            m_cc->mov(
                                targetRegister,
                                registerMap[operand1->getTarget()->getString()]);
                            break;
                        }
                    }

                    auto otherOperandInstType = operand2->getInstType();

                    switch (otherOperandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand2);
                            m_cc->add(targetRegister, casted->getVal());
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand2);
                            m_cc->add(targetRegister, casted->getVal());
                            break;
                        }
                        default:
                        {
                            m_cc->add(
                                targetRegister,
                                registerMap[operand2->getTarget()->getString()]);
                            break;
                        }
                    }
                    break;
                }
                case InstType::Sub:
                {
                    auto& target = currInst->getTarget();
                    auto& operands = currInst->getOperands();
                    auto& operand1 = operands[0]->getTarget();
                    auto& operand2 = operands[1]->getTarget();

                    auto& targetName = target->getString();
                    if (registerMap.find(targetName) == registerMap.end())
                    {
                        auto newReg = m_cc->new_gp64(targetName.c_str());
                        registerMap[targetName] = newReg;
                    }

                    auto& targetRegister = registerMap[targetName];
                    auto operandInstType = operand1->getInstType();
                    switch (operandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand1);
                            m_cc->mov(targetRegister, casted->getVal());
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand1);
                            m_cc->mov(targetRegister, casted->getVal());
                            break;
                        }
                        default:
                        {
                            m_cc->mov(
                                targetRegister,
                                registerMap[operand1->getTarget()->getString()]);
                            break;
                        }
                    }

                    auto otherOperandInstType = operand2->getInstType();
                    switch (otherOperandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand2);
                            m_cc->sub(targetRegister, casted->getVal());
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand2);
                            m_cc->sub(targetRegister, casted->getVal());
                            break;
                        }
                        default:
                        {
                            m_cc->sub(
                                targetRegister,
                                registerMap[operand2->getTarget()->getString()]);
                            break;
                        }
                    }
                    break;
                }
                case InstType::Mul:
                {
                    auto& target = currInst->getTarget();
                    auto& operands = currInst->getOperands();
                    auto& operand1 = operands[0]->getTarget();
                    auto& operand2 = operands[1]->getTarget();

                    auto& targetName = target->getString();
                    if (registerMap.find(targetName) == registerMap.end())
                    {
                        auto newReg = m_cc->new_gp64(targetName.c_str());
                        registerMap[targetName] = newReg;
                    }

                    auto& targetRegister = registerMap[targetName];
                    auto operandInstType = operand1->getInstType();
                    switch (operandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand1);
                            m_cc->mov(targetRegister, casted->getVal());
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand1);
                            m_cc->mov(targetRegister, casted->getVal());
                            break;
                        }
                        default:
                        {
                            m_cc->mov(
                                targetRegister,
                                registerMap[operand1->getTarget()->getString()]);
                            break;
                        }
                    }

                    auto otherOperandInstType = operand2->getInstType();
                    switch (otherOperandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand2);
                            auto tempReg = m_cc->new_gp64();
                            m_cc->mov(tempReg, casted->getVal());
                            m_cc->imul(targetRegister, tempReg);
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand2);
                            auto tempReg = m_cc->new_gp64();
                            m_cc->mov(tempReg, casted->getVal());
                            m_cc->imul(targetRegister, tempReg);
                            break;
                        }
                        default:
                        {
                            m_cc->imul(
                                targetRegister,
                                registerMap[operand2->getTarget()->getString()]);
                            break;
                        }
                    }
                    break;
                }
                case InstType::Div:
                {
                    auto& target = currInst->getTarget();
                    auto& operands = currInst->getOperands();
                    auto& operand1 = operands[0]->getTarget();
                    auto& operand2 = operands[1]->getTarget();

                    auto& targetName = target->getString();
                    if (registerMap.find(targetName) == registerMap.end())
                    {
                        auto newReg = m_cc->new_gp64(targetName.c_str());
                        registerMap[targetName] = newReg;
                    }

                    auto& targetRegister = registerMap[targetName];
                    auto operandInstType = operand1->getInstType();
                    switch (operandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand1);
                            m_cc->mov(targetRegister, casted->getVal());
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand1);
                            m_cc->mov(targetRegister, casted->getVal());
                            break;
                        }
                        default:
                        {
                            m_cc->mov(
                                targetRegister,
                                registerMap[operand1->getTarget()->getString()]);
                            break;
                        }
                    }

                    auto otherOperandInstType = operand2->getInstType();
                    switch (otherOperandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand2);

                            auto a = m_cc->new_gp64("a");
                            auto b = m_cc->new_gp64("b");
                            auto dummy = m_cc->new_gp64("dummy");

                            m_cc->xor_(dummy, dummy);
                            m_cc->mov(a, targetRegister);
                            m_cc->mov(b, casted->getVal());
                            m_cc->idiv(dummy, a, b);
                            m_cc->mov(targetRegister, a);

                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand2);

                            auto a = m_cc->new_gp64("a");
                            auto b = m_cc->new_gp64("b");
                            auto dummy = m_cc->new_gp64("dummy");

                            m_cc->xor_(dummy, dummy);
                            m_cc->mov(a, targetRegister);
                            m_cc->mov(b, casted->getVal());
                            m_cc->idiv(dummy, a, b);
                            m_cc->mov(targetRegister, a);

                            break;
                        }
                        default:
                        {
                            auto a = m_cc->new_gp64("a");
                            auto b = m_cc->new_gp64("b");
                            auto dummy = m_cc->new_gp64("dummy");

                            m_cc->xor_(dummy, dummy);
                            m_cc->mov(a, targetRegister);
                            m_cc->mov(b, registerMap[operand2->getTarget()->getString()]);
                            m_cc->idiv(dummy, a, b);
                            m_cc->mov(targetRegister, a);

                            break;
                        }
                    }
                    break;
                }
                case InstType::Not:
                {
                    auto& target = currInst->getTarget();
                    auto& operands = currInst->getOperands();
                    auto& operand1 = operands[0]->getTarget();

                    auto& targetName = target->getString();
                    if (registerMap.find(targetName) == registerMap.end())
                    {
                        auto newReg = m_cc->new_gp64(targetName.c_str());
                        registerMap[targetName] = newReg;
                    }

                    auto& targetRegister = registerMap[targetName];
                    auto operandInstType = operand1->getInstType();
                    switch (operandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand1);
                            m_cc->mov(targetRegister, casted->getVal());
                            m_cc->neg(targetRegister);
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand1);
                            m_cc->mov(targetRegister, casted->getVal());
                            m_cc->neg(targetRegister);
                            break;
                        }
                        default:
                        {
                            m_cc->mov(
                                targetRegister,
                                registerMap[operand1->getTarget()->getString()]);
                            m_cc->neg(targetRegister);
                            break;
                        }
                    }
                    break;
                }
                case InstType::And:
                {
                    auto& target = currInst->getTarget();
                    auto& operands = currInst->getOperands();
                    auto& operand1 = operands[0]->getTarget();
                    auto& operand2 = operands[1]->getTarget();

                    auto& targetName = target->getString();
                    if (registerMap.find(targetName) == registerMap.end())
                    {
                        auto newReg = m_cc->new_gp64(targetName.c_str());
                        registerMap[targetName] = newReg;
                    }

                    auto& targetRegister = registerMap[targetName];
                    auto operandInstType = operand1->getInstType();
                    switch (operandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand1);
                            m_cc->mov(targetRegister, casted->getVal());
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand1);
                            m_cc->mov(targetRegister, casted->getVal());
                            break;
                        }
                        default:
                        {
                            m_cc->mov(
                                targetRegister,
                                registerMap[operand1->getTarget()->getString()]);
                            break;
                        }
                    }

                    auto otherOperandInstType = operand2->getInstType();
                    switch (otherOperandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand2);
                            auto tempReg = m_cc->new_gp64();
                            m_cc->mov(tempReg, casted->getVal());
                            m_cc->and_(targetRegister, tempReg);
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand2);
                            auto tempReg = m_cc->new_gp64();
                            m_cc->mov(tempReg, casted->getVal());
                            m_cc->and_(targetRegister, tempReg);
                            break;
                        }
                        default:
                        {
                            m_cc->and_(
                                targetRegister,
                                registerMap[operand2->getTarget()->getString()]);
                            break;
                        }
                    }
                    break;
                }
                case InstType::Or:
                {
                    auto& target = currInst->getTarget();
                    auto& operands = currInst->getOperands();
                    auto& operand1 = operands[0]->getTarget();
                    auto& operand2 = operands[1]->getTarget();

                    auto& targetName = target->getString();
                    if (registerMap.find(targetName) == registerMap.end())
                    {
                        auto newReg = m_cc->new_gp64(targetName.c_str());
                        registerMap[targetName] = newReg;
                    }

                    auto& targetRegister = registerMap[targetName];
                    auto operandInstType = operand1->getInstType();
                    switch (operandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand1);
                            m_cc->mov(targetRegister, casted->getVal());
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand1);
                            m_cc->mov(targetRegister, casted->getVal());
                            break;
                        }
                        default:
                        {
                            m_cc->mov(
                                targetRegister,
                                registerMap[operand1->getTarget()->getString()]);
                            break;
                        }
                    }

                    auto otherOperandInstType = operand2->getInstType();
                    switch (otherOperandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand2);
                            auto tempReg = m_cc->new_gp64();
                            m_cc->mov(tempReg, casted->getVal());
                            m_cc->or_(targetRegister, tempReg);
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand2);
                            auto tempReg = m_cc->new_gp64();
                            m_cc->mov(tempReg, casted->getVal());
                            m_cc->or_(targetRegister, tempReg);
                            break;
                        }
                        default:
                        {
                            m_cc->or_(
                                targetRegister,
                                registerMap[operand2->getTarget()->getString()]);
                            break;
                        }
                    }
                    break;
                }
                case InstType::Alloca:
                {
                    auto& target = currInst->getTarget();
                    auto allocaConverted =
                        std::dynamic_pointer_cast<AllocaInst>(currInst);
                    auto size = allocaConverted->getSize();
                    auto type = allocaConverted->getType();

                    auto& targetName = target->getString();

                    if (registerMap.find(targetName) == registerMap.end())
                    {
                        auto newReg = m_cc->new_gp64(targetName.c_str());
                        registerMap[targetName] = newReg;
                    }
                    auto& targetRegister = registerMap[targetName];
                    if (type == Type::BOOLEAN)
                    {
                        m_cc->sub(asmjit::x86::esp, size);
                        m_cc->mov(targetRegister, asmjit::x86::esp);
                    }
                    else if (type == Type::INTEGER)
                    {
                        m_cc->sub(asmjit::x86::esp, size * 4);
                        m_cc->mov(targetRegister, asmjit::x86::esp);
                    }
                    else
                    {
                        throw std::runtime_error("Unknown type");
                    }

                    break;
                }
                case InstType::ArrAccess:
                {
                    auto castedInst =
                          std::dynamic_pointer_cast<ArrAccessInst>(currInst);
                    auto& target = castedInst->getTarget();
                    auto& source = castedInst->getSource()->getTarget();
                    auto& index = castedInst->getIndex()->getTarget();

                    auto& sourceReg = registerMap[source->getString()];
                    auto targetReg = m_cc->new_gp64();
                    auto& targetName = target->getString();
                    registerMap[targetName] = targetReg;

                    auto indexType = castedInst->getType();
                    asmjit::x86::Gp indexTempReg;

                    switch (indexType)
                    {
                        case Type::INTEGER:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(index);
                            indexTempReg = m_cc->new_gp64();
                            m_cc->mov(indexTempReg, casted->getVal() * 4);
                            break;
                        }
                        case Type::BOOLEAN:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(index);
                            indexTempReg = m_cc->new_gp64();
                            m_cc->mov(indexTempReg, casted->getVal());
                            break;
                        }
                        default:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(index);
                            indexTempReg = registerMap[index->getTarget()->getString()];
                            m_cc->mov(indexTempReg, casted->getVal());
                            break;
                        }
                    }

                    m_cc->mov(targetReg, asmjit::x86::dword_ptr(sourceReg, indexTempReg));
                    break;
                }
                case InstType::ArrUpdate:
                {
                    auto castedInst =
                          std::dynamic_pointer_cast<ArrUpdateInst>(currInst);
                    auto& target = castedInst->getTarget();
                    auto& source = castedInst->getSource()->getTarget();
                    auto& index = castedInst->getIndex()->getTarget();
                    auto& val = castedInst->getVal()->getTarget();
                    auto& targetName = target->getString();
                    if (registerMap.find(targetName) == registerMap.end())
                    {
                        throw std::runtime_error(
                            "unknown target register name: " + targetName);
                    }
                    auto& targetRegister = registerMap[targetName];
                    auto& sourceRegister = registerMap[targetName];

                    auto indexType = castedInst->getType();
                    asmjit::x86::Gp indexTempReg;

                    switch (indexType)
                    {
                        case Type::INTEGER:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(index);
                            if (casted == nullptr)
                            {
                                throw std::runtime_error("error when casting pointer!\n");
                            }
                            indexTempReg = m_cc->new_gp64();
                            m_cc->mov(indexTempReg, casted->getVal() * 4);
                            break;
                        }
                        case Type::BOOLEAN:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(index);
                            if (casted == nullptr)
                            {
                                throw std::runtime_error("error when casting pointer!\n");
                            }
                            indexTempReg = m_cc->new_gp64();
                            m_cc->mov(indexTempReg, casted->getVal());
                            break;
                        }
                        default:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(index);
                            if (casted == nullptr)
                            {
                                throw std::runtime_error("error when casting pointer!\n");
                            }
                            indexTempReg = registerMap[index->getTarget()->getString()];
                            m_cc->mov(indexTempReg, casted->getVal());
                            break;
                        }
                    }

                    auto valueType = val->getInstType();
                    asmjit::x86::Gp valueTempReg;
                    switch (valueType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(val);
                            valueTempReg = m_cc->new_gp64();
                            m_cc->mov(valueTempReg, casted->getVal());
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(val);
                            valueTempReg = m_cc->new_gp64();
                            m_cc->mov(valueTempReg, casted->getVal());
                            break;
                        }
                        default:
                        {
                            valueTempReg =
                                registerMap[index->getTarget()->getString()];
                            break;
                        }
                    }

                    m_cc->mov(asmjit::x86::dword_ptr(sourceRegister, indexTempReg), valueTempReg);
                    break;
                }
                case InstType::Assign:
                {
                    if (is_prev_cmp == true)
                    {
                        break;
                    }
                    auto& target = currInst->getTarget();
                    auto& operands = currInst->getOperands();
                    auto& operand1 = operands[0]->getTarget();

                    auto& targetName = target->getString();
                    if (registerMap.find(targetName) == registerMap.end())
                    {
                        auto newReg = m_cc->new_gp64(targetName.c_str());
                        registerMap[targetName] = newReg;
                    }
                    auto& targetRegister = registerMap[targetName];
                    auto operandInstType = operand1->getInstType();
                    switch (operandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand1);
                            m_cc->mov(targetRegister, casted->getVal());
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand1);
                            m_cc->mov(targetRegister, casted->getVal());
                            break;
                        }
                        default:
                        {
                            m_cc->mov(
                                targetRegister,
                                registerMap[operand1->getString()]);
                            break;
                        }
                    }
                    break;
                }
                case InstType::CmpEq:
                {
                    auto cmpEqInst = std::dynamic_pointer_cast<CmpEQInst>(currInst);
                    auto& target = cmpEqInst->getTarget();
                    auto& operands = cmpEqInst->getOperands();
                    auto& operand1 = operands[0]->getTarget();
                    auto& operand2 = operands[1]->getTarget();

                    auto& targetName = target->getString();
                    if (registerMap.find(targetName) == registerMap.end())
                    {
                        auto newReg = m_cc->new_gp64(targetName.c_str());
                        registerMap[targetName] = newReg;
                    }

                    auto& targetRegister = registerMap[targetName];
                    auto operandInstType = operand1->getInstType();
                    switch (operandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand1);
                            m_cc->mov(targetRegister, casted->getVal());
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand1);
                            m_cc->mov(targetRegister, casted->getVal());
                            break;
                        }
                        default:
                        {
                            m_cc->mov(
                                targetRegister,
                                registerMap[operand1->getTarget()->getString()]);
                            break;
                        }
                    }

                    auto otherOperandInstType = operand2->getInstType();
                    switch (otherOperandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand2);
                            auto tempReg = m_cc->new_gp64();
                            m_cc->mov(tempReg, casted->getVal());
                            m_cc->cmp(targetRegister, tempReg);
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand2);
                            auto tempReg = m_cc->new_gp64();
                            m_cc->mov(tempReg, casted->getVal());
                            m_cc->cmp(targetRegister, tempReg);
                            break;
                        }
                        default:
                        {
                            m_cc->cmp(
                                targetRegister,
                                registerMap[operand2->getTarget()->getString()]);
                            break;
                        }
                    }
                    is_prev_cmp = true;
                    state.last_comparison_type = CmpType::EQ;
                    break;
                }
                case InstType::CmpNE:
                {
                    auto cmpNeInst = std::dynamic_pointer_cast<CmpNEInst>(currInst);
                    auto& target = cmpNeInst->getTarget();
                    auto& operands = cmpNeInst->getOperands();
                    auto& operand1 = operands[0]->getTarget();
                    auto& operand2 = operands[1]->getTarget();

                    auto& targetName = target->getString();
                    if (registerMap.find(targetName) == registerMap.end())
                    {
                        auto newReg = m_cc->new_gp64(targetName.c_str());
                        registerMap[targetName] = newReg;
                    }

                    auto& targetRegister = registerMap[targetName];
                    auto operandInstType = operand1->getInstType();
                    switch (operandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand1);
                            m_cc->mov(targetRegister, casted->getVal());
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand1);
                            m_cc->mov(targetRegister, casted->getVal());
                            break;
                        }
                        default:
                        {
                            m_cc->mov(
                                targetRegister,
                                registerMap[operand1->getTarget()->getString()]);
                            break;
                        }
                    }

                    auto otherOperandInstType = operand2->getInstType();
                    switch (otherOperandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand2);
                            auto tempReg = m_cc->new_gp64();
                            m_cc->mov(tempReg, casted->getVal());
                            m_cc->cmp(targetRegister, tempReg);
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand2);
                            auto tempReg = m_cc->new_gp64();
                            m_cc->mov(tempReg, casted->getVal());
                            m_cc->cmp(targetRegister, tempReg);
                            break;
                        }
                        default:
                        {
                            m_cc->cmp(
                                targetRegister,
                                registerMap[operand2->getTarget()->getString()]);
                            break;
                        }
                    }
                    is_prev_cmp = true;
                    state.last_comparison_type = CmpType::NE;
                    break;
                }
                case InstType::CmpLT:
                {
                    auto cmpLtInstr = std::dynamic_pointer_cast<CmpLTInst>(currInst);
                    auto& target = cmpLtInstr->getTarget();
                    auto& operands = cmpLtInstr->getOperands();
                    auto& operand1 = operands[0]->getTarget();
                    auto& operand2 = operands[1]->getTarget();

                    auto& targetName = target->getString();
                    if (registerMap.find(targetName) == registerMap.end())
                    {
                        auto newReg = m_cc->new_gp64(targetName.c_str());
                        registerMap[targetName] = newReg;
                    }

                    auto& targetRegister = registerMap[targetName];
                    auto operandInstType = operand1->getInstType();
                    switch (operandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand1);
                            m_cc->mov(targetRegister, casted->getVal());
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand1);
                            m_cc->mov(targetRegister, casted->getVal());
                            break;
                        }
                        default:
                        {
                            m_cc->mov(
                                targetRegister,
                                registerMap[operand1->getTarget()->getString()]);
                            break;
                        }
                    }

                    auto otherOperandInstType = operand2->getInstType();
                    switch (otherOperandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand2);
                            auto tempReg = m_cc->new_gp64();
                            m_cc->mov(tempReg, casted->getVal());
                            m_cc->cmp(targetRegister, tempReg);
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand2);
                            auto tempReg = m_cc->new_gp64();
                            m_cc->mov(tempReg, casted->getVal());
                            m_cc->cmp(targetRegister, tempReg);
                            break;
                        }
                        default:
                        {
                            m_cc->cmp(
                                targetRegister,
                                registerMap[operand2->getTarget()->getString()]);
                            break;
                        }
                    }
                    is_prev_cmp = true;
                    state.last_comparison_type = CmpType::LT;
                    break;
                }
                case InstType::CmpLTE:
                {
                    auto cmpLteInst = std::dynamic_pointer_cast<CmpLTEInst>(currInst);
                    auto& target = cmpLteInst->getTarget();
                    auto& operands = cmpLteInst->getOperands();
                    auto& operand1 = operands[0]->getTarget();
                    auto& operand2 = operands[1]->getTarget();

                    auto& targetName = target->getString();
                    if (registerMap.find(targetName) == registerMap.end())
                    {
                        auto newReg = m_cc->new_gp64(targetName.c_str());
                        registerMap[targetName] = newReg;
                    }

                    auto& targetRegister = registerMap[targetName];
                    auto operandInstType = operand1->getInstType();
                    switch (operandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand1);
                            m_cc->mov(targetRegister, casted->getVal());
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand1);
                            m_cc->mov(targetRegister, casted->getVal());
                            break;
                        }
                        default:
                        {
                            m_cc->mov(
                                targetRegister,
                                registerMap[operand1->getTarget()->getString()]);
                            break;
                        }
                    }

                    auto otherOperandInstType = operand2->getInstType();
                    switch (otherOperandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand2);
                            auto tempReg = m_cc->new_gp64();
                            m_cc->mov(tempReg, casted->getVal());
                            m_cc->cmp(targetRegister, tempReg);
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand2);
                            auto tempReg = m_cc->new_gp64();
                            m_cc->mov(tempReg, casted->getVal());
                            m_cc->cmp(targetRegister, tempReg);
                            break;
                        }
                        default:
                        {
                            m_cc->cmp(
                                targetRegister,
                                registerMap[operand2->getTarget()->getString()]);
                            break;
                        }
                    }
                    is_prev_cmp = true;
                    state.last_comparison_type = CmpType::LTE;
                    break;
                }
                case InstType::CmpGT:
                {
                    auto cmpGtInst = std::dynamic_pointer_cast<CmpGTInst>(currInst);
                    auto& target = cmpGtInst->getTarget();
                    auto& operands = cmpGtInst->getOperands();
                    auto& operand1 = operands[0]->getTarget();
                    auto& operand2 = operands[1]->getTarget();

                    auto& targetName = target->getString();
                    if (registerMap.find(targetName) == registerMap.end())
                    {
                        auto newReg = m_cc->new_gp64(targetName.c_str());
                        registerMap[targetName] = newReg;
                    }

                    auto& targetRegister = registerMap[targetName];
                    auto operandInstType = operand1->getInstType();
                    switch (operandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand1);
                            m_cc->mov(targetRegister, casted->getVal());
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand1);
                            m_cc->mov(targetRegister, casted->getVal());
                            break;
                        }
                        default:
                        {
                            m_cc->mov(
                                targetRegister,
                                registerMap[operand1->getTarget()->getString()]);
                            break;
                        }
                    }

                    auto otherOperandInstType = operand2->getInstType();
                    switch (otherOperandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand2);
                            auto tempReg = m_cc->new_gp64();
                            m_cc->mov(tempReg, casted->getVal());
                            m_cc->cmp(targetRegister, tempReg);
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand2);
                            auto tempReg = m_cc->new_gp64();
                            m_cc->mov(tempReg, casted->getVal());
                            m_cc->cmp(targetRegister, tempReg);
                            break;
                        }
                        default:
                        {
                            m_cc->cmp(
                                targetRegister,
                                registerMap[operand2->getTarget()->getString()]);
                            break;
                        }
                    }
                    is_prev_cmp = true;
                    state.last_comparison_type = CmpType::GT;
                    break;
                }
                case InstType::CmpGTE:
                {
                    auto cmpGteInst = std::dynamic_pointer_cast<CmpGTEInst>(currInst);
                    auto& target = cmpGteInst->getTarget();
                    auto& operands = cmpGteInst->getOperands();
                    auto& operand1 = operands[0]->getTarget();
                    auto& operand2 = operands[1]->getTarget();

                    auto& targetName = target->getString();
                    if (registerMap.find(targetName) == registerMap.end())
                    {
                        auto newReg = m_cc->new_gp64(targetName.c_str());
                        registerMap[targetName] = newReg;
                    }

                    auto& targetRegister = registerMap[targetName];
                    auto operandInstType = operand1->getInstType();
                    switch (operandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand1);
                            m_cc->mov(targetRegister, casted->getVal());
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand1);
                            m_cc->mov(targetRegister, casted->getVal());
                            break;
                        }
                        default:
                        {
                            m_cc->mov(
                                targetRegister,
                                registerMap[operand1->getTarget()->getString()]);
                            break;
                        }
                    }

                    auto otherOperandInstType = operand2->getInstType();
                    switch (otherOperandInstType)
                    {
                        case InstType::IntConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(operand2);
                            auto tempReg = m_cc->new_gp64();
                            m_cc->mov(tempReg, casted->getVal());
                            m_cc->cmp(targetRegister, tempReg);
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand2);
                            auto tempReg = m_cc->new_gp64();
                            m_cc->mov(tempReg, casted->getVal());
                            m_cc->cmp(targetRegister, tempReg);
                            break;
                        }
                        default:
                        {
                            m_cc->cmp(
                                targetRegister,
                                registerMap[operand2->getTarget()->getString()]);
                            break;
                        }
                    }
                    is_prev_cmp = true;
                    state.last_comparison_type = CmpType::GTE;
                    break;
                }
                case InstType::Jump:
                {
                    auto jumpInst = std::dynamic_pointer_cast<JumpInst>(currInst);
                    auto& targetLabel = jumpInst->getJumpTarget();
                    auto& targetLabelName = targetLabel->getName();
                    if (labelMap.find(targetLabelName) == labelMap.end())
                    {
                        labelMap[targetLabelName] =
                            m_cc->new_named_label(targetLabelName.c_str());
                    }
                    auto& label = labelMap[targetLabelName];
                    m_cc->jmp(label);
                    is_prev_cmp = false;
                    break;
                }
                case InstType::BRT:
                {
                    auto brtInst = std::dynamic_pointer_cast<BRTInst>(currInst);
                    auto& failLabel = brtInst->getTargetFailed();
                    auto& failLabelName = failLabel->getName();
                    if (labelMap.find(failLabelName) == labelMap.end())
                    {
                        labelMap[failLabelName] =
                            m_cc->new_named_label(failLabelName.c_str());
                    }
                    auto& label = labelMap[failLabelName];
                    switch (state.last_comparison_type)
                    {
                        case CmpType::EQ:
                            m_cc->jne(label);
                            break;
                        case CmpType::NE:
                            m_cc->je(label);
                            break;
                        case CmpType::LT:
                            m_cc->jge(label);
                            break;
                        case CmpType::LTE:
                            m_cc->jg(label);
                            break;
                        case CmpType::GT:
                            m_cc->jle(label);
                            break;
                        case CmpType::GTE:
                            m_cc->jl(label);
                            break;

                    }
                    is_prev_cmp = false;
                    break;
                }
                case InstType::BRF:
                {
                    auto brtInst = std::dynamic_pointer_cast<BRFInst>(currInst);
                    auto& failLabel = brtInst->getTargetFailed();
                    auto& failLabelName = failLabel->getName();
                    if (labelMap.find(failLabelName) == labelMap.end())
                    {
                        labelMap[failLabelName] =
                            m_cc->new_named_label(failLabelName.c_str());
                    }
                    auto& label = labelMap[failLabelName];
                    switch (state.last_comparison_type)
                    {
                        case CmpType::EQ:
                            m_cc->je(label);
                            break;
                        case CmpType::NE:
                            m_cc->jne(label);
                            break;
                        case CmpType::LT:
                            m_cc->jl(label);
                            break;
                        case CmpType::LTE:
                            m_cc->jle(label);
                            break;
                        case CmpType::GT:
                            m_cc->jg(label);
                            break;
                        case CmpType::GTE:
                            m_cc->jge(label);
                            break;
                    }
                    is_prev_cmp = false;
                    break;
                }

                case InstType::Put:
                {
                    auto putInst = std::dynamic_pointer_cast<PutInst>(currInst);
                    auto& operands = putInst->getOperands();
                    for (int i = 0; i < operands.size(); ++i)
                    {
                        auto& operand = operands[i]->getTarget();
                        auto operandType = operand->getInstType();
                        switch (operandType)
                        {
                            case InstType::IntConst:
                            {
                                auto intConstInst = std::dynamic_pointer_cast<IntConstInst>(operand);
                                auto val = intConstInst->getVal();
                                syscallPrintInt(val);
                                break;
                            }
                            case InstType::BoolConst:
                            {
                                auto boolConstInst = std::dynamic_pointer_cast<BoolConstInst>(operand);
                                auto val = boolConstInst->getVal();
                                if (val != 0 && val != 1)
                                {
                                    throw std::runtime_error("Boolean value must be 0 or 1!");
                                }
                                
                                if (val)
                                {
                                    syscallPrintString(std::string("true"));
                                }
                                else
                                {
                                    syscallPrintString(std::string("false"));
                                }
                                break;
                            }
                            case InstType::StrConst:
                            {
                                auto strConstInst =
                                    std::dynamic_pointer_cast<StrConstInst>(
                                        operand);
                                auto& val = strConstInst->getString();
                                if (val == "\'\\n\'")
                                {
                                    syscallPutChar('\n');
                                    break;
                                }
                                for (int i = 0; i < val.length(); ++i)
                                {
                                    if(val[i] == '\"' || val[i] == '\'') continue;
                                    syscallPutChar(val[i]);
                                }
                                break;
                            }
                            default:
                            {
                                // todo: check data type, and print true/false or integer based on data type
                                // assume integer
                                auto identifierInst =
                                    std::dynamic_pointer_cast<IdentInst>(
                                        operand);
                                std::string result;
                                std::string st = identifierInst->getString();
                                auto dot_pos = st.find(".");
                                if (dot_pos != std::string::npos)
                                {
                                    result = st.substr(0, dot_pos);
                                }
                                else
                                {
                                    result = st;
                                }

                                auto it = registerMap.find(result);
                                if (it == registerMap.end())
                                {
                                    exit(1);
                                }

                                asmjit::x86::Gp arg1_reg = getFirstArgumentRegister();
                                asmjit::x86::Gp arg2_reg = getSecondArgumentRegister();

                                auto& newReg = m_cc->new_gp64();
                                m_cc->mov(newReg, registerMap[result]);
    
                                // spill for caller saved register
                                m_cc->sub(asmjit::x86::rsp, 16);
                                m_cc->mov(
                                    asmjit::x86::word_ptr(asmjit::x86::rsp),
                                    arg1_reg);
                                m_cc->mov(
                                    asmjit::x86::word_ptr(asmjit::x86::rsp, 8),
                                    arg2_reg);

                                //// Allocate 16 bytes on the stack for the format string and the integer.
                                m_cc->sub(asmjit::x86::rsp, 16);

                                //// Write "%d\0" as a string to [rsp]. "%d" is 0x6425, null terminator is 0.
                                m_cc->mov(asmjit::x86::word_ptr(asmjit::x86::rsp), 0x6425);  // "%d"
                                m_cc->mov(asmjit::x86::byte_ptr(asmjit::x86::rsp, 2), 0);    // null terminator

                                m_cc->lea(arg1_reg, asmjit::x86::ptr(
                                                        asmjit::x86::rsp));
                                m_cc->mov(arg2_reg, newReg);

                                m_cc->sub(asmjit::x86::rsp, 32);
                                m_cc->call(printf);
                                m_cc->add(asmjit::x86::rsp, 32);

                                //// Restore stack
                                m_cc->add(asmjit::x86::rsp, 16);

                                // restore spilled register
                                m_cc->mov(arg1_reg,
                                    asmjit::x86::word_ptr(asmjit::x86::rsp)
                                );
                                m_cc->mov(arg2_reg,
                                    asmjit::x86::word_ptr(asmjit::x86::rsp, 8)
                                );
                                m_cc->add(asmjit::x86::rsp, 16);
                                break;
                            }
                        }
                    }
                    break;
                }
                case InstType::Get:
                {
                    auto getInst = std::dynamic_pointer_cast<GetInst>(currInst);
                    auto target = getInst->getTarget();

                    auto& targetName = target->getString();
                    if (registerMap.find(targetName) == registerMap.end())
                    {
                        auto newReg = m_cc->new_gp64(targetName.c_str());
                        registerMap[targetName] = newReg;
                    }
                    auto& targetRegister = registerMap[targetName];
                    syscallScanInt(targetRegister);
                    m_cc->mov(targetRegister, asmjit::x86::rax);
                    break;
                }
                case InstType::Push:
                {
                    auto pushInst = std::dynamic_pointer_cast<PushInst>(currInst);
                    auto operand = pushInst->getOperands()[0]->getTarget();
                    auto operandType = operand->getInstType();
                    switch (operandType)
                    {
                        case InstType::IntConst:
                        {
                            auto intConstInst = std::dynamic_pointer_cast<IntConstInst>(operand);
                            auto val = intConstInst->getVal();
                            m_cc->push(val);
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto boolConstInst = std::dynamic_pointer_cast<BoolConstInst>(operand);
                            auto val = boolConstInst->getVal();
                            m_cc->push(val);
                            break;
                        }
                        case InstType::StrConst:
                        {
                            throw std::runtime_error("Cannot push StrConst!");
                            break;
                        }
                        default:
                        {
                          break;
                        }
                    }
                    break;
                }
                
                case InstType::Pop:
                {
                    break;
                }

                case InstType::Return:
                {
                    break;
                }

                case InstType::Call:
                {
                    auto callInst = std::dynamic_pointer_cast<CallInst>(currInst);
                    std::string& funcName = callInst->getCalleeStr();
                    asmjit::InvokeNode* invoke_node{nullptr};
                    auto it = m_funcMap.find(funcName);
                    if (it == m_funcMap.end())
                    {
                        throw std::runtime_error("function name " + funcName + " not found!");
                    }

                    asmjit::FuncNode* funcNode = m_funcMap[funcName].first;
                    asmjit::FuncSignature funcSig = m_funcMap[funcName].second;

                    if (funcSig == asmjit::FuncSignature::build<void>())
                    {
                        m_cc->invoke(asmjit::Out(invoke_node), funcNode->label(), funcSig);
                    }
                    else if (funcSig == asmjit::FuncSignature::build<void, int>())
                    {
                        auto& operand = callInst->getOperands()[0]->getTarget();
                        if (operand->getInstType() == InstType::IntConst)
                        {
                            auto val = std::dynamic_pointer_cast<IntConstInst>(operand)->getVal();
                            auto newReg = m_cc->new_gp64();
                            m_cc->mov(newReg, val);
                            m_cc->invoke(asmjit::Out(invoke_node),
                                         funcNode->label(), funcSig);
                            invoke_node->set_arg(0, newReg);
                        }
                        else if (operand->getInstType() == InstType::Ident)
                        {
                            auto& identInst = std::dynamic_pointer_cast<IdentInst>(operand);
                            std::string& identName = identInst->getString();
                            auto it = registerMap.find(identName);
                            if (it == registerMap.end())
                            {
                                throw std::runtime_error("unknown identifier");
                            }
                            m_cc->invoke(asmjit::Out(invoke_node),
                                         funcNode->label(), funcSig);
                            invoke_node->set_arg(0, registerMap[identName]);
                        }
                        else
                        {
                            throw std::runtime_error("argument type other than int and identifier is not implemented yet");
                        }
                    }
                    break;
                }

                case InstType::Halt:
                {
                    m_cc->ret();
                }

                case InstType::FuncSignature:
                {
                    std::cout << "func signature!\n";
                    break;
                }

                case InstType::LowerFunc:
                {
                    auto lowerInst = std::dynamic_pointer_cast<LowerFunc>(currInst);
                    auto& parameters = lowerInst->getParameters();

                    for (const auto& param : parameters)
                    {
                        auto paramType = param->getIdentType();
                        if (paramType == IdentType::VARIABLE)
                        {
                            auto identAST = std::dynamic_pointer_cast<VariableAST>(param);
                            if (identAST == nullptr)
                            {
                                throw std::runtime_error("error, should be variableAST");
                            }
                            else
                            {
                                // Assign register to parameter definition.
                                std::string& name = identAST->getName();
                                auto& newReg = m_cc->new_gp64();
                                registerMap[name] = newReg;
                                m_cc->mov(newReg, getFirstArgumentRegister());
                            }
                        }
                        else
                        {
                            throw std::runtime_error("lowering array is not implemented yet\n");
                        }
                    }
                    std::cout << "lower func!\n";
                    break;
                }

                default:
                    break;
            }
        }

        for (const auto& successor : current_bb->getSuccessors())
        {
            if (visited.find(successor) == visited.end())
            {
                auto newSuccessor =
                    std::make_shared<BasicBlock>(successor->getName());
                visited.insert(successor);
                worklist.push(successor);
            }
        }
    }
    m_cc->end_func();
}

void CodeGen::executeJIT()
{
    m_cc->finalize();  // Translate and assemble the whole 'm_cc' content.    

    std::cout << "==================================" << std::endl;
    Func fn;
    asmjit::Error err = m_rt.add(&fn, &m_code);
    std::cout << m_logger.content().data() << std::endl;

    if (err != asmjit::Error::kOk)
    {gen
        return;  // Handle a possible error returned by AsmJit.
    }

    fn();
    std::cout << "JIT executed!\n";

    m_rt.release(fn);
}
*/
