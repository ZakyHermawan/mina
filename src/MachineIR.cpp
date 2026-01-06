#include "MachineIR.hpp"
#include <utility>
#include <stdexcept>
#include <string>
#include <iostream>
#include <vector>
#include <sstream>
#include <memory>

std::string BasicBlockMIR::getName() const { return m_name; }

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

// ==========================================
// BasicBlockMIR (Basic Block)
// ==========================================
BasicBlockMIR::BasicBlockMIR(std::string name)
    : m_name(std::move(name))
{
}

std::vector<std::shared_ptr<MachineIR>>&
BasicBlockMIR::getInstructions()
{
    return m_instructions;
}

void BasicBlockMIR::printInstructions() const
{
    for (const auto& inst : m_instructions)
    {
        std::cout << "    " << inst->getString() << "\n";
    }
}

void BasicBlockMIR::addInstruction(std::shared_ptr<MachineIR> inst)
{
    m_instructions.push_back(std::move(inst));
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
}

MemoryMIR::MemoryMIR(std::shared_ptr<Register> reg,
                     std::shared_ptr<ConstMIR> offset)
    : m_reg(std::move(reg)), m_offset(std::move(offset))
{
}

MemoryMIR::MemoryMIR(std::shared_ptr<Register> reg,
                     std::shared_ptr<LiteralMIR> literal)
    : m_reg(std::move(reg)), m_literal(std::move(literal))
{
}

MIRType MemoryMIR::getMIRType() const 
{ 
    return MIRType::Memory; 
}

