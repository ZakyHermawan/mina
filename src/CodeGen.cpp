#include "CodeGen.hpp"
#include "BasicBlock.hpp"
#include <asmjit/x86.h>

#include <memory>
#include <queue>
#include <set>

typedef int (*Func)(void);

asmjit::x86::Gp CodeGen::getFirstArgumentRegister(asmjit::x86::Compiler& cc)
{
    asmjit::x86::Gp arg1_reg = (cc.environment().isPlatformWindows())
        ? asmjit::x86::rcx   // 1st argument on
                             // Windows
        : asmjit::x86::rdi;  // 1st argument on
                             // Linux/macOS
    return arg1_reg;
}
asmjit::x86::Gp CodeGen::getSecondArgumentRegister(asmjit::x86::Compiler& cc)
{
    asmjit::x86::Gp arg2_reg = (cc.environment().isPlatformWindows())
        ? asmjit::x86::rdx  // 2nd argument on
                            // Windows
        : asmjit::x86::rsi; // 2nd argument on
                            // Linux/macOS
    return arg2_reg;
}

void CodeGen::syscallPutChar(asmjit::x86::Compiler& cc, char c)
{
    asmjit::x86::Gp arg1_reg = getFirstArgumentRegister(cc);
    cc.mov(arg1_reg, c);
    cc.sub(asmjit::x86::rsp, 32);
    cc.call(putchar);
    cc.add(asmjit::x86::rsp, 32);
}

// implement printf("%d", val);
void CodeGen::syscallPrintInt(asmjit::x86::Compiler& cc, int val)
{
    asmjit::x86::Gp arg1_reg = getFirstArgumentRegister(cc);
    asmjit::x86::Gp arg2_reg = getSecondArgumentRegister(cc);
    
    // Allocate 16 bytes on the stack for the format string and the integer.
    cc.sub(asmjit::x86::rsp, 16);
    
    // Write "%d\0" as a string to [rsp]. "%d" is 0x6425, null terminator is 0.
    cc.mov(asmjit::x86::word_ptr(asmjit::x86::rsp), 0x6425);  // "%d"
    cc.mov(asmjit::x86::byte_ptr(asmjit::x86::rsp, 2), 0);    // null terminator
    
    cc.lea(arg1_reg, asmjit::x86::ptr(asmjit::x86::rsp));
    cc.mov(arg2_reg, val);

    cc.sub(asmjit::x86::rsp, 32);
    cc.call(printf);
    cc.add(asmjit::x86::rsp, 32);
    
    // Restore stack
    cc.add(asmjit::x86::rsp, 16);
}

void CodeGen::syscallPrintString(asmjit::x86::Compiler& cc, std::string& str)
{
  for (int i = 0; i < str.size(); ++i)
  {
      syscallPutChar(cc, str[i]);
  }
}

void CodeGen::syscallScanInt(asmjit::x86::Compiler& cc, asmjit::x86::Gp reg)
{
    // currently, can only scan one digit integer
    asmjit::x86::Gp arg1_reg = getFirstArgumentRegister(cc);

    cc.sub(asmjit::x86::rsp, 32);
    cc.call(asmjit::Imm(getchar));
    cc.add(asmjit::x86::rsp, 32);
    cc.sub(asmjit::x86::eax, '0');
    cc.mov(reg, asmjit::x86::eax);
}

