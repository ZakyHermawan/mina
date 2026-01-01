#include "MachineIR.hpp"
#include <utility>
#include <stdexcept>
#include <string>
#include <iostream>
#include <vector>
#include <sstream>

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
