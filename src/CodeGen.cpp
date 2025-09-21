#include "CodeGen.hpp"
#include "BasicBlock.hpp"
#include <asmjit/x86.h>
#include <asmjit/core/globals.h>

#include <memory>
#include <queue>
#include <set>

typedef int (*Func)(void);

CodeGen::CodeGen(SSA ssa) : m_ssa{ssa}, m_logger{}, m_rt{}, m_code{}
{
    m_code.init(m_rt.environment(), m_rt.cpu_features());
    m_code.set_logger(&m_logger);  // attach logger
    m_cc = std::make_shared<asmjit::x86::Compiler>(&m_code);
}

void CodeGen::setSSA(SSA& ssa) { m_ssa = ssa; }

asmjit::x86::Gp CodeGen::getFirstArgumentRegister(std::shared_ptr<asmjit::x86::Compiler> m_cc)
{
    asmjit::x86::Gp arg1_reg = (m_cc->environment().is_platform_windows())
        ? asmjit::x86::rcx   // 1st argument on
                             // Windows
        : asmjit::x86::rdi;  // 1st argument on
                             // Linux/macOS
    return arg1_reg;
}
asmjit::x86::Gp CodeGen::getSecondArgumentRegister(std::shared_ptr<asmjit::x86::Compiler> m_cc)
{
    asmjit::x86::Gp arg2_reg = (m_cc->environment().is_platform_windows())
        ? asmjit::x86::rdx  // 2nd argument on
                            // Windows
        : asmjit::x86::rsi; // 2nd argument on
                            // Linux/macOS
    return arg2_reg;
}

void CodeGen::syscallPutChar(std::shared_ptr<asmjit::x86::Compiler> m_cc, char c)
{
    asmjit::x86::Gp arg1_reg = getFirstArgumentRegister(m_cc);
    m_cc->mov(arg1_reg, c);
    m_cc->sub(asmjit::x86::rsp, 32);
    m_cc->call(putchar);
    m_cc->add(asmjit::x86::rsp, 32);
}

// implement printf("%d", val);
void CodeGen::syscallPrintInt(std::shared_ptr<asmjit::x86::Compiler> m_cc, int val)
{
    asmjit::x86::Gp arg1_reg = getFirstArgumentRegister(m_cc);
    asmjit::x86::Gp arg2_reg = getSecondArgumentRegister(m_cc);
    
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
}

void CodeGen::syscallPrintString(std::shared_ptr<asmjit::x86::Compiler> m_cc,
                                 std::string& str)
{
  for (int i = 0; i < str.size(); ++i)
  {
      syscallPutChar(m_cc, str[i]);
  }
}

void CodeGen::syscallScanInt(std::shared_ptr<asmjit::x86::Compiler> m_cc,
                             asmjit::x86::Gp reg)
{
    // currently, can only scan one digit integer
    asmjit::x86::Gp arg1_reg = getFirstArgumentRegister(m_cc);

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

void CodeGen::generateX86(std::string funcName)
{
    // BFS
    std::queue<std::shared_ptr<BasicBlock>> worklist;
    std::set<std::shared_ptr<BasicBlock>> visited;
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
                                syscallPrintInt(m_cc, val);
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
                                    syscallPrintString(m_cc, std::string("true"));
                                }
                                else
                                {
                                    syscallPrintString(m_cc, std::string("false"));
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
                                    syscallPutChar(m_cc, '\n');
                                    break;
                                }
                                for (int i = 0; i < val.length(); ++i)
                                {
                                    if(val[i] == '\"' || val[i] == '\'') continue;
                                    syscallPutChar(m_cc, val[i]);
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
                                std::cout << "print var " << result << std::endl;
                                auto it = registerMap.find(result);
                                if (it == registerMap.end())
                                {
                                    exit(1);
                                }

                                auto& reg =
                                    registerMap[result];
                                m_cc->mov(asmjit::x86::rcx, reg);
                                m_cc->add(asmjit::x86::rcx, '0');
                                asmjit::Imm printWithParamAddr =
                                    asmjit::Imm((void*)putchar);
                                m_cc->sub(asmjit::x86::rsp, 32);
                                m_cc->call(printWithParamAddr);
                                //syscallPutChar(m_cc, );
                                m_cc->add(asmjit::x86::rsp, 32);
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
                    syscallScanInt(m_cc, targetRegister);
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
                        else
                        {
                            throw std::runtime_error("argument type other than int and identifier is not implemented yet");
                        }
                    }

                    //invoke_node->set_ret(0, g);
                    break;
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
                                std::string& name = identAST->getName();
                                registerMap[name] = getFirstArgumentRegister(m_cc);
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
    {
        return;  // Handle a possible error returned by AsmJit.
    }

    fn();
    std::cout << "JIT executed!\n";

    m_rt.release(fn);
}
