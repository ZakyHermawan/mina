#pragma once

#include <set>
#include <string>
#include <vector>
#include <memory>
#include <array>

#include <type_traits>
#include <concepts>

namespace mina
{

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
    Jnz,
    Ret
};

// Base Class
class MachineIR
{
protected:
    std::vector<std::shared_ptr<MachineIR>> m_operands;

public:
    virtual ~MachineIR() = default;

    virtual MIRType getMIRType() const;
    virtual std::string getString() const;
    virtual std::vector<std::shared_ptr<MachineIR>>& getOperands();
};

class BasicBlockMIR
{
private:
    std::string m_name;
    std::vector<std::shared_ptr<MachineIR>> m_instructions;
    std::vector<std::shared_ptr<BasicBlockMIR>> m_predecessors,
        m_successors;  // predecessors and successors

    std::set<int> m_def;
    std::set<int> m_use;
    std::set<int> m_liveIn;
    std::set<int> m_liveOut;

    int m_loopDepth = 0;

public:
    BasicBlockMIR(std::string name);
    std::string getName() const;
    std::vector<std::shared_ptr<MachineIR>>& getInstructions();
    void addInstruction(std::shared_ptr<MachineIR> inst);
    void printInstructions() const;

    void setSuccessorFromNames(
        const std::vector<std::string>& successorNames);
    void setPredecessorFromNames(
        const std::vector<std::string>& predecessorNames);

    std::vector<std::shared_ptr<BasicBlockMIR>>& getPredecessors();
    std::vector<std::shared_ptr<BasicBlockMIR>>& getSuccessors();

    void addDef(int regID);
    void addUse(int regID);
    void insertDef(int regID);
    void insertLiveIn(int regID);
    void insertLiveOut(int regID);

    std::set<int>& getDef();
    std::set<int>& getUse();
    std::set<int>& getLiveIn();
    std::set<int>& getLiveOut();

    //std::vector<Register> getDefRegisters() const;
    //std::vector<Register> getUseRegisters() const;
    //std::vector<Register> getLiveInRegisters() const;
    //std::vector<Register> getLiveOutRegisters() const;


    void generateDefUse();
    void printLivenessSets() const;

    void setLoopDepth(int depth);
    int getLoopDepth() const;
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

    std::shared_ptr<Register>& getBaseRegister();
};

class MovMIR : public MachineIR
{
    std::vector<std::shared_ptr<MachineIR>> m_operands;

public:
    MovMIR(std::vector<std::shared_ptr<MachineIR>> operands);

    MIRType getMIRType() const override;
    std::string getString() const override; 

    std::vector<std::shared_ptr<MachineIR>>& getOperands() override;
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

    std::vector<std::shared_ptr<MachineIR>>& getOperands() override;
    
    // Virtual destructor (good practice for inheritance)
    ~LeaMIR() override = default;
};

class CallMIR : public MachineIR
{
    std::string m_calleeName;
    unsigned int m_numArgs;

public:
    CallMIR(std::string calleeName, unsigned int numArgs);
    MIRType getMIRType() const override;
    std::string getString() const override;

    unsigned int getNumArgs() const;
};

class AddMIR : public MachineIR
{
    std::vector<std::shared_ptr<MachineIR>> m_operands;

public:
    AddMIR(std::vector<std::shared_ptr<MachineIR>> operands);
    MIRType getMIRType() const override;
    std::string getString() const override;
    std::vector<std::shared_ptr<MachineIR>>& getOperands() override;
};

class SubMIR : public MachineIR
{
    std::vector<std::shared_ptr<MachineIR>> m_operands;

public:
    SubMIR(std::vector<std::shared_ptr<MachineIR>> operands);
    MIRType getMIRType() const override;
    std::string getString() const override;
    std::vector<std::shared_ptr<MachineIR>>& getOperands() override;
};

class MulMIR : public MachineIR
{
    std::vector<std::shared_ptr<MachineIR>> m_operands;

public:
    MulMIR(std::vector<std::shared_ptr<MachineIR>> operands);
    MIRType getMIRType() const override;
    std::string getString() const override;
    std::vector<std::shared_ptr<MachineIR>>& getOperands() override;
};

class DivMIR : public MachineIR
{
    std::shared_ptr<MemoryMIR> m_divisor; // in a / b, b is divisor

public:
    DivMIR(std::shared_ptr<MemoryMIR> divisor);
    MIRType getMIRType() const override;
    std::string getString() const override;
    std::vector<std::shared_ptr<MachineIR>>& getOperands() override;
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
    std::vector<std::shared_ptr<MachineIR>>& getOperands() override;
};

