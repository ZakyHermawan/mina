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
    Call,
    Add,
    Sub,
    Mul,
    Div,
    Cqo,
    Not,
    And,
    Or,
    Jmp,
    Cmp,
    Sete,
    Setne,
    Setl,
    Setle,
    Setg,
    Setge,
    Movzx,
    Test,
    Jz,
    Jnz
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
    std::string m_32BitName;
    std::string m_16BitName;
    std::string m_8BitHighName;
    std::string m_8BitLowName;

public:
    Register(unsigned int id, std::string name, std::string _32bitName,
             std::string _16BitName, std::string _8BitHighName,
             std::string _8BitLowName);
    Register(unsigned int id, std::string name);
    Register(unsigned int id);

    MIRType getMIRType() const override;
    std::string getString() const override;
    unsigned int getID() const;

    std::string get64BitName() const;
    std::string get32BitName() const;
    std::string get16BitName() const;
    std::string get8BitHighName() const;
    std::string get8BitLowName() const;
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

    std::vector<std::shared_ptr<MachineIR>>& getOperands();
    
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

class AddMIR : public MachineIR
{
    std::vector<std::shared_ptr<MachineIR>> m_operands;

public:
    AddMIR(std::vector<std::shared_ptr<MachineIR>> operands);
    MIRType getMIRType() const override;
    std::string getString() const override;
    std::vector<std::shared_ptr<MachineIR>>& getOperands();
};

class SubMIR : public MachineIR
{
    std::vector<std::shared_ptr<MachineIR>> m_operands;

public:
    SubMIR(std::vector<std::shared_ptr<MachineIR>> operands);
    MIRType getMIRType() const override;
    std::string getString() const override;
    std::vector<std::shared_ptr<MachineIR>>& getOperands();
};

class MulMIR : public MachineIR
{
    std::vector<std::shared_ptr<MachineIR>> m_operands;

public:
    MulMIR(std::vector<std::shared_ptr<MachineIR>> operands);
    MIRType getMIRType() const override;
    std::string getString() const override;
    std::vector<std::shared_ptr<MachineIR>>& getOperands();
};

class DivMIR : public MachineIR
{
    std::shared_ptr<MemoryMIR> m_divisor; // in a / b, b is divisor

public:
    DivMIR(std::shared_ptr<MemoryMIR> divisor);
    MIRType getMIRType() const override;
    std::string getString() const override;
    std::vector<std::shared_ptr<MachineIR>>& getOperands();
};

class CqoMIR : public MachineIR
{
public:
    CqoMIR();
    MIRType getMIRType() const override;
    std::string getString() const override;
};

// To implement logical NOT, but using XOR instruction
class NotMIR : public MachineIR
{
    std::shared_ptr<Register> m_operand;

public:
    NotMIR(std::shared_ptr<Register> operand);
    MIRType getMIRType() const override;
    std::string getString() const override;
    std::vector<std::shared_ptr<MachineIR>>& getOperands();
};

class AndMIR : public MachineIR
{
    std::vector<std::shared_ptr<MachineIR>> m_operands;

public:
    AndMIR(std::vector<std::shared_ptr<MachineIR>> operands);
    MIRType getMIRType() const override;
    std::string getString() const override;
    std::vector<std::shared_ptr<MachineIR>>& getOperands();
};


class OrMIR : public MachineIR
{
    std::vector<std::shared_ptr<MachineIR>> m_operands;

public:
    OrMIR(std::vector<std::shared_ptr<MachineIR>> operands);
    MIRType getMIRType() const override;
    std::string getString() const override;
    std::vector<std::shared_ptr<MachineIR>>& getOperands();
};

class CmpMIR : public MachineIR
{
    std::vector<std::shared_ptr<MachineIR>> m_operands;

 public:
    CmpMIR(std::vector<std::shared_ptr<MachineIR>> operands);
    MIRType getMIRType() const override;
    std::string getString() const override;
    std::vector<std::shared_ptr<MachineIR>>& getOperands();
};

class SeteMIR : public MachineIR
{
    std::shared_ptr<Register> m_reg;

public:
    SeteMIR(std::shared_ptr<Register> reg);
    MIRType getMIRType() const override;
    std::string getString() const override;
    std::vector<std::shared_ptr<MachineIR>>& getOperands();
};

class SetneMIR : public MachineIR
{
    std::shared_ptr<Register> m_reg;

public:
    SetneMIR(std::shared_ptr<Register> reg);
    MIRType getMIRType() const override;
    std::string getString() const override;
    std::vector<std::shared_ptr<MachineIR>>& getOperands();
};

class SetlMIR : public MachineIR
{
    std::shared_ptr<Register> m_reg;

public:
    SetlMIR(std::shared_ptr<Register> reg);
    MIRType getMIRType() const override;
    std::string getString() const override;
    std::vector<std::shared_ptr<MachineIR>>& getOperands();
};

class SetleMIR : public MachineIR
{
    std::shared_ptr<Register> m_reg;

public:
    SetleMIR(std::shared_ptr<Register> reg);
    MIRType getMIRType() const override;
    std::string getString() const override;
    std::vector<std::shared_ptr<MachineIR>>& getOperands();
};

class SetgMIR : public MachineIR
{
    std::shared_ptr<Register> m_reg;

public:
    SetgMIR(std::shared_ptr<Register> reg);
    MIRType getMIRType() const override;
    std::string getString() const override;
    std::vector<std::shared_ptr<MachineIR>>& getOperands();
};

class SetgeMIR : public MachineIR
{
    std::shared_ptr<Register> m_reg;

public:
    SetgeMIR(std::shared_ptr<Register> reg);
    MIRType getMIRType() const override;
    std::string getString() const override;
    std::vector<std::shared_ptr<MachineIR>>& getOperands();
};

class MovzxMIR : public MachineIR
{
    std::shared_ptr<Register> m_reg;
    unsigned int m_toRegSize;
    unsigned int m_fromRegSize;
    unsigned int m_isFromRegLow;

public:
    MovzxMIR(std::shared_ptr<Register> reg, unsigned int toRegSize, unsigned int fromRegSize, bool isFromRegLow);
    MIRType getMIRType() const override;
    std::string getString() const override;
    std::vector<std::shared_ptr<MachineIR>>& getOperands();
};

class TestMIR: public MachineIR
{
    std::shared_ptr<Register> m_reg1;
    std::shared_ptr<Register> m_reg2;

public:
    TestMIR(std::shared_ptr<Register> reg1, std::shared_ptr<Register> reg2);
    MIRType getMIRType() const override;
    std::string getString() const override;
    std::vector<std::shared_ptr<MachineIR>>& getOperands();
};

class JmpMIR : public MachineIR
{
    std::string m_targetLabel;

 public:
    JmpMIR(std::string targetLabel);
    MIRType getMIRType() const override;
    std::string getString() const override;
    std::string getTargetLabel() const;
};

class JzMIR : public MachineIR
{
    std::string m_targetLabel;

public:
    JzMIR(std::string targetLabel);
    MIRType getMIRType() const override;
    std::string getString() const override;
    std::string getTargetLabel() const;
};

class JnzMIR : public MachineIR
{
    std::string m_targetLabel;

 public:
    JnzMIR(std::string targetLabel);
    MIRType getMIRType() const override;
    std::string getString() const override;
    std::string getTargetLabel() const;
};
