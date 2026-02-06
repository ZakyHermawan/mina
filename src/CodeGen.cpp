#include "SSA.hpp"
#include "CodeGen.hpp"
#include "BasicBlock.hpp"
#include "MachineIR.hpp"
#include "InstIR.hpp"
#include "RegisterAllocator.hpp"

#include <map>
#include <set>
#include <memory>
#include <string>
#include <vector>
#include <cassert>
#include <utility>
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <functional>

namespace mina
{

CodeGen::CodeGen(SSA ssa) : m_ssa{ssa}
{
}

void CodeGen::setSSA(SSA& ssa) { m_ssa = ssa; }

void CodeGen::linearizeCFG()
{
    // Implement Reverse Post-Order Traversal to linearize the CFG
    m_linearizedBlocks.clear();
    std::set<std::shared_ptr<BasicBlock>> visited;
    std::function<void(std::shared_ptr<BasicBlock>)> dfs =
        [&](std::shared_ptr<BasicBlock> bb)
    {
        visited.insert(bb);
        const auto& successors = bb->getSuccessors();
        // Traverse successors in reverse order so the first element that is being inserted 
        // into the successors vector is in front of successors that is added later,
        // since we will reverse the linearizedBlocks at the end.
        for (int i = successors.size() - 1; i >= 0; --i)
        {
            auto& succ = successors[i];
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
    auto rax = std::make_shared<Register>(0, "rax", "eax", "ax", "ah", "al");
    auto rbx = std::make_shared<Register>(1, "rbx", "ebx", "bx", "bh", "bl");
    auto rcx = std::make_shared<Register>(2, "rcx", "ecx", "cx", "ch", "cl");
    auto rdx = std::make_shared<Register>(3, "rdx", "edx", "dx", "dh", "dl");
    auto rdi = std::make_shared<Register>(4, "rdi", "edi", "di", "dil", "");
    auto rsi = std::make_shared<Register>(5, "rsi", "esi", "si", "sil", "");
    auto r8 = std::make_shared<Register>(6, "r8", "r8d", "r8w", "r8b", "");
    auto r9 = std::make_shared<Register>(7, "r9", "r9d", "r9w", "r9b", "");
    auto r12 = std::make_shared<Register>(8, "r12", "r12d", "r12w", "r12b", "");
    auto r13 = std::make_shared<Register>(9, "r13", "r13d", "r13w", "r13b", "");
    auto r14 = std::make_shared<Register>(10, "r13", "r13d", "r13w", "r13b", "");
    auto rbp = std::make_shared<Register>(11, "rbp", "ebp", "bp", "", "");
    auto rsp = std::make_shared<Register>(12, "rsp", "esp", "sp", "", "");
    auto rip = std::make_shared<Register>(13, "rip", "eip", "ip", "", "");
    unsigned int nextRegId = 14;
    unsigned int tmpRegId = 0;

    std::map<std::string, size_t> vRegToOffset;
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

    // Map variable name strings to shared Register objects
    std::map<std::string, std::shared_ptr<Register>> m_vregMap;
    auto getOrCreateVReg = [&](const std::string& name) -> std::shared_ptr<Register>
    {
        auto it = m_vregMap.find(name);
        if (it != m_vregMap.end())
        {
            return it->second;
        }

        // If not, create a new one and store it
        auto newReg = std::make_shared<Register>(nextRegId++, name);
        m_vregMap[name] = newReg;
        return newReg;
    };

    auto createTempVReg = [&]() -> std::shared_ptr<Register>
    {
        std::string name = "v_tmp" + std::to_string(tmpRegId++);
        return getOrCreateVReg(name);
    };

    auto legalizeMov = [&](std::shared_ptr<BasicBlockMIR> bb,
                           std::shared_ptr<MachineIR> dst,
                           std::shared_ptr<MachineIR> src)
    {
        auto dstReg = std::dynamic_pointer_cast<Register>(dst);
        auto srcReg = std::dynamic_pointer_cast<Register>(src);

        // Determine if destination is a physical register (0-10)
        bool isDstPhysical = false;
        if (dstReg)
        {
            unsigned int id = dstReg->getID();
            if (id <= 10)
            {
                isDstPhysical = true;
            }
        }

        if (isDstPhysical)
        {
            // Physical registers can take memory or immediate sources directly
            bb->addInstruction(std::make_shared<MovMIR>(
                std::vector<std::shared_ptr<MachineIR>>{dst, src}));
        }
        else
        {
            // Destination is a VReg (> 10) or MemoryMIR.
            // Use rax (ID 0) as a bridge to prevent illegal Mem-to-Mem moves.
            bb->addInstruction(std::make_shared<MovMIR>(
                std::vector<std::shared_ptr<MachineIR>>{rax, src}));
            bb->addInstruction(std::make_shared<MovMIR>(
                std::vector<std::shared_ptr<MachineIR>>{dst, rax}));
        }
    };

    m_mirBlocks.clear();
    linearizeCFG();

    std::map<std::string, std::shared_ptr<BasicBlockMIR>> nameToMIR;
    std::vector<std::shared_ptr<BasicBlockMIR>> linearizedMIRBlock;

    for (const auto& bb : m_linearizedBlocks)
    {
        auto newMIR = std::make_shared<BasicBlockMIR>(bb->getName());
        nameToMIR[bb->getName()] = newMIR;
        linearizedMIRBlock.push_back(newMIR);
    }

    for (size_t i = 0; i < m_linearizedBlocks.size(); ++i)
    {
        auto& oldBB = m_linearizedBlocks[i];
        auto& mirBB = linearizedMIRBlock[i];

        for (const auto& succ : oldBB->getSuccessors())
        {
            mirBB->getSuccessors().push_back(nameToMIR[succ->getName()]);
        }
        for (const auto& pred : oldBB->getPredecessors())
        {
            mirBB->getPredecessors().push_back(nameToMIR[pred->getName()]);
        }
    }

    for (int i = 0; i < m_linearizedBlocks.size(); ++i)
    {
        auto& currBlock = m_linearizedBlocks[i];
        auto& bbMIR = linearizedMIRBlock[i];
        assert(bbMIR->getName() == currBlock->getName());

        // Calculates the element address from the base vreg every time it is
        // needed, we need to rematerialize the address each time to avoid
        // storing addresses in vregs which may become invalid after function
        // calls.
        auto resolveArrayAddress = [&](const std::string& arrayName, std::shared_ptr<Inst> index) -> std::shared_ptr<Register>
        {
            auto addrVReg = createTempVReg();

            // Check if we can fold the constant index
            if (index->getInstType() == InstType::IntConst)
            {
                int indexVal = std::dynamic_pointer_cast<IntConstInst>(index)->getVal();
                int elementOffset = indexVal * 8; // Scale by 8 bytes

                // Get the base array offset from map
                // If array is at [rbp-8], and index is 2, target is [rbp-8-16] = [rbp-24]
                unsigned int baseOffset = vRegToOffset[arrayName];
                int finalOffset = -(static_cast<int>(baseOffset) + elementOffset);

                // Emit a single LEA directly to the calculated element address
                auto foldedMem = std::make_shared<MemoryMIR>(rbp, std::make_shared<ConstMIR>(finalOffset));
                bbMIR->addInstruction(std::make_shared<LeaMIR>(
                    std::vector<std::shared_ptr<MachineIR>>{addrVReg, foldedMem}));
            }
            else
            {
                // Fallback: Dynamic rematerialization
                auto scaledIdxVReg = createTempVReg();
                auto rawIdxVReg = getOrCreateVReg("v_" + index->getString());

                // mov scaledIdxVReg, rawIdxVReg
                bbMIR->addInstruction(std::make_shared<MovMIR>(
                    std::vector<std::shared_ptr<MachineIR>>{scaledIdxVReg,
                                                            rawIdxVReg}));

                // mul scaledIdxVReg, 8
                bbMIR->addInstruction(std::make_shared<MulMIR>(
                    std::vector<std::shared_ptr<MachineIR>>{
                        scaledIdxVReg, std::make_shared<ConstMIR>(8)}));

                // lea addrVReg, [rbp - baseOffset]
                auto memLocation = memoryLocationForVReg(arrayName);
                bbMIR->addInstruction(std::make_shared<LeaMIR>(
                    std::vector<std::shared_ptr<MachineIR>>{addrVReg,
                                                            memLocation}));

                // sub addrVReg, scaledIdxVReg
                bbMIR->addInstruction(std::make_shared<SubMIR>(
                    std::vector<std::shared_ptr<MachineIR>>{addrVReg,
                                                            scaledIdxVReg}));
            }

            return addrVReg;
        };

        auto& inst = currBlock->getInstructions();

        for (int j = 0; j < inst.size(); ++j)
        {
            auto instType = inst[j]->getInstType();

            if (instType == InstType::FuncCall || instType == InstType::ProcCall)
            {
                auto isFunc = (instType == InstType::FuncCall);
                std::string callee;
                std::vector<std::shared_ptr<Inst>> arguments;

                if (isFunc)
                {
                    auto currInst = std::dynamic_pointer_cast<FuncCallInst>(inst[j]);
                    callee = currInst->getCalleeStr();
                    arguments = currInst->getOperands();
                }
                else
                {
                    auto currInst = std::dynamic_pointer_cast<ProcCallInst>(inst[j]);
                    callee = currInst->getCalleeStr();
                    arguments = currInst->getOperands();
                }

                // Windows x64 Calling Convention: rcx, rdx, r8, r9
                std::vector<std::shared_ptr<Register>> paramRegs = {rcx, rdx, r8, r9};

                for (int i = 0; i < arguments.size(); ++i)
                {
                    if (i >= 4)
                    {
                        throw std::runtime_error("CodeGen Error: More than 4 arguments not supported.");
                    }

                    const auto& argTarget = arguments[i]->getTarget();
                    std::shared_ptr<MachineIR> mirSource;

                    if (argTarget->getInstType() == InstType::IntConst)
                    {
                        auto intConst = std::dynamic_pointer_cast<IntConstInst>(argTarget);
                        mirSource = std::make_shared<ConstMIR>(intConst->getVal());
                    }
                    else if (argTarget->getInstType() == InstType::BoolConst)
                    {
                        auto boolConst = std::dynamic_pointer_cast<BoolConstInst>(argTarget);
                        mirSource = std::make_shared<ConstMIR>(boolConst->getVal() ? 1 : 0);
                    }
                    else
                    {
                        mirSource = getOrCreateVReg("v_" + argTarget->getString());
                    }

                    // Move to physical parameter register
                    auto movMIR = std::make_shared<MovMIR>(
                        std::vector<std::shared_ptr<MachineIR>>{paramRegs[i], mirSource});
                    bbMIR->addInstruction(movMIR);
                }

                auto callMIR = std::make_shared<CallMIR>(callee);
                bbMIR->addInstruction(callMIR);

                // If FuncCall, handle return value: mov targetVReg, rax
                if (isFunc)
                {
                    auto currInst = std::dynamic_pointer_cast<FuncCallInst>(inst[j]);
                    const auto& target = currInst->getTarget();
                    auto targetVReg = getOrCreateVReg("v_" + target->getString());

                    auto movToTargetMIR = std::make_shared<MovMIR>(
                        std::vector<std::shared_ptr<MachineIR>>{targetVReg, rax});
                    bbMIR->addInstruction(movToTargetMIR);
                }
            }
            else if (instType == InstType::Func)
            {
                auto currInst = std::dynamic_pointer_cast<Func>(inst[j]);
                auto& arguments = currInst->getParameters();

                // Windows x64 Calling Convention: parameters arrive in rcx, rdx, r8, r9
                std::vector<std::shared_ptr<Register>> paramRegs = {rcx, rdx, r8, r9};

                for (int i = 0; i < arguments.size(); ++i)
                {
                    if (i >= 4)
                    {
                        throw std::runtime_error(
                            "CodeGen Error: Function/procedure with more than 4 "
                            "parameters is not supported.");
                    }

                    auto& arg = arguments[i];
                    std::string vRegName = "v_" + arg->getName();
                    auto vreg = getOrCreateVReg(vRegName);

                    // mov vreg, physicalParamReg
                    // This "picks up" the value from the calling convention register
                    auto movMIR = std::make_shared<MovMIR>(
                        std::vector<std::shared_ptr<MachineIR>>{vreg, paramRegs[i]});
                    bbMIR->addInstruction(movMIR);
                }
            }
            else if (instType == InstType::Return)
            {
                auto returnInst = std::dynamic_pointer_cast<ReturnInst>(inst[j]);
                auto& operands = returnInst->getOperands();

                // Handle if this retInst coming from procedure (no return value)
                if (operands.size() == 0)
                {
                    continue;
                }

                const auto& expr = returnInst->getOperands()[0]->getTarget();
                std::shared_ptr<MachineIR> mirSource;

                // Resolve source (Constant or existing VReg)
                if (expr->getInstType() == InstType::IntConst)
                {
                    auto intConstInst = std::dynamic_pointer_cast<IntConstInst>(expr);
                    mirSource = std::make_shared<ConstMIR>(intConstInst->getVal());
                }
                else if (expr->getInstType() == InstType::BoolConst)
                {
                    auto boolConstInst = std::dynamic_pointer_cast<BoolConstInst>(expr);
                    int boolVal = boolConstInst->getVal() ? 1 : 0;
                    mirSource = std::make_shared<ConstMIR>(boolVal);
                }
                else if (expr->getInstType() == InstType::Ident)
                {
                    auto identInst = std::dynamic_pointer_cast<IdentInst>(expr);
                    mirSource = getOrCreateVReg("v_" + identInst->getString());
                }
                else
                {
                    throw std::runtime_error(
                        "CodeGen Error: Unsupported return expression type.");
                }

                // mov rax, source
                auto movMIR = std::make_shared<MovMIR>(
                    std::vector<std::shared_ptr<MachineIR>>{rax, mirSource});
                bbMIR->addInstruction(movMIR);
            }
            else if (instType == InstType::Assign)
            {
                auto assignInst = std::dynamic_pointer_cast<AssignInst>(inst[j]);
                auto targetVReg = getOrCreateVReg("v_" + assignInst->getTarget()->getString());
                auto source = assignInst->getSource();

                std::shared_ptr<MachineIR> mirSource;
                if (source->getInstType() == InstType::IntConst)
                {
                    mirSource = std::make_shared<ConstMIR>(std::dynamic_pointer_cast<IntConstInst>(source)->getVal());
                }
                else if (source->getInstType() == InstType::BoolConst)
                {
                    mirSource = std::make_shared<ConstMIR>(std::dynamic_pointer_cast<BoolConstInst>(source)->getVal() ? 1 : 0);
                }
                else
                {
                    mirSource = getOrCreateVReg("v_" + source->getString());
                }

                // Make sure no illegal mov occurs (memory to memory)
                legalizeMov(bbMIR, targetVReg, mirSource);
            }
            else if (instType == InstType::Put)
            {
                auto putInst = std::dynamic_pointer_cast<PutInst>(inst[j]);
                auto& operands = putInst->getOperands();
                const auto& targetOp = operands[0]->getTarget();
                auto outputType = targetOp->getInstType();

                if (outputType == InstType::IntConst)
                {
                    // Load format string (%d) into rcx
                    bbMIR->addInstruction(leaToLabel("fmt_str"));

                    // Load constant into rdx
                    auto intConst = std::dynamic_pointer_cast<IntConstInst>(targetOp);
                    auto constMIR = std::make_shared<ConstMIR>(intConst->getVal());
                    
                    bbMIR->addInstruction(std::make_shared<MovMIR>(
                        std::vector<std::shared_ptr<MachineIR>>{rdx, constMIR}));
                }
                else if (outputType == InstType::BoolConst)
                {
                    // Pick the correct string label
                    auto boolConst = std::dynamic_pointer_cast<BoolConstInst>(targetOp);
                    if (boolConst->getVal() == true)
                    {
                        bbMIR->addInstruction(leaToLabel("true_str"));
                    }
                    else
                    {
                        bbMIR->addInstruction(leaToLabel("false_str"));
                    }

                    // Pass boolean as int (0 or 1) into rdx
                    int boolVal = boolConst->getVal() ? 1 : 0;
                    auto constMIR = std::make_shared<ConstMIR>(boolVal);

                    bbMIR->addInstruction(std::make_shared<MovMIR>(
                        std::vector<std::shared_ptr<MachineIR>>{rdx, constMIR}));
                }
                else if (outputType == InstType::StrConst)
                {
                    auto strInst = std::dynamic_pointer_cast<StrConstInst>(targetOp);
                    auto outputStr = strInst->getString();

                    if (outputStr == "'\\n'")
                    {
                        bbMIR->addInstruction(leaToLabel("newline_str"));
                    }
                    else
                    {
                        // Generate a unique label for the literal
                        std::string sectionName = "literal" + std::to_string(strConstCtr++);
                        bbMIR->addInstruction(leaToLabel(sectionName));
                        strLiterals.push_back(outputStr);
                    }
                }
                else if (outputType == InstType::Ident)
                {
                    // Load format string (%d) into rcx
                    bbMIR->addInstruction(leaToLabel("fmt_str"));

                    auto identInst = std::dynamic_pointer_cast<IdentInst>(targetOp);
                    auto sourceVReg = getOrCreateVReg("v_" + identInst->getString());

                    // mov rdx, sourceVReg
                    bbMIR->addInstruction(std::make_shared<MovMIR>(
                        std::vector<std::shared_ptr<MachineIR>>{rdx, sourceVReg}));
                }

                auto callMIR = std::make_shared<CallMIR>("printf");
                bbMIR->addInstruction(callMIR);
            }
            else if (instType == InstType::Get)
            {
                auto getInst = std::dynamic_pointer_cast<GetInst>(inst[j]);
                const auto& target = getInst->getTarget();
                
                assert(target->getInstType() == InstType::Ident && "Can only get integer");
                auto identInst = std::dynamic_pointer_cast<IdentInst>(target);
                std::string targetVRegName = "v_" + identInst->getString();

                // Load format string (%d) into rcx
                bbMIR->addInstruction(leaToLabel("fmt_str"));

                // Create or get the VReg for the target variable
                // We will use this vreg to reload the value after scanf
                auto vreg = getOrCreateVReg(targetVRegName);

                // scanf MUST write to a memory address, so we pin it
                assignVRegToOffsetIfDoesNotExist(targetVRegName);

                // Load the address of the stack slot into rdx
                // lea rdx, QWORD PTR [rbp - offset]
                auto argLeaMIR = std::make_shared<LeaMIR>(
                    std::vector<std::shared_ptr<MachineIR>>{
                        rdx, memoryLocationForVReg(targetVRegName)});
                bbMIR->addInstruction(argLeaMIR);

                auto callMIR = std::make_shared<CallMIR>("scanf");
                bbMIR->addInstruction(callMIR);

                // Reload the value from memory into the VReg
                // This informs the Register Allocator that the VReg's value
                // has been updated by the memory write.
                auto movMIR = std::make_shared<MovMIR>(
                    std::vector<std::shared_ptr<MachineIR>>{
                        vreg, memoryLocationForVReg(targetVRegName)});
                bbMIR->addInstruction(movMIR);
            }
            else if (instType == InstType::Add || instType == InstType::Sub || instType == InstType::Mul)
            {
                auto& currInst = inst[j];
                const auto& targetStr = currInst->getTarget()->getString();
                const auto& operands = currInst->getOperands();

                auto targetVReg = getOrCreateVReg("v_" + targetStr);

                // Resolve Operand 1 and move to target via legalizer
                auto op1 = operands[0]->getTarget();
                std::shared_ptr<MachineIR> op1MIR = (op1->getInstType() == InstType::IntConst) ?
                    std::static_pointer_cast<MachineIR>(std::make_shared<ConstMIR>(std::dynamic_pointer_cast<IntConstInst>(op1)->getVal())) :
                    std::static_pointer_cast<MachineIR>(getOrCreateVReg("v_" + op1->getString()));

                legalizeMov(bbMIR, targetVReg, op1MIR);

                // Resolve Operand 2 and force it into a physical register (rdx)
                auto op2 = operands[1]->getTarget();
                std::shared_ptr<MachineIR> op2MIR = (op2->getInstType() == InstType::IntConst) ?
                    std::static_pointer_cast<MachineIR>(std::make_shared<ConstMIR>(std::dynamic_pointer_cast<IntConstInst>(op2)->getVal())) :
                    std::static_pointer_cast<MachineIR>(getOrCreateVReg("v_" + op2->getString()));

                bbMIR->addInstruction(std::make_shared<MovMIR>(std::vector<std::shared_ptr<MachineIR>>{rdx, op2MIR}));

                // Emit the actual arithmetic: [Target] Op [RDX]
                if (instType == InstType::Add)
                {
                    bbMIR->addInstruction(std::make_shared<AddMIR>(
                        std::vector<std::shared_ptr<MachineIR>>{targetVReg,
                                                                rdx}));
                }
                else if (instType == InstType::Sub)
                {
                    bbMIR->addInstruction(std::make_shared<SubMIR>(
                        std::vector<std::shared_ptr<MachineIR>>{targetVReg,
                                                                rdx}));
                }
                else
                {
                    bbMIR->addInstruction(std::make_shared<MulMIR>(
                        std::vector<std::shared_ptr<MachineIR>>{targetVReg,
                                                                rdx}));
                }
            }
            else if (instType == InstType::Div)
            {
                auto divInst = std::dynamic_pointer_cast<DivInst>(inst[j]);
                auto targetStr = divInst->getTarget()->getString();
                auto& operands = divInst->getOperands();
                const auto& operand1 = operands[0]->getTarget();
                const auto& operand2 = operands[1]->getTarget();

                // Prepare Dividend: mov rax, operand1
                if (operand1->getInstType() == InstType::IntConst)
                {
                    auto intConst = std::dynamic_pointer_cast<IntConstInst>(operand1);
                    bbMIR->addInstruction(std::make_shared<MovMIR>(
                        std::vector<std::shared_ptr<MachineIR>>{rax, std::make_shared<ConstMIR>(intConst->getVal())}));
                }
                else
                {
                    auto op1VReg = getOrCreateVReg("v_" + operand1->getString());
                    bbMIR->addInstruction(std::make_shared<MovMIR>(
                        std::vector<std::shared_ptr<MachineIR>>{rax, op1VReg}));
                }

                // Sign-extend rax into rdx (Requirement for idiv)
                bbMIR->addInstruction(std::make_shared<CqoMIR>());

                // Pin Divisor to Memory
                std::string divisorName;
                if (operand2->getInstType() == InstType::IntConst)
                {
                    // For constants, create a unique hidden stack variable
                    divisorName = "v_tmp" + std::to_string(tmpRegId++);
                    auto intConst = std::dynamic_pointer_cast<IntConstInst>(operand2);
                    
                    assignVRegToOffsetIfDoesNotExist(divisorName);
                    
                    // Store the constant into the pinned memory slot
                    bbMIR->addInstruction(std::make_shared<MovMIR>(
                        std::vector<std::shared_ptr<MachineIR>>{
                            memoryLocationForVReg(divisorName), 
                            std::make_shared<ConstMIR>(intConst->getVal())}));
                }
                else
                {
                    // For variables, ensure it's in the stack map
                    divisorName = "v_" + operand2->getString();
                    assignVRegToOffsetIfDoesNotExist(divisorName);
                    
                    // Sync current VReg value to memory before dividing
                    auto op2VReg = getOrCreateVReg(divisorName);
                    bbMIR->addInstruction(std::make_shared<MovMIR>(
                        std::vector<std::shared_ptr<MachineIR>>{
                            memoryLocationForVReg(divisorName), op2VReg}));
                }

                // idiv QWORD PTR [rbp - offset]
                bbMIR->addInstruction(std::make_shared<DivMIR>(memoryLocationForVReg(divisorName)));

                // mov targetVReg, rax
                auto targetVReg = getOrCreateVReg("v_" + targetStr);
                bbMIR->addInstruction(std::make_shared<MovMIR>(
                    std::vector<std::shared_ptr<MachineIR>>{targetVReg, rax}));
            }
            else if (instType == InstType::Not)
            {
                // Operand must be boolean
                auto notInst = std::dynamic_pointer_cast<NotInst>(inst[j]);

                const auto& targetStr = notInst->getTarget()->getString();
                const auto& operandStr = notInst->getOperand()->getTarget()->getString();

                auto targetVReg = getOrCreateVReg("v_" + targetStr);
                auto operandVReg = getOrCreateVReg("v_" + operandStr);

                // mov targetVReg, operandVReg
                // We copy the operand to the target first so we don't destroy the original variable
                auto prepareMov = std::make_shared<MovMIR>(
                    std::vector<std::shared_ptr<MachineIR>>{targetVReg, operandVReg});
                bbMIR->addInstruction(prepareMov);

                // Perform logical NOT by using xor targetVReg, 1
                auto notMIR = std::make_shared<NotMIR>(targetVReg);
                bbMIR->addInstruction(notMIR);                
            }
            else if (instType == InstType::Or || instType == InstType::And)
            {
                auto& currInst = inst[j];
                const auto& targetStr = currInst->getTarget()->getString();
                const auto& operands = currInst->getOperands();
                const auto& operand1 = operands[0]->getTarget();
                const auto& operand2 = operands[1]->getTarget();

                auto targetVReg = getOrCreateVReg("v_" + targetStr);

                std::shared_ptr<MachineIR> op1MIR;
                if (operand1->getInstType() == InstType::BoolConst)
                {
                    auto boolConst = std::dynamic_pointer_cast<BoolConstInst>(operand1);
                    op1MIR = std::make_shared<ConstMIR>(boolConst->getVal() ? 1 : 0);
                }
                else
                {
                    op1MIR = getOrCreateVReg("v_" + operand1->getString());
                }

                std::shared_ptr<MachineIR> op2MIR;
                if (operand2->getInstType() == InstType::BoolConst)
                {
                    auto boolConst = std::dynamic_pointer_cast<BoolConstInst>(operand2);
                    op2MIR = std::make_shared<ConstMIR>(boolConst->getVal() ? 1 : 0);
                }
                else
                {
                    op2MIR = getOrCreateVReg("v_" + operand2->getString());
                }

                // mov targetVReg, op1MIR
                bbMIR->addInstruction(std::make_shared<MovMIR>(
                    std::vector<std::shared_ptr<MachineIR>>{targetVReg, op1MIR}));

                if (instType == InstType::Or)
                {
                    bbMIR->addInstruction(std::make_shared<OrMIR>(
                        std::vector<std::shared_ptr<MachineIR>>{targetVReg, op2MIR}));
                }
                else
                {
                    bbMIR->addInstruction(std::make_shared<AndMIR>(
                        std::vector<std::shared_ptr<MachineIR>>{targetVReg, op2MIR}));
                }
            }
            else if (instType == InstType::CmpEq ||
                     instType == InstType::CmpNE ||
                     instType == InstType::CmpLT ||
                     instType == InstType::CmpLTE ||
                     instType == InstType::CmpGT ||
                     instType == InstType::CmpGTE)
            {
                auto& currInst = inst[j];
                auto targetStr = currInst->getTarget()->getString();
                auto& operands = currInst->getOperands();

                auto targetVReg = getOrCreateVReg("v_" + targetStr);

                auto getOpMIR = [&](std::shared_ptr<Inst> op) -> std::shared_ptr<MachineIR>
                {
                    if (op->getInstType() == InstType::IntConst)
                    {
                        return std::make_shared<ConstMIR>(std::dynamic_pointer_cast<IntConstInst>(op)->getVal());
                    }
                    return getOrCreateVReg("v_" + op->getString());
                };

                auto op1MIR = getOpMIR(operands[0]->getTarget());
                auto op2MIR = getOpMIR(operands[1]->getTarget());

                // Legalize Op1 and Op2 into physical registers before CMP.
                // We use rax and rdx directly as destinations here because 
                // we know they are physical (IDs 0 and 3).
                legalizeMov(bbMIR, rax, op1MIR);
                legalizeMov(bbMIR, rdx, op2MIR);

                // Now the CMP is guaranteed to be Reg-to-Reg
                bbMIR->addInstruction(std::make_shared<CmpMIR>(
                    std::vector<std::shared_ptr<MachineIR>>{rax, rdx}));

                std::shared_ptr<MachineIR> setccMIR;
                switch (instType)
                {
                    case InstType::CmpEq:  { setccMIR = std::make_shared<SeteMIR>(targetVReg); break; }
                    case InstType::CmpNE:  { setccMIR = std::make_shared<SetneMIR>(targetVReg); break; }
                    case InstType::CmpLT:  { setccMIR = std::make_shared<SetlMIR>(targetVReg); break; }
                    case InstType::CmpLTE: { setccMIR = std::make_shared<SetleMIR>(targetVReg); break; }
                    case InstType::CmpGT:  { setccMIR = std::make_shared<SetgMIR>(targetVReg); break; }
                    case InstType::CmpGTE: { setccMIR = std::make_shared<SetgeMIR>(targetVReg); break; }
                    default: { throw std::runtime_error("Unknown comparison type"); }
                }
    
                // Note: setcc results in a byte-sized write.
                // If targetVReg is a stack slot, this is a memory write.
                bbMIR->addInstruction(setccMIR);

                // Final zero-extension
                auto movzxMIR = std::make_shared<MovzxMIR>(targetVReg, 64, 8, true);
                bbMIR->addInstruction(movzxMIR);
            }
            else if (instType == InstType::Jump)
            {
                auto jumpInst = std::dynamic_pointer_cast<JumpInst>(inst[j]);
                auto targetBB = jumpInst->getJumpTarget();

                // Unconditional jump to the label of the target BasicBlock
                auto jmpMIR = std::make_shared<JmpMIR>(targetBB->getName());
                bbMIR->addInstruction(jmpMIR);
            }
            else if (instType == InstType::BRT || instType == InstType::BRF)
            {
                bool isBRT = (instType == InstType::BRT);

                std::shared_ptr<Inst> cond;
                std::shared_ptr<BasicBlock> targetSuccess;
                std::shared_ptr<BasicBlock> targetFailed;

                if (isBRT)
                {
                    auto brt = std::dynamic_pointer_cast<BRTInst>(inst[j]);
                    cond = brt->getCond()->getTarget();
                    targetSuccess = brt->getTargetSuccess();
                    targetFailed = brt->getTargetFailed();
                }
                else
                {
                    auto brf = std::dynamic_pointer_cast<BRFInst>(inst[j]);
                    cond = brf->getCond()->getTarget();
                    targetSuccess = brf->getTargetSuccess();
                    targetFailed = brf->getTargetFailed();
                }

                // Fetch the existing VReg for the condition variable
                auto condVReg = getOrCreateVReg("v_" + cond->getString());

                // Perform test to update Condition Codes (EFLAGS)
                // test condVReg, condVReg sets ZF=1 if value is 0, ZF=0 if value is 1
                bbMIR->addInstruction(std::make_shared<TestMIR>(condVReg, condVReg));

                if (isBRT)
                {
                    // BRT: Jump to success label if vreg != 0 (ZF=0)
                    bbMIR->addInstruction(std::make_shared<JnzMIR>(targetSuccess->getName()));
                }
                else
                {
                    // BRF: Jump to success label if vreg == 0 (ZF=1)
                    bbMIR->addInstruction(std::make_shared<JzMIR>(targetSuccess->getName()));
                }

                // Unconditional jump to the failed target (the "else" path)
                bbMIR->addInstruction(std::make_shared<JmpMIR>(targetFailed->getName()));
            }
            else if (instType == InstType::Alloca)
            {
                auto allocaInst = std::dynamic_pointer_cast<AllocaInst>(inst[j]);
                auto targetStr = allocaInst->getTarget()->getString();
    
                // Reserve stack space
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
            else if (instType == InstType::ArrUpdate || instType == InstType::ArrAccess)
            {
                bool isUpdate = (instType == InstType::ArrUpdate);
                std::string arrayName;
                std::shared_ptr<Inst> indexOperand;

                std::shared_ptr<ArrUpdateInst> arrUpdate = nullptr;
                std::shared_ptr<ArrAccessInst> arrAccess = nullptr;

                if (isUpdate)
                {
                    arrUpdate = std::dynamic_pointer_cast<ArrUpdateInst>(inst[j]);
                    arrayName = arrUpdate->getTarget()->getString();
                    indexOperand = arrUpdate->getIndex()->getTarget();
                }
                else
                {
                    arrAccess = std::dynamic_pointer_cast<ArrAccessInst>(inst[j]);
                    arrayName = arrAccess->getSource()->getTarget()->getString();
                    indexOperand = arrAccess->getIndex()->getTarget();
                }

                // Rematerialize element address (e.g., lea finalAddrVReg, [rbp - offset])
                auto finalAddrVReg = resolveArrayAddress(arrayName, indexOperand);

                // Create the memory operand [finalAddrVReg + 0]
                auto memOp = std::make_shared<MemoryMIR>(finalAddrVReg, 0);

                if (isUpdate)
                {
                    auto value = arrUpdate->getVal()->getTarget();
                    std::shared_ptr<MachineIR> valSource;

                    if (value->getInstType() == InstType::IntConst)
                    {
                        valSource = std::make_shared<ConstMIR>(
                            std::dynamic_pointer_cast<IntConstInst>(value)->getVal());
                    }
                    else
                    {
                        valSource = getOrCreateVReg("v_" + value->getString());
                    }

                    // Prevents mov QWORD PTR [reg_addr], QWORD PTR [stack_slot]
                    legalizeMov(bbMIR, memOp, valSource);
                }
                else
                {
                    auto targetVReg = getOrCreateVReg("v_" + arrAccess->getTarget()->getString());

                    // Prevents mov QWORD PTR [stack_slot], QWORD PTR [reg_addr]
                    legalizeMov(bbMIR, targetVReg, memOp);
                }
            }
        }

        auto& lastInst = currBlock->getInstructions().back();
        if (lastInst->getInstType() == InstType::Return ||
            lastInst->getInstType() == InstType::Halt)
        {
            // TERMINAL BLOCK: Clear successors to break the circle
            currBlock->getSuccessors().clear();
            bbMIR->getSuccessors().clear();

            bbMIR->addInstruction(std::make_shared<MovMIR>(
                std::vector<std::shared_ptr<MachineIR>>{
                    rax, std::make_shared<ConstMIR>(0)}));

            // Add RetMIR at the end of terminal block
            // For Liveness Analysis
            bbMIR->addInstruction(std::make_shared<RetMIR>());
        }
        m_mirBlocks.push_back(std::move(bbMIR));
    }

    for (unsigned int i = 0; i < strLiterals.size(); ++i)
    {
        auto& literalLabel = strLiterals[i];
        if (m_stringLiterals.count(literalLabel) == 0)
        {
            m_stringLiterals.insert(literalLabel);
            std::cout << "literal" << std::to_string(i) << ": .string ";
            std::cout << literalLabel << std::endl;
        }
    }

    // Shadow space 32 byte and 8 byte for each variable
    size_t offset = 32 + vRegToOffset.size() * 8;

    // Offset for arrays
    for (const auto& arrPair : arrVRegToSize)
    {
        offset += static_cast<size_t>(arrPair.second) * 8;
    }

    unsigned int aligned_offset = (offset + 15) & ~15; // 16-byte alignment
    std::cout << "    push rbp\n    mov rbp, rsp\n";
    std::cout << "    sub rsp, " << aligned_offset << std::endl;

    int ctr = 0;
    for (const auto& mirBlock : m_mirBlocks)
    {
        if(ctr++)
            std::cout << mirBlock->getName() << ": \n";
        mirBlock->printInstructions();
    }

    std::cout << "    add rsp, " << aligned_offset << std::endl;
    std::cout << "    mov rsp, rbp\n    pop rbp\n    ret\n";
}

void CodeGen::addSSA(std::string funcName, SSA& ssa)
{
    if (m_functionSSAMap.find(funcName) != m_functionSSAMap.end())
    {
        throw std::runtime_error("SSA for function '" + funcName + "' already exists in CodeGen.");
    }
    m_functionSSAMap[funcName] = ssa;
}

void CodeGen::generateAllFunctionsMIR()
{
    // Global prologue
    std::cout << std::endl << std::endl;
    std::cout << ".intel_syntax noprefix\n.globl main\n";
    std::cout << ".section .text\n";
    std::cout << "fmt_str: .string \"%d\"\n";
    std::cout << "true_str: .string \"true\"\n";
    std::cout << "false_str: .string \"false\"\n";

    std::cout << "main: \n";
    m_stringLiterals.insert("fmt_str");
    m_stringLiterals.insert("true_str");
    m_stringLiterals.insert("false_str");
    m_stringLiterals.insert("newline_str");

    // Generate MIR for main function first
    m_ssa.renameSSA();
    //m_ssa.printCFG();
    generateMIR();

    for (auto& [funcName, ssa] : m_functionSSAMap)
    {
        m_ssa = ssa;
        m_ssa.renameSSA();
        //m_ssa.printCFG();
        std::cout << "\n" << funcName << ": \n";
        generateMIR();
    }

    // Global epilogue
    std::cout << "\nnewline_str: .string \"\\n\"\n";
    std::cout << "\n";

    // Allocate registers
    RegisterAllocator ra(std::move(m_mirBlocks));
}

}  // namespace mina
