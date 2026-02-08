#include "MachineIR.hpp"

#include <set>
#include <array>
#include <utility>
#include <stdexcept>
#include <string>
#include <iostream>
#include <vector>
#include <sstream>
#include <memory>

namespace mina
{

// ==========================================
// MachineIR (Base)
// ==========================================

MIRType MachineIR::getMIRType() const
{
    return MIRType::None;
}

std::string MachineIR::getString() const
{
    return "";
}

std::vector<std::shared_ptr<MachineIR>>& MachineIR::getOperands()
{
    return m_operands;
}

// ==========================================
// BasicBlockMIR (Basic Block)
// ==========================================
BasicBlockMIR::BasicBlockMIR(std::string name)
    : m_name(std::move(name))
{
}

std::string BasicBlockMIR::getName() const { return m_name; }

std::vector<std::shared_ptr<MachineIR>>&
BasicBlockMIR::getInstructions()
{
    return m_instructions;
}

void BasicBlockMIR::printInstructions() const
{
    for (const auto& inst : m_instructions)
    {
        // RetMIR is only for liveness, skip it
        if(inst->getMIRType() == MIRType::Ret) continue;
        std::cout << "    " << inst->getString() << "\n";
    }
}

void BasicBlockMIR::addInstruction(std::shared_ptr<MachineIR> inst)
{
    m_instructions.push_back(std::move(inst));
}

void BasicBlockMIR::setSuccessorFromNames(const std::vector<std::string>& successorNames)
{
    for (const auto& succName : successorNames)
    {
        auto succBB = std::make_shared<BasicBlockMIR>(succName);
        m_successors.push_back(succBB);
    }
}

void BasicBlockMIR::setPredecessorFromNames(const std::vector<std::string>& predecessorNames)
{
    for (const auto& predName : predecessorNames)
    {
        auto predBB = std::make_shared<BasicBlockMIR>(predName);
        m_predecessors.push_back(predBB);
    }
}

std::vector<std::shared_ptr<BasicBlockMIR>>& BasicBlockMIR::getPredecessors()
{
    return m_predecessors;
}

std::vector<std::shared_ptr<BasicBlockMIR>>& BasicBlockMIR::getSuccessors()
{
    return m_successors;
}

void BasicBlockMIR::addDef(int regID)
{
    m_def.insert(regID);
}

void BasicBlockMIR::addUse(int regID)
{
    m_use.insert(regID);
}

void BasicBlockMIR::insertDef(int regID)
{
    m_def.insert(regID);
}

void BasicBlockMIR::insertLiveIn(int regID)
{
    m_liveIn.insert(regID);
}

void BasicBlockMIR::insertLiveOut(int regID)
{
    m_liveOut.insert(regID);
}

std::set<int>& BasicBlockMIR::getDef()
{
    return m_def;
}

std::set<int>& BasicBlockMIR::getUse()
{
    return m_use;
}

std::set<int>& BasicBlockMIR::getLiveIn()
{
    return m_liveIn;
}

std::set<int>& BasicBlockMIR::getLiveOut()
{
    return m_liveOut;
}

void BasicBlockMIR::generateDefUse()
{
    m_def.clear();
    m_use.clear();

    // Mark use first
    for (const auto& inst : m_instructions)
    {
        auto& operands = inst->getOperands();
        auto mirType = inst->getMIRType();

        auto getID = [](const std::shared_ptr<MachineIR>& op)
        {
            return std::dynamic_pointer_cast<Register>(op)->getID();
        };

        auto isReserved = [](int id)
        {
            // RBP = 11, RSP = 12, RIP = 13, R10 = 14, R11 = 15
            return (id >= 11 && id <= 15);
        };

        auto markUse = [&](int id)
        {
            if (!isReserved(id))
            {
                // Only mark as USE if not already defined in this block (Upward Exposed)
                if (m_def.find(id) == m_def.end())
                {
                    m_use.insert(id);
                }
            }
        };

        auto markDef = [&](int id)
        {
            if (!isReserved(id))
            {
                m_def.insert(id);
            }
        };

        switch (mirType)
        {
            case MIRType::Add:
            case MIRType::Sub:
            case MIRType::And:
            case MIRType::Or:
            {
                if (operands.size() > 1)
                {
                    if (operands[1]->getMIRType() == MIRType::Reg)
                    {
                        markUse(getID(operands[1]));
                    }
                }

                if (!operands.empty() && operands[0]->getMIRType() == MIRType::Reg)
                {
                    int id = getID(operands[0]);
                    markUse(id); // Read old value
                    markDef(id); // Write new value
                }
                break;
            }

            case MIRType::Mov:
            case MIRType::Lea:
            case MIRType::Movzx:
            {
                for (size_t i = 1; i < operands.size(); ++i)
                {
                    if (operands[i]->getMIRType() == MIRType::Reg)
                    {
                        markUse(getID(operands[i]));
                    }
                    else if (operands[i]->getMIRType() == MIRType::Memory)
                    {
                        auto memOp = std::dynamic_pointer_cast<MemoryMIR>(operands[i]);
                        if (memOp != nullptr && memOp->getBaseRegister() != nullptr)
                        {
                            markUse(memOp->getBaseRegister()->getID());
                        }
                    }
                }

                if (!operands.empty())
                {
                    if (operands[0]->getMIRType() == MIRType::Reg)
                    {
                        // Writing to a register is a DEF
                        markDef(getID(operands[0]));
                    }
                    else if (operands[0]->getMIRType() == MIRType::Memory)
                    {
                        // Writing to [reg] is a USE of reg (calculating address)
                        auto memOp = std::dynamic_pointer_cast<MemoryMIR>(operands[0]);
                        if (memOp != nullptr && memOp->getBaseRegister() != nullptr)
                        {
                            markUse(memOp->getBaseRegister()->getID());
                        }
                    }
                }
                break;
            }

            case MIRType::Call:
            {
                // Uses (Arguments)
                const auto callInst = std::dynamic_pointer_cast<CallMIR>(inst);
                unsigned int numArgs = callInst->getNumArgs();
                if (numArgs >= 1) markUse(to_int(RegID::RCX));
                if (numArgs >= 2) markUse(to_int(RegID::RDX));
                if (numArgs >= 3) markUse(to_int(RegID::R8));
                if (numArgs >= 4) markUse(to_int(RegID::R9));

                // Defs (Clobbers)
                markDef(to_int(RegID::RAX));
                markDef(to_int(RegID::RCX));
                markDef(to_int(RegID::RDX));
                markDef(to_int(RegID::R8));
                markDef(to_int(RegID::R9));
                markDef(to_int(RegID::R10));
                markDef(to_int(RegID::R11));
                break;
            }

            case MIRType::Ret:
            {
                markUse(to_int(RegID::RAX));
                break;
            }

            default:
            {
                break;
            }
        }
    }
}

void BasicBlockMIR::printLivenessSets() const
{
    auto printSet = [](const std::string& label, const std::set<int>& s)
    {
        std::cout << "  " << label << ": { ";
        for (int id : s)
        {
            if (id >= 0 && id < (int)RegID::COUNT)
            {
                std::cout << getReg(static_cast<RegID>(id))->get64BitName() << " ";
            }
            else
            {
                std::cout << "v" << id << " ";
            }
        }
        std::cout << "}\n";
    };

    std::cout << "Block: " << this->getName() << "\n";
    printSet("USE", m_use);
    printSet("DEF", m_def);
    printSet("LIVE-IN", m_liveIn);
    printSet("LIVE-OUT", m_liveOut);
    std::cout << "---------------------------\n";
}

void BasicBlockMIR::setLoopDepth(int depth)
{
    m_loopDepth = depth;
}

int BasicBlockMIR::getLoopDepth() const
{
    return m_loopDepth;
}

// ==========================================
// Register
// ==========================================

Register::Register(unsigned int id, std::string name, std::string _32bitName,
                   std::string _16BitName, std::string _8BitHighName,
                   std::string _8BitLowName)
    : m_id(id),
      m_regName(std::move(name)), m_32BitName(std::move(_32bitName)),
      m_16BitName(std::move(_16BitName)),
      m_8BitHighName(std::move(_8BitHighName)),
      m_8BitLowName(std::move(_8BitLowName))
{
}

Register::Register(unsigned int id, std::string name)
    : m_id(id), m_regName(std::move(name))
{
}

Register::Register(unsigned int id) 
    : m_id(id)
{
    m_regName = "R" + std::to_string(id);
}

MIRType Register::getMIRType() const 
{ 
    return MIRType::Reg; 
}

std::string Register::getString() const 
{ 
    return m_regName; 
}

unsigned int Register::getID() const 
{ 
    return m_id; 
}

std::string Register::get64BitName() const
{
    return getString();
}


std::string Register::get32BitName() const
{
    return m_32BitName;
}

std::string Register::get16BitName() const
{
    return m_16BitName;
}

std::string Register::get8BitHighName() const
{
    return m_8BitHighName;
}

std::string Register::get8BitLowName() const
{
    return m_8BitLowName;
}

// ==========================================
// LiteralMIR
// ==========================================

LiteralMIR::LiteralMIR(std::string literal)
    : m_literal(std::move(literal))
{
}

MIRType LiteralMIR::getMIRType() const { return MIRType::Literal; }

std::string LiteralMIR::getString() const { return m_literal; }

// ==========================================
// ConstMIR
// ==========================================

ConstMIR::ConstMIR(int constant) 
    : m_const(constant) 
{
}

MIRType ConstMIR::getMIRType() const 
{ 
    return MIRType::Const; 
}

std::string ConstMIR::getString() const
{
    return std::to_string(m_const);
}

int ConstMIR::getConst() const 
{ 
    return m_const; 
}

// ==========================================
// MemoryMIR
// ==========================================

MemoryMIR::MemoryMIR(std::shared_ptr<Register> reg, int offset)
    : m_reg(std::move(reg)), m_offset(std::make_shared<ConstMIR>(offset))
{
    m_operands.push_back(m_reg);
    m_operands.push_back(m_offset);
    m_operands.push_back(nullptr);  // Placeholder for literal
}

MemoryMIR::MemoryMIR(std::shared_ptr<Register> reg,
                     std::shared_ptr<ConstMIR> offset)
    : m_reg(std::move(reg)), m_offset(std::move(offset))
{
    m_operands.push_back(m_reg);
    m_operands.push_back(m_offset);
    m_operands.push_back(nullptr);  // Placeholder for literal
}

MemoryMIR::MemoryMIR(std::shared_ptr<Register> reg,
                     std::shared_ptr<LiteralMIR> literal)
    : m_reg(std::move(reg)), m_literal(std::move(literal))
{
    m_operands.push_back(m_reg);
    m_operands.push_back(nullptr);  // Placeholder for offset
    m_operands.push_back(m_literal);
}

MIRType MemoryMIR::getMIRType() const 
{ 
    return MIRType::Memory; 
}

std::string MemoryMIR::getString() const
{
    auto reg = std::dynamic_pointer_cast<Register>(m_operands[0]);
    auto offset = std::dynamic_pointer_cast<ConstMIR>(m_operands[1]);
    auto literal = std::dynamic_pointer_cast<LiteralMIR>(m_operands[2]);

    if (offset)
    {
        std::string strRepr = "QWORD PTR [" + reg->getString();
        int offsetVal = offset->getConst();
        if (offsetVal < 0)
        {
            strRepr += " - " + std::to_string(-offsetVal);
        }
        else if (offsetVal > 0)
        {
            strRepr += " + " + std::to_string(offsetVal);
        }

        strRepr += "]";
        return strRepr;
    }
    if (literal)
    {
        return "QWORD PTR [" + reg->getString() + " + " + literal->getString() + "]";
    }
    return "No Representation for MemoryMIR";
}

std::shared_ptr<Register>& MemoryMIR::getBaseRegister()
{
    return m_reg;
}

std::shared_ptr<MachineIR>& MemoryMIR::getOffsetOrLiteral()
{
    if (m_operands[1])
    {
        return m_operands[1];
    }
    return m_operands[2];
}

// ==========================================
// MovMIR
// ==========================================

MovMIR::MovMIR(std::vector<std::shared_ptr<MachineIR>> operands)
    : m_operands{std::move(operands)}
{
    if (m_operands.size() != 2)
    {
        throw std::runtime_error("MovMIR should have exactly 2 operands!");
    }
}

std::vector<std::shared_ptr<MachineIR>>& MovMIR::getOperands() 
{ 
    return m_operands; 
}

MIRType MovMIR::getMIRType() const 
{ 
    return MIRType::Mov; 
}

std::string MovMIR::getString() const
{
    // Access operands safely assuming the constructor check passed
    return "mov " + m_operands[0]->getString() + ", " + m_operands[1]->getString();
}

// ==========================================
// LeaMIR
// ==========================================

LeaMIR::LeaMIR(std::vector<std::shared_ptr<MachineIR>> operands)
    : m_operands(std::move(operands)) 
{
}

MIRType LeaMIR::getMIRType() const 
{
    return MIRType::Lea;
}

std::string LeaMIR::getString() const 
{
    if (m_operands.empty())
    {
        return "lea <error: no operands>";
    }

    std::stringstream ss;
    ss << "lea ";

    // Operand 0: Destination Register (e.g., "rax")
    if (m_operands.size() > 0 && m_operands[0])
    {
        ss << m_operands[0]->getString();
    }
    else
    {
        ss << "???";
    }

    ss << ", ";

    // Operand 1: Source Memory Address (e.g., "[rbp - 4]")
    // We assume the operand's own getString() handles the brackets [ ] 
    // or the underlying reference logic.
    if (m_operands.size() > 1 && m_operands[1])
    {
        ss << m_operands[1]->getString();
    }
    else
    {
        ss << "???";
    }

    return ss.str();
}

std::vector<std::shared_ptr<MachineIR>>& LeaMIR::getOperands()
{
    return m_operands;
}


// ==========================================
// CallMIR
// ==========================================

CallMIR::CallMIR(std::string calleeName, unsigned int numArgs)
    : m_calleeName{calleeName}, m_numArgs{numArgs}
{
}

MIRType CallMIR::getMIRType() const
{
    return MIRType::Call;
}

std::string CallMIR::getString() const
{
    return "call " + m_calleeName;
}

unsigned int CallMIR::getNumArgs() const
{
    return m_numArgs;
}

// ==========================================
// AddMIR
// ==========================================

AddMIR::AddMIR(std::vector<std::shared_ptr<MachineIR>> operands)
    : m_operands{std::move(operands)}
{
    if (m_operands.size() != 2)
    {
        throw std::runtime_error("AddMIR should have exactly 2 operands!");
    }
}

std::vector<std::shared_ptr<MachineIR>>& AddMIR::getOperands()
{ 
    return m_operands; 
}

MIRType AddMIR::getMIRType() const 
{ 
    return MIRType::Add;
}

std::string AddMIR::getString() const
{
    // Access operands safely assuming the constructor check passed
    return "add " + m_operands[0]->getString() + ", " + m_operands[1]->getString();
}

// ==========================================
// SubMIR
// ==========================================

SubMIR::SubMIR(std::vector<std::shared_ptr<MachineIR>> operands)
    : m_operands{std::move(operands)}
{
    if (m_operands.size() != 2)
    {
        throw std::runtime_error("SubMIR should have exactly 2 operands!");
    }
}

std::vector<std::shared_ptr<MachineIR>>& SubMIR::getOperands()
{ 
    return m_operands; 
}

MIRType SubMIR::getMIRType() const 
{ 
    return MIRType::Sub;
}

std::string SubMIR::getString() const
{
    // Access operands safely assuming the constructor check passed
    return "sub " + m_operands[0]->getString() + ", " + m_operands[1]->getString();
}

// ==========================================
// MulMIR
// ==========================================

MulMIR::MulMIR(std::vector<std::shared_ptr<MachineIR>> operands)
    : m_operands{std::move(operands)}
{
    if (m_operands.size() != 2)
    {
        throw std::runtime_error("MulMIR should have exactly 2 operands!");
    }
}

std::vector<std::shared_ptr<MachineIR>>& MulMIR::getOperands()
{ 
    return m_operands; 
}

MIRType MulMIR::getMIRType() const 
{ 
    return MIRType::Mul;
}

std::string MulMIR::getString() const
{
    // Access operands safely assuming the constructor check passed
    return "imul " + m_operands[0]->getString() + ", " + m_operands[1]->getString();
}

// ==========================================
// DivMIR
// ==========================================

DivMIR::DivMIR(std::shared_ptr<MemoryMIR> divisor)
    : m_divisor{std::move(divisor)}
{
    m_operands.push_back(m_divisor);
}

std::vector<std::shared_ptr<MachineIR>>& DivMIR::getOperands()
{ 
    return m_operands;
}

MIRType DivMIR::getMIRType() const 
{ 
    return MIRType::Div;
}

std::string DivMIR::getString() const
{
    // Access operands safely assuming the constructor check passed
    return "idiv " + m_operands[0]->getString();
}

// ==========================================
// CqoMIR
// ==========================================

CqoMIR::CqoMIR()
{
}

MIRType CqoMIR::getMIRType() const
{
    return MIRType::Cqo;
}

std::string CqoMIR::getString() const
{
    return "cqo";
}

// ==========================================
// NotMIR
// ==========================================

NotMIR::NotMIR(std::shared_ptr<Register> operand)
    : m_operand{std::move(operand)}
{
    m_operands.push_back(m_operand);
}

std::vector<std::shared_ptr<MachineIR>>& NotMIR::getOperands()
{
    return m_operands;
}

MIRType NotMIR::getMIRType() const 
{ 
    return MIRType::Not;
}

std::string NotMIR::getString() const
{
    return "xor " + m_operands[0]->getString() + ", 1";
}

// ==========================================
// AndMIR
// ==========================================

AndMIR::AndMIR(std::vector<std::shared_ptr<MachineIR>> operands)
    : m_operands{std::move(operands)}
{
    if (m_operands.size() != 2)
    {
        throw std::runtime_error("AndMIR should have exactly 2 operands!");
    }
}

std::vector<std::shared_ptr<MachineIR>>& AndMIR::getOperands()
{ 
    return m_operands; 
}

MIRType AndMIR::getMIRType() const 
{ 
    return MIRType::And;
}

std::string AndMIR::getString() const
{
    // Access operands safely assuming the constructor check passed
    return "and " + m_operands[0]->getString() + ", " + m_operands[1]->getString();
}

// ==========================================
// OrMIR
// ==========================================

OrMIR::OrMIR(std::vector<std::shared_ptr<MachineIR>> operands)
    : m_operands{std::move(operands)}
{
    if (m_operands.size() != 2)
    {
        throw std::runtime_error("OrMIR should have exactly 2 operands!");
    }
}

std::vector<std::shared_ptr<MachineIR>>& OrMIR::getOperands()
{ 
    return m_operands; 
}

MIRType OrMIR::getMIRType() const 
{ 
    return MIRType::Or;
}

std::string OrMIR::getString() const
{
    // Access operands safely assuming the constructor check passed
    return "or " + m_operands[0]->getString() + ", " + m_operands[1]->getString();
}

// ==========================================
// CmpMIR
// ==========================================

CmpMIR::CmpMIR(std::vector<std::shared_ptr<MachineIR>> operands)
    : m_operands{std::move(operands)}
{
    if (m_operands.size() != 2)
    {
        throw std::runtime_error("CmpMIR should have exactly 2 operands!");
    }
}

std::vector<std::shared_ptr<MachineIR>>& CmpMIR::getOperands()
{ 
    return m_operands;
}

MIRType CmpMIR::getMIRType() const 
{ 
    return MIRType::Cmp;
}

std::string CmpMIR::getString() const
{
    // Access operands safely assuming the constructor check passed
    return "cmp " + m_operands[0]->getString() + ", " + m_operands[1]->getString();
}

// ==========================================
// SeteMIR
// ==========================================

SeteMIR::SeteMIR(std::shared_ptr<Register> reg)
    : m_reg{std::move(reg)}
{
    m_operands.push_back(m_reg);
}

MIRType SeteMIR::getMIRType() const
{
    return MIRType::Sete;
}

std::string SeteMIR::getString() const
{
    auto reg = std::dynamic_pointer_cast<Register>(m_operands[0]);
    return "sete " + reg->get8BitLowName();
}

std::vector<std::shared_ptr<MachineIR>>& SeteMIR::getOperands()
{
    return m_operands;
}

// ==========================================
// SetneMIR
// ==========================================

SetneMIR::SetneMIR(std::shared_ptr<Register> reg)
    : m_reg{std::move(reg)}
{
    m_operands.push_back(m_reg);
}

MIRType SetneMIR::getMIRType() const
{
    return MIRType::Setne;
}

std::string SetneMIR::getString() const
{
    auto reg = std::dynamic_pointer_cast<Register>(m_operands[0]);
    return "setne " + reg->get8BitLowName();
}

std::vector<std::shared_ptr<MachineIR>>& SetneMIR::getOperands()
{
    return m_operands;
}

// ==========================================
// SetlMIR
// ==========================================

SetlMIR::SetlMIR(std::shared_ptr<Register> reg)
    : m_reg{std::move(reg)}
{
    m_operands.push_back(m_reg);
}

MIRType SetlMIR::getMIRType() const
{
    return MIRType::Setl;
}

std::string SetlMIR::getString() const
{
    auto reg = std::dynamic_pointer_cast<Register>(m_operands[0]);
    return "setl " + reg->get8BitLowName();
}

std::vector<std::shared_ptr<MachineIR>>& SetlMIR::getOperands()
{
    return m_operands;
}

// ==========================================
// SetleMIR
// ==========================================

SetleMIR::SetleMIR(std::shared_ptr<Register> reg)
    : m_reg{std::move(reg)}
{
    m_operands.push_back(m_reg);
}

MIRType SetleMIR::getMIRType() const
{
    return MIRType::Setle;
}

std::string SetleMIR::getString() const
{
    auto reg = std::dynamic_pointer_cast<Register>(m_operands[0]);
    return "setle " + reg->get8BitLowName();
}

std::vector<std::shared_ptr<MachineIR>>& SetleMIR::getOperands()
{
    return m_operands;
}

// ==========================================
// SetgMIR
// ==========================================

SetgMIR::SetgMIR(std::shared_ptr<Register> reg)
    : m_reg{std::move(reg)}
{
    m_operands.push_back(m_reg);
}

MIRType SetgMIR::getMIRType() const
{
    return MIRType::Setg;
}

std::string SetgMIR::getString() const
{
    auto reg = std::dynamic_pointer_cast<Register>(m_operands[0]);
    return "setg " + reg->get8BitLowName();
}

std::vector<std::shared_ptr<MachineIR>>& SetgMIR::getOperands()
{
    return m_operands;
}

// ==========================================
// SetgeMIR
// ==========================================

SetgeMIR::SetgeMIR(std::shared_ptr<Register> reg)
    : m_reg{std::move(reg)}
{
    m_operands.push_back(m_reg);
}

MIRType SetgeMIR::getMIRType() const
{
    return MIRType::Setge;
}

std::string SetgeMIR::getString() const
{
    auto reg = std::dynamic_pointer_cast<Register>(m_operands[0]);
    return "setge " + reg->get8BitLowName();
}

std::vector<std::shared_ptr<MachineIR>>& SetgeMIR::getOperands()
{
    return m_operands;
}

// ==========================================
// MovzxMIR
// ==========================================

MovzxMIR::MovzxMIR(std::shared_ptr<Register> reg, unsigned int toRegSize,
    unsigned int fromRegSize, bool isFromRegLow)
    : m_reg{std::move(reg)},
      m_toRegSize(toRegSize),
      m_fromRegSize(fromRegSize),
      m_isFromRegLow(isFromRegLow)
{
    m_operands.push_back(m_reg);
}

MIRType MovzxMIR::getMIRType() const
{
    return MIRType::Movzx;
}

std::string MovzxMIR::getString() const
{
    std::string strRepr = "movzx ";
    auto reg = std::dynamic_pointer_cast<Register>(m_reg);
    // Determine destination register name based on size
    if (m_toRegSize == 64)
    {
        strRepr += reg->getString(); // Full register name for 64-bit
    }
    else if (m_toRegSize == 32)
    {
        strRepr += reg->get32BitName();
    }
    else if (m_toRegSize == 16)
    {
        strRepr += reg->get16BitName();
    }
    else
    {
        throw std::runtime_error("Movzx can only accept source register with size 64, 32, or 16 bits!");
    }
    strRepr += ", ";

    // Determine source register name based on size and low/high byte
    if (m_fromRegSize == 8)
    {
        if (m_isFromRegLow)
        {
            strRepr += reg->get8BitLowName();
        }
        else
        {
            strRepr += reg->get8BitHighName();
        }
    }
    else if (m_fromRegSize == 16)
    {
        strRepr += reg->get16BitName();
    }
    else if (m_fromRegSize == 32)
    {
        strRepr += reg->get32BitName();
    }
    else if (m_fromRegSize == 64)
    {
        strRepr += reg->getString();
    }
    else
    {
        throw std::runtime_error(
            "Movzx can only accept source register with size 64, 32, 16, "
            "or 8 bits!");
    }

    return strRepr;
}

std::vector<std::shared_ptr<MachineIR>>& MovzxMIR::getOperands()
{
    return m_operands;
}

unsigned int MovzxMIR::getToRegSize() const
{
    return m_toRegSize;
}

unsigned int MovzxMIR::getFromRegSize() const
{
    return m_fromRegSize;
}

bool MovzxMIR::isFromRegLow() const
{
    return m_isFromRegLow;
}


// ==========================================
// TestMIR
// ==========================================

TestMIR::TestMIR(std::shared_ptr<Register> reg1, std::shared_ptr<Register> reg2)
    : m_reg1{std::move(reg1)}, m_reg2{std::move(reg2)}
{
    m_operands.push_back(m_reg1);
    m_operands.push_back(m_reg2);
}

MIRType TestMIR::getMIRType() const
{
    return MIRType::Test;
}

std::string TestMIR::getString() const
{
    return "test " + m_operands[0]->getString() + ", " +  m_operands[1]->getString();
}

std::vector<std::shared_ptr<MachineIR>>& TestMIR::getOperands()
{
    return m_operands;
}

// ==========================================
// JmpMIR
// ==========================================

JmpMIR::JmpMIR(std::string targetLabel) : m_targetLabel{targetLabel}
{
}

MIRType JmpMIR::getMIRType() const
{
    return MIRType::Jmp;
}

std::string JmpMIR::getString() const
{
    return "jmp " + m_targetLabel;
}

std::string JmpMIR::getTargetLabel() const
{
    return m_targetLabel;
}

// ==========================================
// JzMIR
// ==========================================

JzMIR::JzMIR(std::string targetLabel) : m_targetLabel{targetLabel}
{
}

MIRType JzMIR::getMIRType() const
{
    return MIRType::Jz;
}

std::string JzMIR::getString() const
{
    return "jz " + m_targetLabel;
}

std::string JzMIR::getTargetLabel() const
{
    return m_targetLabel;
}

// ==========================================
// JnzMIR
// ==========================================

JnzMIR::JnzMIR(std::string targetLabel) : m_targetLabel{targetLabel}
{
}

MIRType JnzMIR::getMIRType() const
{
    return MIRType::Jmp;
}

std::string JnzMIR::getString() const
{
    return "jnz " + m_targetLabel;
}

std::string JnzMIR::getTargetLabel() const
{
    return m_targetLabel;
}

// ==========================================
// RetMIR
// ==========================================

RetMIR::RetMIR() {}

MIRType RetMIR::getMIRType() const { return MIRType::Ret; }

// Does not print anything because we assume "ret" is handled at a higher level
// RetMIR is only for liveness Anaysis
std::string RetMIR::getString() const { return ""; }

// Use static to prevent this function to be called from another file
static std::array<std::shared_ptr<Register>, to_int(RegID::COUNT)> registersFactory()
{
    // Initialize with COUNT to pre-allocate exact size
    std::array<std::shared_ptr<Register>, to_int(RegID::COUNT)> registers;

    auto add = [&](RegID id, auto... names)
    {
        registers[to_int(id)] = std::make_shared<Register>(to_int(id), names...);
    };

    add(RegID::RAX, "rax", "eax", "ax",  "ah",  "al");
    add(RegID::RBX, "rbx", "ebx", "bx",  "bh",  "bl");
    add(RegID::RCX, "rcx", "ecx", "cx",  "ch",  "cl");
    add(RegID::RDX, "rdx", "edx", "dx",  "dh",  "dl");
    add(RegID::RDI, "rdi", "edi", "di",  "dil", "");
    add(RegID::RSI, "rsi", "esi", "si",  "sil", "");
    add(RegID::R8,  "r8",  "r8d", "r8w", "r8b", "");
    add(RegID::R9,  "r9",  "r9d", "r9w", "r9b", "");
    add(RegID::R12, "r12", "r12d", "r12w", "r12b", "");
    add(RegID::R13, "r13", "r13d", "r13w", "r13b", "");
    add(RegID::R14, "r14", "r14d", "r14w", "r14b", "");

    add(RegID::RBP, "rbp", "ebp", "bp", "bpl", "");
    add(RegID::RSP, "rsp", "esp", "sp", "spl", "");
    add(RegID::RIP, "rip");

    add(RegID::R10, "r10", "r10d", "r10w", "r10b", "");
    add(RegID::R11, "r11", "r11d", "r11w", "r11b", "");

    return registers;
}

const std::array<std::shared_ptr<Register>, to_int(RegID::COUNT)>&
getAllRegisters()
{
    static const auto registers = registersFactory();
    return registers;
}

const std::shared_ptr<Register>& getReg(RegID id)
{
    return getAllRegisters()[to_int(id)];
}

}  // namespace mina