std::string MemoryMIR::getString() const
{
    
    if (m_offset)
    {
        std::string strRepr = "QWORD PTR [" + m_reg->getString();
        int offsetVal = m_offset->getConst();
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
    if (m_literal)
    {
        return "[" + m_reg->getString() + " + " + m_literal->getString() + "]";
    }
    return "No Representation for MemoryMIR";
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
    return MIRType::Lea; // Assuming MIRType::LEA exists in your enum
}

std::string LeaMIR::getString() const 
{
    if (m_operands.empty()) {
        return "lea <error: no operands>";
    }

    std::stringstream ss;
    ss << "lea ";

    // Operand 0: Destination Register (e.g., "rax")
    if (m_operands.size() > 0 && m_operands[0]) {
        ss << m_operands[0]->getString();
    } else {
        ss << "???";
    }

    ss << ", ";

    // Operand 1: Source Memory Address (e.g., "[rbp - 4]")
    // We assume the operand's own getString() handles the brackets [ ] 
    // or the underlying reference logic.
    if (m_operands.size() > 1 && m_operands[1]) {
        ss << m_operands[1]->getString();
    } else {
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

CallMIR::CallMIR(std::string calleeName) : m_calleeName{calleeName}
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

// ==========================================
// AddMIR
// ==========================================

AddMIR::AddMIR(std::vector<std::shared_ptr<MachineIR>> operands)
    : m_operands{std::move(operands)}
{
    if (m_operands.size() != 2)
    {
        throw std::runtime_error("MovMIR should have exactly 2 operands!");
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
        throw std::runtime_error("MovMIR should have exactly 2 operands!");
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
        throw std::runtime_error("MovMIR should have exactly 2 operands!");
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
}

std::vector<std::shared_ptr<MachineIR>>& DivMIR::getOperands()
{ 
    return std::vector<std::shared_ptr<MachineIR>>{m_divisor};
}

MIRType DivMIR::getMIRType() const 
{ 
    return MIRType::Div;
}

std::string DivMIR::getString() const
{
    // Access operands safely assuming the constructor check passed
    return "idiv QWORD PTR [" + m_divisor->getString() + "]";
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
}

std::vector<std::shared_ptr<MachineIR>>& NotMIR::getOperands()
{
    return std::vector<std::shared_ptr<MachineIR>>{m_operand};
}

MIRType NotMIR::getMIRType() const 
{ 
    return MIRType::Not;
}

std::string NotMIR::getString() const
{
    return "xor " + m_operand->getString() + ", 1";
}

// ==========================================
// AndMIR
// ==========================================

AndMIR::AndMIR(std::vector<std::shared_ptr<MachineIR>> operands)
    : m_operands{std::move(operands)}
{
    if (m_operands.size() != 2)
    {
        throw std::runtime_error("MovMIR should have exactly 2 operands!");
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
        throw std::runtime_error("MovMIR should have exactly 2 operands!");
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
        throw std::runtime_error("MovMIR should have exactly 2 operands!");
    }
}

std::vector<std::shared_ptr<MachineIR>>& CmpMIR::getOperands()
{ 
    return m_operands; 
}

MIRType CmpMIR::getMIRType() const 
{ 
    return MIRType::Or;
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
}

MIRType SeteMIR::getMIRType() const
{
    return MIRType::Sete;
}

std::string SeteMIR::getString() const
{
    return "sete " + m_reg->get8BitLowName();
}

std::vector<std::shared_ptr<MachineIR>>& SeteMIR::getOperands()
{
    return std::vector<std::shared_ptr<MachineIR>>{m_reg};
}

// ==========================================
// SetneMIR
// ==========================================

SetneMIR::SetneMIR(std::shared_ptr<Register> reg)
    : m_reg{std::move(reg)}
{
}

MIRType SetneMIR::getMIRType() const
{
    return MIRType::Setne;
}

std::string SetneMIR::getString() const
{
    return "setne " + m_reg->get8BitLowName();
}

std::vector<std::shared_ptr<MachineIR>>& SetneMIR::getOperands()
{
    return std::vector<std::shared_ptr<MachineIR>>{m_reg};
}

// ==========================================
// SetlMIR
// ==========================================

SetlMIR::SetlMIR(std::shared_ptr<Register> reg)
    : m_reg{std::move(reg)}
{
}

MIRType SetlMIR::getMIRType() const
{
    return MIRType::Setl;
}

std::string SetlMIR::getString() const
{
    return "setl " + m_reg->get8BitLowName();
}

std::vector<std::shared_ptr<MachineIR>>& SetlMIR::getOperands()
{
    return std::vector<std::shared_ptr<MachineIR>>{m_reg};
}

// ==========================================
// SetleMIR
// ==========================================

SetleMIR::SetleMIR(std::shared_ptr<Register> reg)
    : m_reg{std::move(reg)}
{
}

MIRType SetleMIR::getMIRType() const
{
    return MIRType::Setle;
}

std::string SetleMIR::getString() const
{
    return "setle " + m_reg->get8BitLowName();
}

std::vector<std::shared_ptr<MachineIR>>& SetleMIR::getOperands()
{
    return std::vector<std::shared_ptr<MachineIR>>{m_reg};
}

// ==========================================
// SetgMIR
// ==========================================

SetgMIR::SetgMIR(std::shared_ptr<Register> reg)
    : m_reg{std::move(reg)}
{
}

MIRType SetgMIR::getMIRType() const
{
    return MIRType::Setg;
}

std::string SetgMIR::getString() const
{
    return "setg " + m_reg->get8BitLowName();
}

std::vector<std::shared_ptr<MachineIR>>& SetgMIR::getOperands()
{
    return std::vector<std::shared_ptr<MachineIR>>{m_reg};
}

// ==========================================
// SetgeMIR
// ==========================================

SetgeMIR::SetgeMIR(std::shared_ptr<Register> reg)
    : m_reg{std::move(reg)}
{
}

MIRType SetgeMIR::getMIRType() const
{
    return MIRType::Setge;
}

std::string SetgeMIR::getString() const
{
    return "setge " + m_reg->get8BitLowName();
}

std::vector<std::shared_ptr<MachineIR>>& SetgeMIR::getOperands()
{
    return std::vector<std::shared_ptr<MachineIR>>{m_reg};
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
}

MIRType MovzxMIR::getMIRType() const
{
    return MIRType::Movzx;
}

std::string MovzxMIR::getString() const
{
    std::string strRepr = "movzx ";
    
    // Determine destination register name based on size
    if (m_toRegSize == 64)
    {
        strRepr += m_reg->getString(); // Full register name for 64-bit
    }
    else if (m_toRegSize == 32)
    {
        strRepr += m_reg->get32BitName();
    }
    else if (m_toRegSize == 16)
    {
        strRepr += m_reg->get16BitName();
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
            strRepr += m_reg->get8BitLowName();
        }
        else
        {
            strRepr += m_reg->get8BitHighName();
        }
    }
    else if (m_fromRegSize == 16)
    {
        strRepr += m_reg->get16BitName();
    }
    else if (m_fromRegSize == 32)
    {
        strRepr += m_reg->get32BitName();
    }
    else if (m_fromRegSize == 64)
    {
        strRepr += m_reg->getString();
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
    return std::vector<std::shared_ptr<MachineIR>>{m_reg};
}

// ==========================================
// TestMIR
// ==========================================

TestMIR::TestMIR(std::shared_ptr<Register> reg1, std::shared_ptr<Register> reg2)
    : m_reg1{std::move(reg1)}, m_reg2{std::move(reg2)}
{
}

MIRType TestMIR::getMIRType() const
{
    return MIRType::Test;
}

std::string TestMIR::getString() const
{
    return "test " + m_reg1->getString() + ", " + m_reg2->getString();
}

std::vector<std::shared_ptr<MachineIR>>& TestMIR::getOperands()
{
    return std::vector<std::shared_ptr<MachineIR>>{m_reg1, m_reg2};
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
