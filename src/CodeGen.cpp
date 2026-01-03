#include "CodeGen.hpp"
#include "BasicBlock.hpp"
#include "MachineIR.hpp"
#include "InstIR.hpp"

#include <functional>
#include <iostream>
#include <vector>
#include <memory>
#include <algorithm>
#include <set>
#include <map>

typedef int (*Func)(void);

CodeGen::CodeGen(SSA ssa) : m_ssa{ssa}
{
}

void CodeGen::setSSA(SSA& ssa) { m_ssa = ssa; }

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
                else if (sourceType == InstType::BoolConst)
                {
                    auto boolConstInst =
                        std::dynamic_pointer_cast<BoolConstInst>(source);
                    int boolVal = boolConstInst->getVal() ? 1 : 0;
                    std::shared_ptr<MachineIR> mirSource{
                        new ConstMIR{boolVal}};

                    // mov QWORD PTR [target], constant
                    auto movMIR = std::make_shared<MovMIR>(
                        std::vector<std::shared_ptr<MachineIR>>{mirTarget,
                                                                mirSource});
                    bbMIR->addInstruction(movMIR);
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
            else if (instType == InstType::Not)
            {
                auto notInst = std::dynamic_pointer_cast<NotInst>(inst[j]);

                auto targetStr =
                    notInst->getTarget()->getString();  // Temporary variable
                assignVRegToOffsetIfDoesNotExist(targetStr);

                auto operand = notInst->getOperand()->getTarget();
                auto opMemMIR =
                    memoryLocationForVReg(operand->getString());  // rax
                
                // mov rax, QWORD PTR [operand]
                auto movMIR1 = std::make_shared<MovMIR>(
                    std::vector<std::shared_ptr<MachineIR>>{rax, opMemMIR});
                bbMIR->addInstruction(movMIR1);

                // xor rax, 1
                auto notMIR = std::make_shared<NotMIR>(rax);
                bbMIR->addInstruction(notMIR);

                // mov QWORD PTR [target], rax
                auto targetMemMIR = memoryLocationForVReg(targetStr);
                auto movMIR2 = std::make_shared<MovMIR>(
                    std::vector<std::shared_ptr<MachineIR>>{targetMemMIR, rax});
                bbMIR->addInstruction(movMIR2);
            }
            else if (instType == InstType::Or)
            {
                auto orInst = std::dynamic_pointer_cast<OrInst>(inst[j]);
                auto targetStr = orInst->getTarget()->getString(); // Temporary variable
                auto& operands = orInst->getOperands();
                auto& operand1 = operands[0]->getTarget();
                auto& operand2 = operands[1]->getTarget();
                assignVRegToOffsetIfDoesNotExist(targetStr);
                auto op1MemMIR =
                    memoryLocationForVReg(operand1->getString());  // rax
                auto op2MemMIR =
                    memoryLocationForVReg(operand2->getString());  // rbx
                
                // mov rax, QWORD PTR [operand1]
                auto movMIR1 = std::make_shared<MovMIR>(
                    std::vector<std::shared_ptr<MachineIR>>{rax, op1MemMIR});
                bbMIR->addInstruction(movMIR1);

                // mov rbx, QWORD PTR [operand2]
                auto movMIR2 = std::make_shared<MovMIR>(
                    std::vector<std::shared_ptr<MachineIR>>{rbx, op2MemMIR});
                bbMIR->addInstruction(movMIR2);

                // or rax, rbx
                auto orMIR = std::make_shared<OrMIR>(
                    std::vector<std::shared_ptr<MachineIR>>{rax, rbx});
                bbMIR->addInstruction(orMIR);

                // mov QWORD PTR [target], rax
                auto targetMemMIR = memoryLocationForVReg(targetStr);
                auto movMIR3 = std::make_shared<MovMIR>(
                    std::vector<std::shared_ptr<MachineIR>>{targetMemMIR, rax});
                bbMIR->addInstruction(movMIR3);
            }
            else if (instType == InstType::And)
            {
                auto andInst = std::dynamic_pointer_cast<AndInst>(inst[j]);
                auto targetStr = andInst->getTarget()->getString(); // Temporary variable
                auto& operands = andInst->getOperands();
                auto& operand1 = operands[0]->getTarget();
                auto& operand2 = operands[1]->getTarget();
                assignVRegToOffsetIfDoesNotExist(targetStr);
                auto op1MemMIR =
                    memoryLocationForVReg(operand1->getString());  // rax
                auto op2MemMIR =
                    memoryLocationForVReg(operand2->getString());  // rbx
                
                // mov rax, QWORD PTR [operand1]
                auto movMIR1 = std::make_shared<MovMIR>(
                    std::vector<std::shared_ptr<MachineIR>>{rax, op1MemMIR});
                bbMIR->addInstruction(movMIR1);

                // mov rbx, QWORD PTR [operand2]
                auto movMIR2 = std::make_shared<MovMIR>(
                    std::vector<std::shared_ptr<MachineIR>>{rbx, op2MemMIR});
                bbMIR->addInstruction(movMIR2);

                // and rax, rbx
                auto andMIR = std::make_shared<AndMIR>(
                    std::vector<std::shared_ptr<MachineIR>>{rax, rbx});
                bbMIR->addInstruction(andMIR);

                // mov QWORD PTR [target], rax
                auto targetMemMIR = memoryLocationForVReg(targetStr);
                auto movMIR3 = std::make_shared<MovMIR>(
                    std::vector<std::shared_ptr<MachineIR>>{targetMemMIR, rax});
                bbMIR->addInstruction(movMIR3);
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
