#pragma once

#include <string>
#include <vector>
#include <memory>

// Enum declaration remains in the header
enum class MIRType
{
    None,
    Reg,
    Const,
    Memory,
    Literal,
    Mov,
    Lea,
    Call
};

// Base Class
class MachineIR
{
public:
    virtual ~MachineIR() = default;
    
    virtual MIRType getMIRType() const;
    virtual std::string getString() const;
};

class BasicBlockMIR
{
private:
    std::string m_name;
    std::vector<std::shared_ptr<MachineIR>> m_instructions;

public:
    BasicBlockMIR(std::string name);
    std::string getName() const;
    std::vector<std::shared_ptr<MachineIR>>& getInstructions();
    void addInstruction(std::shared_ptr<MachineIR> inst);
    void printInstructions() const;
};

class Register : public MachineIR
{
    unsigned int m_id;
    std::string m_regName;

public:
    Register(unsigned int id, std::string name);
    Register(unsigned int id);

    MIRType getMIRType() const override;
    std::string getString() const override;
    unsigned int getID() const;
};

// Literal Class, to emit operand as is
class LiteralMIR : public MachineIR
{
    std::string m_literal;

public:
    LiteralMIR(std::string literal);

    MIRType getMIRType() const override;
    std::string getString() const override;
};

class ConstMIR : public MachineIR
{
    int m_const;

public:
    ConstMIR(int constant);

    MIRType getMIRType() const override;
    std::string getString() const override;
    int getConst() const;
};

class MemoryMIR : public MachineIR
{
    std::shared_ptr<Register> m_reg;
    std::shared_ptr<ConstMIR> m_offset;
    std::shared_ptr<LiteralMIR> m_literal;

public:
    MemoryMIR(std::shared_ptr<Register> reg, int offset);
    MemoryMIR(std::shared_ptr<Register> reg, std::shared_ptr<ConstMIR> offset);
    MemoryMIR(std::shared_ptr<Register> reg, std::shared_ptr<LiteralMIR> literal);

    MIRType getMIRType() const override;
    std::string getString() const override;
};

class MovMIR : public MachineIR
{
    std::vector<std::shared_ptr<MachineIR>> m_operands;

public:
    MovMIR(std::vector<std::shared_ptr<MachineIR>> operands);

    MIRType getMIRType() const override;
    std::string getString() const override; 
    
    std::vector<std::shared_ptr<MachineIR>>& getOperands();
};

class LeaMIR : public MachineIR
{
    std::vector<std::shared_ptr<MachineIR>> m_operands;

public:
    // Constructor
    explicit LeaMIR(std::vector<std::shared_ptr<MachineIR>> operands);

    // Override methods
    MIRType getMIRType() const override;
    std::string getString() const override;
    
    // Virtual destructor (good practice for inheritance)
    ~LeaMIR() override = default;
};

class CallMIR : public MachineIR
{
    std::string m_calleeName;
public:
    CallMIR(std::string calleeName);
    MIRType getMIRType() const override;
    std::string getString() const override; 
};