void CodeGen::generateX86()
{
    // BFS
    std::queue<std::shared_ptr<BasicBlock>> worklist;
    std::set<std::shared_ptr<BasicBlock>> visited;
    worklist.push(m_ssa.getCFG());
    visited.insert(m_ssa.getCFG());
    
    std::vector<std::string> variables;

    //asmjit::FileLogger logger(stdout);  // Logger should always survive CodeHolder.
    asmjit::StringLogger logger;
    // Runtime designed for JIT - it holds relocated functions and controls
    // their lifetime.
    asmjit::JitRuntime rt;

    // Holds code and relocation information during code generation.
    asmjit::CodeHolder code;

    // Initialize CodeHolder. It will be configured to match the host CPU
    // architecture. If you run this on an x86/x86_64 machine, it will generate
    // x86 code.
    code.init(rt.environment(), rt.cpuFeatures());
    code.setLogger(&logger);  // attach logger

    // Use x86::Assembler to emit x86/x86_64 code.
    asmjit::x86::Compiler cc(&code);
    cc.addFunc(asmjit::FuncSignature::build<void>());  // Begin a function of `int
                                                       // fn(void)` signature.

    std::unordered_map<std::string, asmjit::x86::Gp> registerMap;
    std::unordered_map<std::string, asmjit::Label> labelMap;

    while (!worklist.empty())
    {
        std::shared_ptr<BasicBlock> current_bb = worklist.front();
        auto bbName = current_bb->getName();
        if (labelMap.find(bbName) == labelMap.end())
        {
            labelMap[bbName] = cc.newNamedLabel(bbName.c_str());
        }
        auto& label = labelMap[bbName];
        cc.bind(label);

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
                        auto newReg = cc.newGp32(targetName.c_str());
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
                            auto err = cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand1);
                            cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        default:
                        {
                            cc.mov(
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
                            cc.add(targetRegister, casted->getVal());
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand2);
                            cc.add(targetRegister, casted->getVal());
                            break;
                        }
                        default:
                        {
                            cc.add(
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
                        auto newReg = cc.newGp32(targetName.c_str());
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
                            cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand1);
                            cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        default:
                        {
                            cc.mov(
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
                            cc.sub(targetRegister, casted->getVal());
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand2);
                            cc.sub(targetRegister, casted->getVal());
                            break;
                        }
                        default:
                        {
                            cc.sub(
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
                        auto newReg = cc.newGp32(targetName.c_str());
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
                            cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand1);
                            cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        default:
                        {
                            cc.mov(
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
                            auto tempReg = cc.newGp32();
                            cc.mov(tempReg, casted->getVal());
                            cc.imul(targetRegister, tempReg);
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand2);
                            auto tempReg = cc.newGp32();
                            cc.mov(tempReg, casted->getVal());
                            cc.imul(targetRegister, tempReg);
                            break;
                        }
                        default:
                        {
                            cc.imul(
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
                        auto newReg = cc.newGp32(targetName.c_str());
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
                            cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand1);
                            cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        default:
                        {
                            cc.mov(
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

                            auto a = cc.newInt32("a");
                            auto b = cc.newInt32("b");
                            auto dummy = cc.newInt32("dummy");

                            cc.xor_(dummy, dummy);
                            cc.mov(a, targetRegister);
                            cc.mov(b, casted->getVal());
                            cc.idiv(dummy, a, b);
                            cc.mov(targetRegister, a);

                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand2);

                            auto a = cc.newInt32("a");
                            auto b = cc.newInt32("b");
                            auto dummy = cc.newInt32("dummy");

                            cc.xor_(dummy, dummy);
                            cc.mov(a, targetRegister);
                            cc.mov(b, casted->getVal());
                            cc.idiv(dummy, a, b);
                            cc.mov(targetRegister, a);

                            break;
                        }
                        default:
                        {
                            auto a = cc.newInt32("a");
                            auto b = cc.newInt32("b");
                            auto dummy = cc.newInt32("dummy");

                            cc.xor_(dummy, dummy);
                            cc.mov(a, targetRegister);
                            cc.mov(b, registerMap[operand2->getTarget()->getString()]);
                            cc.idiv(dummy, a, b);
                            cc.mov(targetRegister, a);

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
                        auto newReg = cc.newGp32(targetName.c_str());
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
                            cc.mov(targetRegister, casted->getVal());
                            cc.neg(targetRegister);
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand1);
                            cc.mov(targetRegister, casted->getVal());
                            cc.neg(targetRegister);
                            break;
                        }
                        default:
                        {
                            cc.mov(
                                targetRegister,
                                registerMap[operand1->getTarget()->getString()]);
                            cc.neg(targetRegister);
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
                        auto newReg = cc.newGp32(targetName.c_str());
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
                            cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand1);
                            cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        default:
                        {
                            cc.mov(
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
                            auto tempReg = cc.newGp32();
                            cc.mov(tempReg, casted->getVal());
                            cc.and_(targetRegister, tempReg);
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand2);
                            auto tempReg = cc.newGp32();
                            cc.mov(tempReg, casted->getVal());
                            cc.and_(targetRegister, tempReg);
                            break;
                        }
                        default:
                        {
                            cc.and_(
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
                        auto newReg = cc.newGp32(targetName.c_str());
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
                            cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand1);
                            cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        default:
                        {
                            cc.mov(
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
                            auto tempReg = cc.newGp32();
                            cc.mov(tempReg, casted->getVal());
                            cc.or_(targetRegister, tempReg);
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand2);
                            auto tempReg = cc.newGp32();
                            cc.mov(tempReg, casted->getVal());
                            cc.or_(targetRegister, tempReg);
                            break;
                        }
                        default:
                        {
                            cc.or_(
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
                        auto newReg = cc.newGp32(targetName.c_str());
                        registerMap[targetName] = newReg;
                    }
                    auto& targetRegister = registerMap[targetName];
                    if (type == Type::BOOLEAN)
                    {
                        cc.sub(asmjit::x86::esp, size);
                        cc.mov(targetRegister, asmjit::x86::esp);
                    }
                    else if (type == Type::INTEGER)
                    {
                        cc.sub(asmjit::x86::esp, size * 4);
                        cc.mov(targetRegister, asmjit::x86::esp);
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
                    auto targetReg = cc.newGp32();
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
                            indexTempReg = cc.newGp32();
                            cc.mov(indexTempReg, casted->getVal() * 4);
                            break;
                        }
                        case Type::BOOLEAN:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(index);
                            indexTempReg = cc.newGp32();
                            cc.mov(indexTempReg, casted->getVal());
                            break;
                        }
                        default:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<IntConstInst>(index);
                            indexTempReg = registerMap[index->getTarget()->getString()];
                            cc.mov(indexTempReg, casted->getVal());
                            break;
                        }
                    }

                    cc.mov(targetReg, asmjit::x86::dword_ptr(sourceReg, indexTempReg));
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
                            indexTempReg = cc.newGp32();
                            cc.mov(indexTempReg, casted->getVal() * 4);
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
                            indexTempReg = cc.newGp32();
                            cc.mov(indexTempReg, casted->getVal());
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
                            cc.mov(indexTempReg, casted->getVal());
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
                            valueTempReg = cc.newGp32();
                            cc.mov(valueTempReg, casted->getVal());
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(val);
                            valueTempReg = cc.newGp32();
                            cc.mov(valueTempReg, casted->getVal());
                            break;
                        }
                        default:
                        {
                            valueTempReg =
                                registerMap[index->getTarget()->getString()];
                            break;
                        }
                    }

                    cc.mov(asmjit::x86::dword_ptr(sourceRegister, indexTempReg), valueTempReg);
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
                        auto newReg = cc.newGp32(targetName.c_str());
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
                            cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand1);
                            cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        default:
                        {
                            cc.mov(
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
                        auto newReg = cc.newGp32(targetName.c_str());
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
                            cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand1);
                            cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        default:
                        {
                            cc.mov(
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
                            auto tempReg = cc.newGp32();
                            cc.mov(tempReg, casted->getVal());
                            cc.cmp(targetRegister, tempReg);
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand2);
                            auto tempReg = cc.newGp32();
                            cc.mov(tempReg, casted->getVal());
                            cc.cmp(targetRegister, tempReg);
                            break;
                        }
                        default:
                        {
                            cc.cmp(
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
                        auto newReg = cc.newGp32(targetName.c_str());
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
                            cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand1);
                            cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        default:
                        {
                            cc.mov(
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
                            auto tempReg = cc.newGp32();
                            cc.mov(tempReg, casted->getVal());
                            cc.cmp(targetRegister, tempReg);
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand2);
                            auto tempReg = cc.newGp32();
                            cc.mov(tempReg, casted->getVal());
                            cc.cmp(targetRegister, tempReg);
                            break;
                        }
                        default:
                        {
                            cc.cmp(
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
                        auto newReg = cc.newGp32(targetName.c_str());
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
                            cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand1);
                            cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        default:
                        {
                            cc.mov(
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
                            auto tempReg = cc.newGp32();
                            cc.mov(tempReg, casted->getVal());
                            cc.cmp(targetRegister, tempReg);
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand2);
                            auto tempReg = cc.newGp32();
                            cc.mov(tempReg, casted->getVal());
                            cc.cmp(targetRegister, tempReg);
                            break;
                        }
                        default:
                        {
                            cc.cmp(
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
                        auto newReg = cc.newGp32(targetName.c_str());
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
                            cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand1);
                            cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        default:
                        {
                            cc.mov(
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
                            auto tempReg = cc.newGp32();
                            cc.mov(tempReg, casted->getVal());
                            cc.cmp(targetRegister, tempReg);
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand2);
                            auto tempReg = cc.newGp32();
                            cc.mov(tempReg, casted->getVal());
                            cc.cmp(targetRegister, tempReg);
                            break;
                        }
                        default:
                        {
                            cc.cmp(
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
                        auto newReg = cc.newGp32(targetName.c_str());
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
                            cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand1);
                            cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        default:
                        {
                            cc.mov(
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
                            auto tempReg = cc.newGp32();
                            cc.mov(tempReg, casted->getVal());
                            cc.cmp(targetRegister, tempReg);
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand2);
                            auto tempReg = cc.newGp32();
                            cc.mov(tempReg, casted->getVal());
                            cc.cmp(targetRegister, tempReg);
                            break;
                        }
                        default:
                        {
                            cc.cmp(
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
                        auto newReg = cc.newGp32(targetName.c_str());
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
                            cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand1);
                            cc.mov(targetRegister, casted->getVal());
                            break;
                        }
                        default:
                        {
                            cc.mov(
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
                            auto tempReg = cc.newGp32();
                            cc.mov(tempReg, casted->getVal());
                            cc.cmp(targetRegister, tempReg);
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto casted =
                                std::dynamic_pointer_cast<BoolConstInst>(operand2);
                            auto tempReg = cc.newGp32();
                            cc.mov(tempReg, casted->getVal());
                            cc.cmp(targetRegister, tempReg);
                            break;
                        }
                        default:
                        {
                            cc.cmp(
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
                            cc.newNamedLabel(targetLabelName.c_str());
                    }
                    auto& label = labelMap[targetLabelName];
                    cc.jmp(label);
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
                            cc.newNamedLabel(failLabelName.c_str());
                    }
                    auto& label = labelMap[failLabelName];
                    switch (state.last_comparison_type)
                    {
                        case CmpType::EQ:
                            cc.jne(label);
                            break;
                        case CmpType::NE:
                            cc.je(label);
                            break;
                        case CmpType::LT:
                            cc.jge(label);
                            break;
                        case CmpType::LTE:
                            cc.jg(label);
                            break;
                        case CmpType::GT:
                            cc.jle(label);
                            break;
                        case CmpType::GTE:
                            cc.jl(label);
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
                            cc.newNamedLabel(failLabelName.c_str());
                    }
                    auto& label = labelMap[failLabelName];
                    switch (state.last_comparison_type)
                    {
                        case CmpType::EQ:
                            cc.je(label);
                            break;
                        case CmpType::NE:
                            cc.jne(label);
                            break;
                        case CmpType::LT:
                            cc.jl(label);
                            break;
                        case CmpType::LTE:
                            cc.jle(label);
                            break;
                        case CmpType::GT:
                            cc.jg(label);
                            break;
                        case CmpType::GTE:
                            cc.jge(label);
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
                                syscallPrintInt(cc, val);
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
                                    syscallPrintString(cc, std::string("true"));
                                }
                                else
                                {
                                    syscallPrintString(cc, std::string("false"));
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
                                    syscallPutChar(cc, '\n');
                                    break;
                                }
                                for (int i = 0; i < val.length(); ++i)
                                {
                                    if(val[i] == '\"' || val[i] == '\'') continue;
                                    syscallPutChar(cc, val[i]);
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

                                auto& reg =
                                    registerMap[result];
                                cc.mov(asmjit::x86::ecx, reg);
                                cc.add(asmjit::x86::ecx, '0');
                                asmjit::Imm printWithParamAddr =
                                    asmjit::Imm((void*)putchar);
                                cc.sub(asmjit::x86::rsp, 32);
                                cc.call(printWithParamAddr);
                                cc.add(asmjit::x86::rsp, 32);
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
                        auto newReg = cc.newGp32(targetName.c_str());
                        registerMap[targetName] = newReg;
                    }
                    auto& targetRegister = registerMap[targetName];
                    syscallScanInt(cc, targetRegister);
                    cc.mov(targetRegister, asmjit::x86::eax);
                    break;
                }
                case InstType::Push:
                {
                    auto pushInst = std::dynamic_pointer_cast<GetInst>(currInst);
                    auto operand = pushInst->getOperands()[0]->getTarget();
                    auto operandType = operand->getInstType();
                    switch (operandType)
                    {
                        case InstType::IntConst:
                        {
                            auto intConstInst = std::dynamic_pointer_cast<IntConstInst>(operand);
                            auto val = intConstInst->getVal();
                            cc.push(val);
                            break;
                        }
                        case InstType::BoolConst:
                        {
                            auto boolConstInst = std::dynamic_pointer_cast<BoolConstInst>(operand);
                            auto val = boolConstInst->getVal();
                            cc.push(val);
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
                    break;
                }

                case InstType::FuncSignature:
                {
                    std::cout << "func signature!\n";
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

    cc.ret();
    cc.endFunc();   // End of the function body.
    cc.finalize();  // Translate and assemble the whole 'cc' content.    

    std::cout << "==================================" << std::endl;
    Func fn;
    asmjit::Error err = rt.add(&fn, &code);
    std::cout << logger.content().data() << std::endl;

    if (err)
    {
        printf("AsmJit failed: %s\n", asmjit::DebugUtils::errorAsString(err));
        return;
    }

    fn();
    std::cout << "JIT executed!\n";

    rt.release(fn);
}