class AndMIR : public MachineIR
{
    std::vector<std::shared_ptr<MachineIR>> m_operands;

public:
    AndMIR(std::vector<std::shared_ptr<MachineIR>> operands);
    MIRType getMIRType() const override;
    std::string getString() const override;
    std::vector<std::shared_ptr<MachineIR>>& getOperands() override;
};


class OrMIR : public MachineIR
{
    std::vector<std::shared_ptr<MachineIR>> m_operands;

public:
    OrMIR(std::vector<std::shared_ptr<MachineIR>> operands);
    MIRType getMIRType() const override;
    std::string getString() const override;
    std::vector<std::shared_ptr<MachineIR>>& getOperands() override;
};

class CmpMIR : public MachineIR
{
    std::vector<std::shared_ptr<MachineIR>> m_operands;

public:
    CmpMIR(std::vector<std::shared_ptr<MachineIR>> operands);
    MIRType getMIRType() const override;
    std::string getString() const override;
    std::vector<std::shared_ptr<MachineIR>>& getOperands() override;
};

class SeteMIR : public MachineIR
{
    std::shared_ptr<Register> m_reg;

public:
    SeteMIR(std::shared_ptr<Register> reg);
    MIRType getMIRType() const override;
    std::string getString() const override;
    std::vector<std::shared_ptr<MachineIR>>& getOperands() override;
};

class SetneMIR : public MachineIR
{
    std::shared_ptr<Register> m_reg;

public:
    SetneMIR(std::shared_ptr<Register> reg);
    MIRType getMIRType() const override;
    std::string getString() const override;
    std::vector<std::shared_ptr<MachineIR>>& getOperands() override;
};

class SetlMIR : public MachineIR
{
    std::shared_ptr<Register> m_reg;

public:
    SetlMIR(std::shared_ptr<Register> reg);
    MIRType getMIRType() const override;
    std::string getString() const override;
    std::vector<std::shared_ptr<MachineIR>>& getOperands() override;
};

class SetleMIR : public MachineIR
{
    std::shared_ptr<Register> m_reg;

public:
    SetleMIR(std::shared_ptr<Register> reg);
    MIRType getMIRType() const override;
    std::string getString() const override;
    std::vector<std::shared_ptr<MachineIR>>& getOperands() override;
};

class SetgMIR : public MachineIR
{
    std::shared_ptr<Register> m_reg;

public:
    SetgMIR(std::shared_ptr<Register> reg);
    MIRType getMIRType() const override;
    std::string getString() const override;
    std::vector<std::shared_ptr<MachineIR>>& getOperands() override;
};

class SetgeMIR : public MachineIR
{
    std::shared_ptr<Register> m_reg;

public:
    SetgeMIR(std::shared_ptr<Register> reg);
    MIRType getMIRType() const override;
    std::string getString() const override;
    std::vector<std::shared_ptr<MachineIR>>& getOperands() override;
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
    std::vector<std::shared_ptr<MachineIR>>& getOperands() override;
};

class TestMIR: public MachineIR
{
    std::shared_ptr<Register> m_reg1;
    std::shared_ptr<Register> m_reg2;

public:
    TestMIR(std::shared_ptr<Register> reg1, std::shared_ptr<Register> reg2);
    MIRType getMIRType() const override;
    std::string getString() const override;
    std::vector<std::shared_ptr<MachineIR>>& getOperands() override;
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

class RetMIR : public MachineIR
{
public:
    RetMIR();
    MIRType getMIRType() const override;
    std::string getString() const override;
};

// Register Identifiers
// Note: Keep in sync with registersFactory() in MachineIR.cpp
// Also keep in sync with calling convention usage in CodeGen.cpp
// RBP, RSP, RIP are reserved and should not be used for allocation
// R10 and R11 are caller-saved that will be used for instruction fixup,
// Do not include them in interference graph!
enum class RegID : int
{
    RAX, RBX, RCX, RDX, RDI, RSI, R8, R9, R12, R13, R14, RBP, RSP, RIP, R10, R11,
    COUNT 
};

template<typename T>
concept IsEnum = std::is_enum_v<T>;

template<IsEnum T>
constexpr auto to_int(T e) noexcept
{
    return static_cast<std::underlying_type_t<T>>(e);
}

const std::array<std::shared_ptr<Register>, to_int(RegID::COUNT)>& getAllRegisters();
const std::shared_ptr<Register>& getReg(RegID id);

}; // namespace mina
