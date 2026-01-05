#include "SSA.hpp"
#include "CodeGen.hpp"
#include "BasicBlock.hpp"
#include "MachineIR.hpp"
#include "InstIR.hpp"

#include <map>
#include <set>
#include <memory>
#include <string>
#include <vector>
#include <utility>
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <functional>

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
    std::shared_ptr<Register> rax{
        new Register{2, "rax", "eax", "ax", "ah", "al"}};
    std::shared_ptr<Register> rbx{
        new Register{3, "rbx", "ebx", "bx", "bh", "bl"}};
    std::shared_ptr<Register> rcx{
        new Register{4, "rcx", "ecx", "cx", "ch", "cl"}};
    std::shared_ptr<Register> rdx{
        new Register{5, "rdx", "edx", "dx", "dh", "dl"}};
    std::shared_ptr<Register> rip{new Register{6, "rip"}};

    std::map<std::string, unsigned int> vRegToOffset;
    std::map<std::string, unsigned int> arrVRegToSize;
    std::vector<std::string> strLiterals;
    unsigned int strConstCtr = 0;

    auto assignVRegToOffsetIfDoesNotExist =
        [&](std::string targetStr)
    {
        if (vRegToOffset.find(targetStr) == vRegToOffset.end())
        {
            auto offset = vRegToOffset.size() * 8;
            for (const auto& arrPair : arrVRegToSize)
            {
                offset += static_cast<unsigned long long>(arrPair.second) * 8;
            }

            vRegToOffset[targetStr] = offset + 8;
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
        if (vRegToOffset.find(vReg) == vRegToOffset.end())
        {
            throw std::runtime_error("CodeGen Error: Variable '" + vReg + "' not found in stack map.");
        }
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
            if (instType == InstType::FuncCall)
            {
                auto currInst = std::dynamic_pointer_cast<FuncCallInst>(inst[j]);
                auto target = currInst->getTarget();
                auto targetStr = target->getTarget()->getString();
                assignVRegToOffsetIfDoesNotExist(targetStr);
                std::cout << "function call codegen is not being implemented yet!\n";
            }
            else if (instType == InstType::Assign)
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
            else if (instType == InstType::Add || instType == InstType::Sub || instType == InstType::Mul)
            {
                auto& currInst = inst[j];
                auto targetStr = currInst->getTarget()->getString(); // Temporary variable

                auto& operands = currInst->getOperands();
                auto& operand1 = operands[0]->getTarget();
                auto& operand2 = operands[1]->getTarget();

                assignVRegToOffsetIfDoesNotExist(targetStr);

                if (operand1->getInstType() == InstType::IntConst)
                {
                    auto intConstInst =
                        std::dynamic_pointer_cast<IntConstInst>(operand1);
                    auto constMIR =
                        std::make_shared<ConstMIR>(intConstInst->getVal());

                    // mov rax, constant
                    auto movMIR = std::make_shared<MovMIR>(
                        std::vector<std::shared_ptr<MachineIR>>{rax, constMIR});
                    bbMIR->addInstruction(movMIR);
                }
                else
                {
                    auto op1MemMIR = memoryLocationForVReg(
                        operand1->getTarget()->getString());

                    // mov rax, QWORD PTR [op1]
                    auto movMIR = std::make_shared<MovMIR>(
                        std::vector<std::shared_ptr<MachineIR>>{rax,
                                                                op1MemMIR});
                    bbMIR->addInstruction(movMIR);
                }

                if (operand2->getInstType() == InstType::IntConst)
                {
                    auto intConstInst =
                        std::dynamic_pointer_cast<IntConstInst>(operand2);
                    auto constMIR =
                        std::make_shared<ConstMIR>(intConstInst->getVal());

                    // mov rbx, constant
                    auto movMIR = std::make_shared<MovMIR>(
                        std::vector<std::shared_ptr<MachineIR>>{rbx, constMIR});
                    bbMIR->addInstruction(movMIR);
                }
                else
                {
                    auto op2MemMIR = memoryLocationForVReg(
                        operand2->getTarget()->getString());

                    // mov rbx, QWORD PTR [op2]
                    auto movMIR = std::make_shared<MovMIR>(
                        std::vector<std::shared_ptr<MachineIR>>{rbx,
                                                                op2MemMIR});
                    bbMIR->addInstruction(movMIR);
                }

                if (instType == InstType::Add)
                {
                    // add rax, rbx
                    auto addMIR = std::make_shared<AddMIR>(
                        std::vector<std::shared_ptr<MachineIR>>{rax, rbx});
                    bbMIR->addInstruction(addMIR);
                }
                else if (instType == InstType::Sub)
                {
                    // sub rax, rbx
                    auto subMIR = std::make_shared<SubMIR>(
                        std::vector<std::shared_ptr<MachineIR>>{rax, rbx});
                    bbMIR->addInstruction(subMIR);
                }
                else
                {
                    // imul rax, rbx
                    auto mulMIR = std::make_shared<MulMIR>(
                        std::vector<std::shared_ptr<MachineIR>>{rax, rbx});
                    bbMIR->addInstruction(mulMIR);
                }

                // mov QOWRD PTR [target], rax
                auto targetMemMIR = memoryLocationForVReg(targetStr);
                auto movMIR = std::make_shared<MovMIR>(
                    std::vector<std::shared_ptr<MachineIR>>{targetMemMIR, rax});
                bbMIR->addInstruction(movMIR);
            }
            else if (instType == InstType::Div)
            {
                auto divInst = std::dynamic_pointer_cast<DivInst>(inst[j]);
                auto targetStr = divInst->getTarget()->getString(); // Temporary variable

                auto& operands = divInst->getOperands();
                auto& operand1 = operands[0]->getTarget();
                auto& operand2 = operands[1]->getTarget();

                assignVRegToOffsetIfDoesNotExist(targetStr);

                if (operand1->getInstType() == InstType::IntConst)
                {
                    auto intConstInst =
                        std::dynamic_pointer_cast<IntConstInst>(operand1);
                    auto constMIR =
                        std::make_shared<ConstMIR>(intConstInst->getVal());

                    // mov rax, constant
                    auto movMIR = std::make_shared<MovMIR>(
                        std::vector<std::shared_ptr<MachineIR>>{rax, constMIR});
                    bbMIR->addInstruction(movMIR);
                }
                else
                {
                    auto op1MemMIR = memoryLocationForVReg(
                        operand1->getTarget()->getString());

                    // mov rax, QWORD PTR [op1]
                    auto movMIR = std::make_shared<MovMIR>(
                        std::vector<std::shared_ptr<MachineIR>>{rax,
                                                                op1MemMIR});
                    bbMIR->addInstruction(movMIR);
                }

                // cqo
                auto cqoMIR = std::make_shared<CqoMIR>();
                bbMIR->addInstruction(cqoMIR);

                if (operand2->getInstType() == InstType::IntConst)
                {
                    throw std::runtime_error(
                        "Division by constant not supported yet.");
                }
                else
                {
                    auto op2MemMIR =
                        memoryLocationForVReg(operand2->getString());  // rbx

                    auto divMIR = std::make_shared<DivMIR>(op2MemMIR);
                    bbMIR->addInstruction(divMIR);
                }

                // mov QWORD PTR [target], rax
                auto targetMemMIR = memoryLocationForVReg(targetStr);
                auto movMIR = std::make_shared<MovMIR>(
                    std::vector<std::shared_ptr<MachineIR>>{targetMemMIR,
                                                            rax});
                bbMIR->addInstruction(movMIR);
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
            else if (instType == InstType::Or || instType == InstType::And)
            {
                auto& currInst = inst[j];
                auto& targetStr = currInst->getTarget()->getString(); // Temporary variable
                auto& operands = currInst->getOperands();
                auto& operand1 = operands[0]->getTarget();
                auto& operand2 = operands[1]->getTarget();
                assignVRegToOffsetIfDoesNotExist(targetStr);

                if (operand1->getInstType() == InstType::BoolConst)
                {
                    auto boolConstInst =
                        std::dynamic_pointer_cast<BoolConstInst>(operand1);
                    int boolVal = boolConstInst->getVal() ? 1 : 0;
                    auto constMIR = std::make_shared<ConstMIR>(boolVal);

                    // mov rax, constant
                    auto movMIR = std::make_shared<MovMIR>(
                        std::vector<std::shared_ptr<MachineIR>>{rax, constMIR});
                    bbMIR->addInstruction(movMIR);
                }
                else
                {
                    // mov rax, QWORD PTR [operand1]
                    auto op1MemMIR =
                        memoryLocationForVReg(operand1->getString());
                    auto movMIR1 = std::make_shared<MovMIR>(
                        std::vector<std::shared_ptr<MachineIR>>{rax, op1MemMIR});
                    bbMIR->addInstruction(movMIR1);
                }

                if (operand2->getInstType() == InstType::BoolConst)
                {
                    auto boolConstInst =
                        std::dynamic_pointer_cast<BoolConstInst>(operand2);
                    int boolVal = boolConstInst->getVal() ? 1 : 0;
                    auto constMIR = std::make_shared<ConstMIR>(boolVal);

                    // mov rbx, constant
                    auto movMIR = std::make_shared<MovMIR>(
                        std::vector<std::shared_ptr<MachineIR>>{rbx, constMIR});
                    bbMIR->addInstruction(movMIR);
                }
                else
                {
                    // mov rbx, QWORD PTR [operand2]
                    auto op2MemMIR =
                        memoryLocationForVReg(operand2->getString());
                    auto movMIR2 = std::make_shared<MovMIR>(
                        std::vector<std::shared_ptr<MachineIR>>{rbx, op2MemMIR});
                    bbMIR->addInstruction(movMIR2);
                }

                if (instType == InstType::Or)
                {
                    // or rax, rbx
                    auto orMIR = std::make_shared<OrMIR>(
                        std::vector<std::shared_ptr<MachineIR>>{rax, rbx});
                    bbMIR->addInstruction(orMIR);
                }
                else
                {
                    // and rax, rbx
                    auto andMIR = std::make_shared<AndMIR>(
                        std::vector<std::shared_ptr<MachineIR>>{rax, rbx});
                    bbMIR->addInstruction(andMIR);
                }

                // mov QWORD PTR [target], rax
                auto targetMemMIR = memoryLocationForVReg(targetStr);
                auto movMIR3 = std::make_shared<MovMIR>(
                    std::vector<std::shared_ptr<MachineIR>>{targetMemMIR, rax});
                bbMIR->addInstruction(movMIR3);
            }
            else if (instType == InstType::CmpEq  || instType == InstType::CmpNE  ||
                     instType == InstType::CmpLT  || instType == InstType::CmpLTE ||
                     instType == InstType::CmpGT  || instType == InstType::CmpGTE)
            {
                auto& currInst = inst[j];
                auto target = currInst->getTarget();
                auto targetStr = target->getString();
                auto& operands = currInst->getOperands();
                auto& operand1 = operands[0]->getTarget();
                auto& operand2 = operands[1]->getTarget();

                assignVRegToOffsetIfDoesNotExist(targetStr);

                if (operand1->getInstType() == InstType::IntConst)
                {
                    auto intConst = std::dynamic_pointer_cast<IntConstInst>(operand1);
                    auto constMIR = std::make_shared<ConstMIR>(intConst->getVal());

                    // mov rax, constant
                    bbMIR->addInstruction(std::make_shared<MovMIR>(
                        std::vector<std::shared_ptr<MachineIR>>{rax, constMIR}));
                }
                else {
                    auto op1MemMIR = memoryLocationForVReg(operand1->getString());

                    // mov rax, QWORD PTR [op1]
                    bbMIR->addInstruction(std::make_shared<MovMIR>(
                        std::vector<std::shared_ptr<MachineIR>>{rax, op1MemMIR}));
                }

                if (operand2->getInstType() == InstType::IntConst) {
                    auto intConst = std::dynamic_pointer_cast<IntConstInst>(operand2);
                    auto constMIR = std::make_shared<ConstMIR>(intConst->getVal());

                    // cmp rax, constant
                    bbMIR->addInstruction(std::make_shared<CmpMIR>(
                        std::vector<std::shared_ptr<MachineIR>>{rax, constMIR}));
                } else {
                    auto op2MemMIR = memoryLocationForVReg(operand2->getString());

                    // cmp rax, QWORD PTR [op2]
                    bbMIR->addInstruction(std::make_shared<CmpMIR>(
                        std::vector<std::shared_ptr<MachineIR>>{rax, op2MemMIR}));
                }

                // setcc al
                std::shared_ptr<MachineIR> setccMIR;
                switch (instType)
                {
                    case InstType::CmpEq:  setccMIR = std::make_shared<SeteMIR>(rax); break;
                    case InstType::CmpNE:  setccMIR = std::make_shared<SetneMIR>(rax); break;
                    case InstType::CmpLT:  setccMIR = std::make_shared<SetlMIR>(rax); break;
                    case InstType::CmpLTE: setccMIR = std::make_shared<SetleMIR>(rax); break;
                    case InstType::CmpGT:  setccMIR = std::make_shared<SetgMIR>(rax); break;
                    case InstType::CmpGTE: setccMIR = std::make_shared<SetgeMIR>(rax); break;
                    default: break;
                }
                bbMIR->addInstruction(setccMIR);

                // movzx rax, al
                auto movzxMIR = std::make_shared<MovzxMIR>(rax, 64, 8, true);
                bbMIR->addInstruction(movzxMIR);

                // mov QWORD PTR [target], rax
                auto targetMemMIR = memoryLocationForVReg(targetStr);
                auto movMIR2 = std::make_shared<MovMIR>(
                    std::vector<std::shared_ptr<MachineIR>>{targetMemMIR, rax});
                bbMIR->addInstruction(movMIR2);
            }
            else if (instType == InstType::Jump)
            {
                auto jumpInst = std::dynamic_pointer_cast<JumpInst>(inst[j]);
                auto targetBB = jumpInst->getJumpTarget();
                auto jmpMIR = std::make_shared<JmpMIR>(targetBB->getName());
                bbMIR->addInstruction(jmpMIR);
            }
            else if (instType == InstType::BRT)
            {
                auto brtInst = std::dynamic_pointer_cast<BRTInst>(inst[j]);
                auto condInst = brtInst->getCond();
                auto targetSuccessBB = brtInst->getTargetSuccess();
                auto targetFailedBB = brtInst->getTargetFailed();
                auto condMemMIR = memoryLocationForVReg(condInst->getString());

                // mov rax, QWORD PTR [cond]
                auto movMIR = std::make_shared<MovMIR>(
                    std::vector<std::shared_ptr<MachineIR>>{rax, condMemMIR});
                bbMIR->addInstruction(movMIR);

                // test rax, rax
                auto testMIR = std::make_shared<TestMIR>(rax, rax);
                bbMIR->addInstruction(testMIR);

                // jnz targetSuccess
                auto jnzMIR =
                    std::make_shared<JnzMIR>(targetSuccessBB->getName());
                bbMIR->addInstruction(jnzMIR);

                // jmp targetFailed
                auto jmpMIR =
                    std::make_shared<JmpMIR>(targetFailedBB->getName());
                bbMIR->addInstruction(jmpMIR);
            }
            else if (instType == InstType::BRF)
            {
                auto brfInst = std::dynamic_pointer_cast<BRFInst>(inst[j]);
                auto condInst = brfInst->getCond()->getTarget();
                auto targetSuccessBB = brfInst->getTargetSuccess();
                auto targetFailedBB = brfInst->getTargetFailed();
                auto condMemMIR = memoryLocationForVReg(condInst->getString());

                // mov rax, QWORD PTR [cond]
                auto movMIR = std::make_shared<MovMIR>(
                    std::vector<std::shared_ptr<MachineIR>>{rax, condMemMIR});
                bbMIR->addInstruction(movMIR);

                // test rax, rax
                auto testMIR = std::make_shared<TestMIR>(rax, rax);
                bbMIR->addInstruction(testMIR);

                // jz targetSuccess
                auto jzMIR =
                    std::make_shared<JzMIR>(targetSuccessBB->getName());
                bbMIR->addInstruction(jzMIR);

                // jmp targetFailed
                auto jmpMIR =
                    std::make_shared<JmpMIR>(targetFailedBB->getName());
                bbMIR->addInstruction(jmpMIR);
            }
            else if (instType == InstType::Alloca)
            {
                auto allocaInst = std::dynamic_pointer_cast<AllocaInst>(inst[j]);
                auto targetStr = allocaInst->getTarget()->getString();
                if (arrVRegToSize.find(targetStr) == arrVRegToSize.end() &&
                    vRegToOffset.find(targetStr) == vRegToOffset.end())
                {
                    assignVRegToOffsetIfDoesNotExist(targetStr);
                    arrVRegToSize[targetStr] = allocaInst->getSize();
                }
                else
                {
                    throw std::runtime_error("Array already allocated: " + targetStr);
                }
            }
            else if (instType == InstType::ArrUpdate)
            {
                auto arrUpdateInst = std::dynamic_pointer_cast<ArrUpdateInst>(inst[j]);
                auto& source = arrUpdateInst->getTarget();
                auto& sourceName = source->getString();
                auto& index = arrUpdateInst->getIndex()->getTarget();
                auto& value = arrUpdateInst->getVal()->getTarget();

                // Array Base Address Check
                if (vRegToOffset.find(sourceName) == vRegToOffset.end())
                {
                    throw std::runtime_error("ArrUpdate: Array '" + sourceName + "' not allocated in stack.");
                }
                auto sourceOffset = vRegToOffset[sourceName];

                // Save Value to RAX
                if (value->getInstType() == InstType::IntConst)
                {
                    auto intConstInst = std::dynamic_pointer_cast<IntConstInst>(value);
                    auto constMIR = std::make_shared<ConstMIR>(intConstInst->getVal());
                    bbMIR->addInstruction(std::make_shared<MovMIR>(std::vector<std::shared_ptr<MachineIR>>{rax, constMIR}));
                }
                else if (value->getInstType() == InstType::Ident)
                {
                    std::string varName = value->getTarget()->getString();
                    bbMIR->addInstruction(std::make_shared<MovMIR>(std::vector<std::shared_ptr<MachineIR>>{rax, memoryLocationForVReg(varName)}));
                }

                // Compute Offset into RBX
                if (index->getInstType() == InstType::IntConst)
                {
                    auto intConstInst = std::dynamic_pointer_cast<IntConstInst>(index);
                    int indexVal = intConstInst->getVal();
                    unsigned int indexOffset = indexVal * 8;
                    auto offsetMIR = std::make_shared<ConstMIR>(-static_cast<int>(sourceOffset + indexOffset));
                    auto sourceMemMIR = std::make_shared<MemoryMIR>(rbp, offsetMIR);
                    bbMIR->addInstruction(std::make_shared<MovMIR>(std::vector<std::shared_ptr<MachineIR>>{sourceMemMIR, rax}));
                }
                else if (index->getInstType() == InstType::Ident)
                {
                    std::string indexName = index->getTarget()->getString();

                    // Safe Lookup for Index (e.g., Variable "A")
                    auto indexMemMIR = memoryLocationForVReg(indexName);

                    // mov rbx, QWORD PTR [index]
                    bbMIR->addInstruction(std::make_shared<MovMIR>(std::vector<std::shared_ptr<MachineIR>>{rbx, indexMemMIR}));

                    // imul rbx, 8
                    bbMIR->addInstruction(std::make_shared<MulMIR>(std::vector<std::shared_ptr<MachineIR>>{rbx, std::make_shared<ConstMIR>(8)}));

                    // lea rcx, [rbp - sourceOffset] (Base)
                    auto sourceOffsetMIR = std::make_shared<ConstMIR>(-static_cast<int>(sourceOffset));
                    bbMIR->addInstruction(std::make_shared<LeaMIR>(std::vector<std::shared_ptr<MachineIR>>{rcx, std::make_shared<MemoryMIR>(rbp, sourceOffsetMIR)}));

                    // sub rcx, rbx
                    bbMIR->addInstruction(std::make_shared<SubMIR>(std::vector<std::shared_ptr<MachineIR>>{rcx, rbx}));

                    // mov [rcx], rax
                    bbMIR->addInstruction(std::make_shared<MovMIR>(std::vector<std::shared_ptr<MachineIR>>{std::make_shared<MemoryMIR>(rcx, std::make_shared<ConstMIR>(0)), rax}));
                }
            }
            else if (instType == InstType::ArrAccess)
            {
                auto arrAccessInst = std::dynamic_pointer_cast<ArrAccessInst>(inst[j]);
                auto& target = arrAccessInst->getTarget();
                auto& source = arrAccessInst->getSource()->getTarget();
                auto& index = arrAccessInst->getIndex()->getTarget();

                if (vRegToOffset.find(source->getString()) == vRegToOffset.end())
                {
                    throw std::runtime_error("ArrAccess: Array '" + source->getString() + "' not allocated.");
                }
                auto sourceOffset = vRegToOffset[source->getString()];

                assignVRegToOffsetIfDoesNotExist(target->getString());

                if (index->getInstType() == InstType::IntConst)
                {
                    auto intConstInst = std::dynamic_pointer_cast<IntConstInst>(index);
                    int indexVal = intConstInst->getVal();
                    unsigned int indexOffset = indexVal * 8;
                    auto offsetMIR = std::make_shared<ConstMIR>(-static_cast<int>(sourceOffset + indexOffset));
                    auto sourceMemMIR = std::make_shared<MemoryMIR>(rbp, offsetMIR);
                    bbMIR->addInstruction(std::make_shared<MovMIR>(std::vector<std::shared_ptr<MachineIR>>{rax, sourceMemMIR}));
                } 
                else if (index->getInstType() == InstType::Ident) 
                {
                    std::string indexName = index->getTarget()->getString();

                    // Safe lookup
                    auto indexMemMIR = memoryLocationForVReg(indexName);

                    // mov rbx, QWORD PTR [index]
                    bbMIR->addInstruction(std::make_shared<MovMIR>(std::vector<std::shared_ptr<MachineIR>>{rbx, indexMemMIR}));

                    // imul rbx, 8
                    bbMIR->addInstruction(std::make_shared<MulMIR>(std::vector<std::shared_ptr<MachineIR>>{rbx, std::make_shared<ConstMIR>(8)}));

                    // lea rcx, QWORD PTR [rbp - sourceOffset]
                    auto sourceOffsetMIR = std::make_shared<ConstMIR>(-static_cast<int>(sourceOffset));
                    bbMIR->addInstruction(std::make_shared<LeaMIR>(std::vector<std::shared_ptr<MachineIR>>{rcx, std::make_shared<MemoryMIR>(rbp, sourceOffsetMIR)}));

                    // sub rcx, rbx
                    bbMIR->addInstruction(std::make_shared<SubMIR>(std::vector<std::shared_ptr<MachineIR>>{rcx, rbx}));

                    // mov rax, QWORD PTR [rcx]
                    bbMIR->addInstruction(std::make_shared<MovMIR>(std::vector<std::shared_ptr<MachineIR>>{rax, std::make_shared<MemoryMIR>(rcx, std::make_shared<ConstMIR>(0))}));
                }

                auto targetMemMIR = memoryLocationForVReg(target->getString());
                bbMIR->addInstruction(std::make_shared<MovMIR>(std::vector<std::shared_ptr<MachineIR>>{targetMemMIR, rax}));
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

    // Shadow space 32 byte and 8 byte for each variable
    unsigned int offset = 32 + vRegToOffset.size() * 8;

    // Offset for arrays
    for (const auto& arrPair : arrVRegToSize)
    {
        offset += arrPair.second * 8;
    }

    unsigned int aligned_offset = (offset + 15) & ~15; // 16-byte alignment
    std::cout << "    push rbp\n    mov rbp, rsp\n";
    std::cout << "    sub rsp, " << aligned_offset << std::endl;

    for (const auto& mirBlock : m_mirBlocks)
    {
        std::cout << mirBlock->getName() << ": \n";
        mirBlock->printInstructions();
    }

    std::cout << "    add rsp, " << aligned_offset << std::endl;
    std::cout << "    mov rsp, rbp\n    pop rbp\n    ret\n";

    std::cout << "\nnewline_str: .string \"\\n\"\n";
    std::cout << "\n";
}
