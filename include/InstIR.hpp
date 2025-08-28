#pragma once
#include <memory>
#include <string>
#include <vector>
#include <iostream>

#include "Types.hpp"
#include "BasicBlock.hpp"

class BasicBlock;

enum class InstType
{

    IntConst,
    BoolConst,
    StrConst,
    Ident,
    Add,
    Sub,
    Mul,
    Div,
    Not,
    And,
    Or,
    Alloca,
    ArrAccess,
    ArrUpdate,
    Assign,
    CmpEq,
    CmpNE,
    CmpLT,
    CmpLTE,
    CmpGT,
    CmpGTE,
    Jump,
    BRT,
    BRF,
    Put,
    Get,
    Push,
    Pop,
    Return,
    FuncSignature,
    Call,
    Phi,
    Undef,
    Noop,

    Undefined
};

class Inst : public std::enable_shared_from_this<Inst>
{
protected:
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;

public:
    Inst() {};
    virtual std::string getString() { return ""; };
    virtual std::shared_ptr<Inst> getTarget() { return shared_from_this(); }
    virtual void setTarget(std::shared_ptr<Inst> target) { }
    virtual void push_user(std::shared_ptr<Inst> user) {};
    virtual void setup_def_use() {}
    virtual bool isPhi() { return false; }
    virtual std::vector<std::shared_ptr<Inst>>& getOperands() { return m_operands; }
    virtual void setOperands(std::vector<std::shared_ptr<Inst>>& operands) { m_operands = operands; }
    virtual bool canBeRenamed() const { return true; }
    virtual std::shared_ptr<BasicBlock> getBlock() { return nullptr; };
    virtual InstType getInstType() const { return InstType::Undefined; }
};

class IntConstInst : public Inst
{
private:
    int m_val;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;
    std::shared_ptr<BasicBlock> m_block;

public:
    IntConstInst(int val, std::shared_ptr<BasicBlock> block);

    virtual ~IntConstInst() = default;
    IntConstInst(const IntConstInst&) = delete;
    IntConstInst(IntConstInst&&) noexcept = default;
    IntConstInst& operator=(const IntConstInst&) = delete;
    IntConstInst& operator=(IntConstInst&&) noexcept = default;

    int getVal();
    virtual std::string getString() override;
    virtual void push_user(std::shared_ptr<Inst> user) override;
    virtual void setup_def_use();
    virtual std::vector<std::shared_ptr<Inst>>& getOperands() override;
    virtual bool canBeRenamed() const override;
    virtual std::shared_ptr<BasicBlock> getBlock() override;
    virtual InstType getInstType() const override;
};

class BoolConstInst : public Inst
{
private:
    bool m_val;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;
    std::shared_ptr<BasicBlock> m_block;

public:
    BoolConstInst(bool val, std::shared_ptr<BasicBlock> block);

    virtual ~BoolConstInst() = default;
    BoolConstInst(const BoolConstInst&) = delete;
    BoolConstInst(BoolConstInst&&) noexcept = default;
    BoolConstInst& operator=(const BoolConstInst&) = delete;
    BoolConstInst& operator=(BoolConstInst&&) noexcept = default;

    bool getVal();
    virtual std::string getString() override;
    virtual void push_user(std::shared_ptr<Inst> user) override;
    virtual void setup_def_use();
    virtual std::vector<std::shared_ptr<Inst>>& getOperands() override;
    virtual bool canBeRenamed() const override;
    virtual std::shared_ptr<BasicBlock> getBlock() override;
    virtual InstType getInstType() const override;
};

class StrConstInst : public Inst
{
private:
    std::string m_val;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;
    std::shared_ptr<BasicBlock> m_block;

public:
    StrConstInst(std::string val, std::shared_ptr<BasicBlock> block);

    virtual ~StrConstInst() = default;
    StrConstInst(const StrConstInst&) = delete;
    StrConstInst(StrConstInst&&) noexcept = default;
    StrConstInst& operator=(const StrConstInst&) = delete;
    StrConstInst& operator=(StrConstInst&&) noexcept = default;

    virtual std::string getString() override;
    virtual void push_user(std::shared_ptr<Inst> user) override;
    virtual void setup_def_use();
    virtual std::vector<std::shared_ptr<Inst>>& getOperands() override;
    virtual bool canBeRenamed() const override;
    virtual std::shared_ptr<BasicBlock> getBlock() override;
    virtual InstType getInstType() const override;
};

class IdentInst : public Inst
{
private:
    std::string m_name;
    std::vector<std::shared_ptr<Inst>> m_users;
    std::shared_ptr<BasicBlock> m_block;

public:
    IdentInst(std::string name, std::shared_ptr<BasicBlock> block);

    virtual ~IdentInst() = default;
    IdentInst(const IdentInst&) = delete;
    IdentInst(IdentInst&&) noexcept = default;
    IdentInst& operator=(const IdentInst&) = delete;
    IdentInst& operator=(IdentInst&&) noexcept = default;

    virtual std::string getString() override;
    virtual void push_user(std::shared_ptr<Inst> user) override;
    virtual void setup_def_use();
    virtual std::vector<std::shared_ptr<Inst>>& getOperands() override;
    virtual std::shared_ptr<BasicBlock> getBlock() override;
    virtual InstType getInstType() const override;
};

class AddInst: public Inst
{
private:
    std::shared_ptr<Inst> m_target, m_operand1, m_operand2;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;
    std::shared_ptr<BasicBlock> m_block;

public:
    AddInst(std::shared_ptr<Inst> target, std::shared_ptr<Inst> operand1,
            std::shared_ptr<Inst> operand2, std::shared_ptr<BasicBlock> block);

    virtual ~AddInst() = default;
    AddInst(const AddInst&) = delete;
    AddInst(AddInst&&) noexcept = default;
    AddInst& operator=(const AddInst&) = delete;
    AddInst& operator=(AddInst&&) noexcept = default;

    std::shared_ptr<Inst> getTarget() override;
    std::shared_ptr<Inst> getOperand1();
    std::shared_ptr<Inst> getOperand2();

    virtual std::string getString() override;
    virtual void push_user(std::shared_ptr<Inst> user) override;
    virtual void setup_def_use();
    virtual std::vector<std::shared_ptr<Inst>>& getOperands() override;
    virtual void setTarget(std::shared_ptr<Inst> target);
    virtual std::shared_ptr<BasicBlock> getBlock() override;
    virtual InstType getInstType() const override;
};

class SubInst: public Inst
{
private:
    std::shared_ptr<Inst> m_target, m_operand1, m_operand2;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;
    std::shared_ptr<BasicBlock> m_block;

public:
    SubInst(std::shared_ptr<Inst> target, std::shared_ptr<Inst> operand1,
            std::shared_ptr<Inst> operand2, std::shared_ptr<BasicBlock> block);

    virtual ~SubInst() = default;
    SubInst(const SubInst&) = delete;
    SubInst(SubInst&&) noexcept = default;
    SubInst& operator=(const SubInst&) = delete;
    SubInst& operator=(SubInst&&) noexcept = default;

    std::shared_ptr<Inst> getTarget() override;
    std::shared_ptr<Inst> getOperand1();
    std::shared_ptr<Inst> getOperand2();

    virtual std::string getString() override;
    virtual void push_user(std::shared_ptr<Inst> user) override;
    virtual void setup_def_use();
    virtual std::vector<std::shared_ptr<Inst>>& getOperands() override;
    virtual void setTarget(std::shared_ptr<Inst> target);
    virtual std::shared_ptr<BasicBlock> getBlock() override;
    virtual InstType getInstType() const override;
};

class MulInst: public Inst
{
private:
    std::shared_ptr<Inst> m_target, m_operand1, m_operand2;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;
    std::shared_ptr<BasicBlock> m_block;

public:
    MulInst(std::shared_ptr<Inst> target, std::shared_ptr<Inst> operand1,
            std::shared_ptr<Inst> operand2, std::shared_ptr<BasicBlock> block);

    virtual ~MulInst() = default;
    MulInst(const MulInst&) = delete;
    MulInst(MulInst&&) noexcept = default;
    MulInst& operator=(const MulInst&) = delete;
    MulInst& operator=(MulInst&&) noexcept = default;

    std::shared_ptr<Inst> getTarget() override;
    std::shared_ptr<Inst> getOperand1();
    std::shared_ptr<Inst> getOperand2();

    virtual std::string getString() override;
    virtual void push_user(std::shared_ptr<Inst> user) override;
    virtual void setup_def_use();
    virtual std::vector<std::shared_ptr<Inst>>& getOperands() override;
    virtual void setTarget(std::shared_ptr<Inst> target);
    virtual std::shared_ptr<BasicBlock> getBlock() override;
    virtual InstType getInstType() const override;
};

class DivInst: public Inst
{
private:
    std::shared_ptr<Inst> m_target, m_operand1, m_operand2;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;
    std::shared_ptr<BasicBlock> m_block;

public:
    DivInst(std::shared_ptr<Inst> target, std::shared_ptr<Inst> operand1,
            std::shared_ptr<Inst> operand2, std::shared_ptr<BasicBlock> block);

    virtual ~DivInst() = default;
    DivInst(const DivInst&) = delete;
    DivInst(DivInst&&) noexcept = default;
    DivInst& operator=(const DivInst&) = delete;
    DivInst& operator=(DivInst&&) noexcept = default;

    std::shared_ptr<Inst> getTarget() override;
    std::shared_ptr<Inst> getOperand1();
    std::shared_ptr<Inst> getOperand2();

    virtual std::string getString() override;
    virtual void push_user(std::shared_ptr<Inst> user) override;
    virtual void setup_def_use();
    virtual std::vector<std::shared_ptr<Inst>>& getOperands() override;
    virtual void setTarget(std::shared_ptr<Inst> target);
    virtual std::shared_ptr<BasicBlock> getBlock() override;
    virtual InstType getInstType() const override;
};

class NotInst: public Inst
{
private:
    std::shared_ptr<Inst> m_target, m_operand;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;
    std::shared_ptr<BasicBlock> m_block;

public:
    NotInst(std::shared_ptr<Inst> target, std::shared_ptr<Inst> operand,
            std::shared_ptr<BasicBlock> block);

    virtual ~NotInst() = default;
    NotInst(const NotInst&) = delete;
    NotInst(NotInst&&) noexcept = default;
    NotInst& operator=(const NotInst&) = delete;
    NotInst& operator=(NotInst&&) noexcept = default;

    std::shared_ptr<Inst> getTarget() override;
    std::shared_ptr<Inst> getOperand();

    virtual std::string getString() override;
    virtual void push_user(std::shared_ptr<Inst> user) override;
    virtual void setup_def_use();
    virtual std::vector<std::shared_ptr<Inst>>& getOperands() override;
    virtual void setTarget(std::shared_ptr<Inst> target);
    virtual std::shared_ptr<BasicBlock> getBlock() override;
    virtual InstType getInstType() const override;
};

class AndInst: public Inst
{
private:
    std::shared_ptr<Inst> m_target, m_operand1, m_operand2;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;
    std::shared_ptr<BasicBlock> m_block;

public:
    AndInst(std::shared_ptr<Inst> target, std::shared_ptr<Inst> operand1,
            std::shared_ptr<Inst> operand2, std::shared_ptr<BasicBlock> block);

    virtual ~AndInst() = default;
    AndInst(const AndInst&) = delete;
    AndInst(AndInst&&) noexcept = default;
    AndInst& operator=(const AndInst&) = delete;
    AndInst& operator=(AndInst&&) noexcept = default;

    std::shared_ptr<Inst> getTarget() override;
    std::shared_ptr<Inst> getOperand1();
    std::shared_ptr<Inst> getOperand2();

    virtual std::string getString() override;
    virtual void push_user(std::shared_ptr<Inst> user) override;
    virtual void setup_def_use();
    virtual std::vector<std::shared_ptr<Inst>>& getOperands() override;
    virtual void setTarget(std::shared_ptr<Inst> target);
    virtual std::shared_ptr<BasicBlock> getBlock() override;
    virtual InstType getInstType() const override;
};

class OrInst: public Inst
{
private:
    std::shared_ptr<Inst> m_target, m_operand1, m_operand2;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;
    std::shared_ptr<BasicBlock> m_block;

public:
    OrInst(std::shared_ptr<Inst> target, std::shared_ptr<Inst> operand1,
           std::shared_ptr<Inst> operand2, std::shared_ptr<BasicBlock> block);

    virtual ~OrInst() = default;
    OrInst(const OrInst&) = delete;
    OrInst(OrInst&&) noexcept = default;
    OrInst& operator=(const OrInst&) = delete;
    OrInst& operator=(OrInst&&) noexcept = default;

    std::shared_ptr<Inst> getTarget() override;
    std::shared_ptr<Inst> getOperand1();
    std::shared_ptr<Inst> getOperand2();

    virtual std::string getString() override;
    virtual void push_user(std::shared_ptr<Inst> user) override;
    virtual void setup_def_use();
    virtual std::vector<std::shared_ptr<Inst>>& getOperands() override;
    virtual void setTarget(std::shared_ptr<Inst> target);
    virtual std::shared_ptr<BasicBlock> getBlock() override;
    virtual InstType getInstType() const override;
};

class AllocaInst : public Inst
{
private:
    std::shared_ptr<Inst> m_target;
    Type m_type;
    unsigned int m_size;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;
    std::shared_ptr<BasicBlock> m_block;

public:
    AllocaInst(std::shared_ptr<Inst> target, Type type, unsigned int size,
               std::shared_ptr<BasicBlock> block);

    virtual ~AllocaInst() = default;
    AllocaInst(const AllocaInst&) = delete;
    AllocaInst(AllocaInst&&) noexcept = default;
    AllocaInst& operator=(const AllocaInst&) = delete;
    AllocaInst& operator=(AllocaInst&&) noexcept = default;
    
    std::shared_ptr<Inst> getTarget() override;
    Type getType();
    unsigned int getSize();
    
    virtual std::string getString() override;
    virtual void push_user(std::shared_ptr<Inst> user) override;
    virtual void setup_def_use();
    virtual std::vector<std::shared_ptr<Inst>>& getOperands() override;
    virtual void setTarget(std::shared_ptr<Inst> target);
    virtual std::shared_ptr<BasicBlock> getBlock() override;
    virtual InstType getInstType() const override;
};

class ArrAccessInst : public Inst
{
private:
    std::shared_ptr<Inst> m_target, m_source, m_index;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;
    std::shared_ptr<BasicBlock> m_block;
    Type m_type;

public:
    ArrAccessInst(std::shared_ptr<Inst> target, std::shared_ptr<Inst> source,
                  std::shared_ptr<Inst> index, std::shared_ptr<BasicBlock> block, Type type);
    
    virtual ~ArrAccessInst() = default;
    ArrAccessInst(const ArrAccessInst&) = delete;
    ArrAccessInst(ArrAccessInst&&) noexcept = default;
    ArrAccessInst& operator=(const ArrAccessInst&) = delete;
    ArrAccessInst& operator=(ArrAccessInst&&) noexcept = default;
    
    std::shared_ptr<Inst> getTarget() override;
    std::shared_ptr<Inst> getSource();
    std::shared_ptr<Inst> getIndex();
    Type getType() { return m_type; }
    
    virtual std::string getString() override;
    virtual void push_user(std::shared_ptr<Inst> user) override;
    virtual void setup_def_use();
    virtual std::vector<std::shared_ptr<Inst>>& getOperands() override;
    virtual void setTarget(std::shared_ptr<Inst> target);
    virtual std::shared_ptr<BasicBlock> getBlock() override;
    virtual InstType getInstType() const override;
};

class ArrUpdateInst : public Inst
{
private:
    std::shared_ptr<Inst> m_target, m_source, m_index, m_val;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;
    std::shared_ptr<BasicBlock> m_block;
    Type m_type;

public:
    ArrUpdateInst(std::shared_ptr<Inst> target, std::shared_ptr<Inst> source,
                  std::shared_ptr<Inst> index, std::shared_ptr<Inst> val, std::shared_ptr<BasicBlock> block, Type type);

    virtual ~ArrUpdateInst() = default;
    ArrUpdateInst(const ArrUpdateInst&) = delete;
    ArrUpdateInst(ArrUpdateInst&&) noexcept = default;
    ArrUpdateInst& operator=(const ArrUpdateInst&) = delete;
    ArrUpdateInst& operator=(ArrUpdateInst&&) noexcept = default;
    
    std::shared_ptr<Inst> getTarget() override;
    std::shared_ptr<Inst> getSource();
    std::shared_ptr<Inst> getIndex();
    std::shared_ptr<Inst> getVal();
    Type getType() const { return m_type; }
    
    virtual std::string getString() override;
    virtual void push_user(std::shared_ptr<Inst> user) override;
    virtual void setup_def_use();
    virtual std::vector<std::shared_ptr<Inst>>& getOperands() override;
    virtual void setTarget(std::shared_ptr<Inst> target);
    virtual std::shared_ptr<BasicBlock> getBlock() override;
    virtual InstType getInstType() const override;
};

class AssignInst : public Inst
{
private:
    std::shared_ptr<Inst> m_target, m_source;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;
    std::shared_ptr<BasicBlock> m_block;

public:
    AssignInst(std::shared_ptr<Inst> target, std::shared_ptr<Inst> source,
               std::shared_ptr<BasicBlock> block);
  
    virtual ~AssignInst() = default;
    AssignInst(const AssignInst&) = delete;
    AssignInst(AssignInst&&) noexcept = default;
    AssignInst& operator=(const AssignInst&) = delete;
    AssignInst& operator=(AssignInst&&) noexcept = default;
  
    std::shared_ptr<Inst> getTarget() override;
    std::shared_ptr<Inst> getSource();
  
    virtual std::string getString() override;
    virtual void push_user(std::shared_ptr<Inst> user) override;
    virtual void setup_def_use();
    virtual std::vector<std::shared_ptr<Inst>>& getOperands() override;
    virtual void setTarget(std::shared_ptr<Inst> target);
    virtual std::shared_ptr<BasicBlock> getBlock() override;
    virtual InstType getInstType() const override;
};

class CmpEQInst : public Inst
{
private:
    std::shared_ptr<Inst> m_target, m_operand1, m_operand2;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;
    std::shared_ptr<BasicBlock> m_block;

public:
    CmpEQInst(std::shared_ptr<Inst> target, std::shared_ptr<Inst> operand1,
              std::shared_ptr<Inst> operand2, std::shared_ptr<BasicBlock> block);
  
    virtual ~CmpEQInst() = default;
    CmpEQInst(const CmpEQInst&) = delete;
    CmpEQInst(CmpEQInst&&) noexcept = default;
    CmpEQInst& operator=(const CmpEQInst&) = delete;
    CmpEQInst& operator=(CmpEQInst&&) noexcept = default;
  
    std::shared_ptr<Inst> getTarget() override;
    std::shared_ptr<Inst> getOperand1();
    std::shared_ptr<Inst> getOperand2();
  
    virtual std::string getString() override;
    virtual void push_user(std::shared_ptr<Inst> user) override;
    virtual void setup_def_use();
    virtual std::vector<std::shared_ptr<Inst>>& getOperands() override;
    virtual void setTarget(std::shared_ptr<Inst> target);
    virtual std::shared_ptr<BasicBlock> getBlock() override;
    virtual InstType getInstType() const override;
};

class CmpNEInst : public Inst
{
private:
    std::shared_ptr<Inst> m_target, m_operand1, m_operand2;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;
    std::shared_ptr<BasicBlock> m_block;

public:
    CmpNEInst(std::shared_ptr<Inst> target, std::shared_ptr<Inst> operand1,
              std::shared_ptr<Inst> operand2, std::shared_ptr<BasicBlock> block);
  
    virtual ~CmpNEInst() = default;
    CmpNEInst(const CmpNEInst&) = delete;
    CmpNEInst(CmpNEInst&&) noexcept = default;
    CmpNEInst& operator=(const CmpNEInst&) = delete;
    CmpNEInst& operator=(CmpNEInst&&) noexcept = default;
  
    std::shared_ptr<Inst> getTarget() override;
    std::shared_ptr<Inst> getOperand1();
    std::shared_ptr<Inst> getOperand2();
  
    virtual std::string getString() override;
    virtual void push_user(std::shared_ptr<Inst> user) override;
    virtual void setup_def_use();
    virtual std::vector<std::shared_ptr<Inst>>& getOperands() override;
    virtual void setTarget(std::shared_ptr<Inst> target);
    virtual std::shared_ptr<BasicBlock> getBlock() override;
    virtual InstType getInstType() const override;
};

class CmpLTInst : public Inst
{
private:
    std::shared_ptr<Inst> m_target, m_operand1, m_operand2;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;
    std::shared_ptr<BasicBlock> m_block;

public:
    CmpLTInst(std::shared_ptr<Inst> target, std::shared_ptr<Inst> operand1,
              std::shared_ptr<Inst> operand2, std::shared_ptr<BasicBlock> block);
  
    virtual ~CmpLTInst() = default;
    CmpLTInst(const CmpLTInst&) = delete;
    CmpLTInst(CmpLTInst&&) noexcept = default;
    CmpLTInst& operator=(const CmpLTInst&) = delete;
    CmpLTInst& operator=(CmpLTInst&&) noexcept = default;
  
    std::shared_ptr<Inst> getTarget() override;
    std::shared_ptr<Inst> getOperand1();
    std::shared_ptr<Inst> getOperand2();
  
    virtual std::string getString() override;
    virtual void push_user(std::shared_ptr<Inst> user) override;
    virtual void setup_def_use();
    virtual std::vector<std::shared_ptr<Inst>>& getOperands() override;
    virtual void setTarget(std::shared_ptr<Inst> target);
    virtual std::shared_ptr<BasicBlock> getBlock() override;
    virtual InstType getInstType() const override;
};

class CmpLTEInst : public Inst
{
private:
    std::shared_ptr<Inst> m_target, m_operand1, m_operand2;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;
    std::shared_ptr<BasicBlock> m_block;

public:
    CmpLTEInst(std::shared_ptr<Inst> target, std::shared_ptr<Inst> operand1,
               std::shared_ptr<Inst> operand2, std::shared_ptr<BasicBlock> block);

    virtual ~CmpLTEInst() = default;
    CmpLTEInst(const CmpLTEInst&) = delete;
    CmpLTEInst(CmpLTEInst&&) noexcept = default;
    CmpLTEInst& operator=(const CmpLTEInst&) = delete;
    CmpLTEInst& operator=(CmpLTEInst&&) noexcept = default;
    
    std::shared_ptr<Inst> getTarget() override;
    std::shared_ptr<Inst> getOperand1();
    std::shared_ptr<Inst> getOperand2();
    
    virtual std::string getString() override;
    virtual void push_user(std::shared_ptr<Inst> user) override;
    virtual void setup_def_use();
    virtual std::vector<std::shared_ptr<Inst>>& getOperands() override;
    virtual void setTarget(std::shared_ptr<Inst> target);
    virtual std::shared_ptr<BasicBlock> getBlock() override;
    virtual InstType getInstType() const override;
};

class CmpGTInst : public Inst
{
private:
    std::shared_ptr<Inst> m_target, m_operand1, m_operand2;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;
    std::shared_ptr<BasicBlock> m_block;

public:
    CmpGTInst(std::shared_ptr<Inst> target, std::shared_ptr<Inst> operand1,
              std::shared_ptr<Inst> operand2, std::shared_ptr<BasicBlock> block);

    virtual ~CmpGTInst() = default;
    CmpGTInst(const CmpGTInst&) = delete;
    CmpGTInst(CmpGTInst&&) noexcept = default;
    CmpGTInst& operator=(const CmpGTInst&) = delete;
    CmpGTInst& operator=(CmpGTInst&&) noexcept = default;
    
    std::shared_ptr<Inst> getTarget() override;
    std::shared_ptr<Inst> getOperand1();
    std::shared_ptr<Inst> getOperand2();
    
    virtual std::string getString() override;
    virtual void push_user(std::shared_ptr<Inst> user) override;
    virtual void setup_def_use();
    virtual std::vector<std::shared_ptr<Inst>>& getOperands() override;
    virtual void setTarget(std::shared_ptr<Inst> target);
    virtual std::shared_ptr<BasicBlock> getBlock() override;
    virtual InstType getInstType() const override;
};

class CmpGTEInst : public Inst
{
private:
    std::shared_ptr<Inst> m_target, m_operand1, m_operand2;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;
    std::shared_ptr<BasicBlock> m_block;

public:
    CmpGTEInst(std::shared_ptr<Inst> target, std::shared_ptr<Inst> operand1,
               std::shared_ptr<Inst> operand2, std::shared_ptr<BasicBlock> block);
  
    virtual ~CmpGTEInst() = default;
    CmpGTEInst(const CmpGTEInst&) = delete;
    CmpGTEInst(CmpGTEInst&&) noexcept = default;
    CmpGTEInst& operator=(const CmpGTEInst&) = delete;
    CmpGTEInst& operator=(CmpGTEInst&&) noexcept = default;
  
    std::shared_ptr<Inst> getTarget() override;
    std::shared_ptr<Inst> getOperand1();
    std::shared_ptr<Inst> getOperand2();
  
    virtual std::string getString() override;
    virtual void push_user(std::shared_ptr<Inst> user) override;
    virtual void setup_def_use();
    virtual std::vector<std::shared_ptr<Inst>>& getOperands() override;
    virtual void setTarget(std::shared_ptr<Inst> target);
    virtual std::shared_ptr<BasicBlock> getBlock() override;
    virtual InstType getInstType() const override;
};

class JumpInst : public Inst
{
private:
    std::shared_ptr<BasicBlock> m_target;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;
    std::shared_ptr<BasicBlock> m_block;

public:
    JumpInst(std::shared_ptr<BasicBlock> target);
  
    virtual ~JumpInst() = default;
    JumpInst(const JumpInst&) = delete;
    JumpInst(JumpInst&&) noexcept = default;
    JumpInst& operator=(const JumpInst&) = delete;
    JumpInst& operator=(JumpInst&&) noexcept = default;
  
    std::shared_ptr<BasicBlock> getJumpTarget();
  
    virtual std::string getString() override;
    virtual void push_user(std::shared_ptr<Inst> user) override;
    virtual void setup_def_use();
    virtual std::vector<std::shared_ptr<Inst>>& getOperands() override;
    virtual std::shared_ptr<BasicBlock> getBlock() override;
    virtual InstType getInstType() const override;
};

class BRTInst : public Inst
{
private:
    std::shared_ptr<BasicBlock> m_targetSuccess;
    std::shared_ptr<BasicBlock> m_targetFailed;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;
    std::shared_ptr<BasicBlock> m_block;

public:
    BRTInst(std::shared_ptr<Inst> cond, std::shared_ptr<BasicBlock> targetSuccess,
            std::shared_ptr<BasicBlock> targetFailed, std::shared_ptr<BasicBlock> block);
  
    virtual ~BRTInst() = default;
    BRTInst(const BRTInst&) = delete;
    BRTInst(BRTInst&&) noexcept = default;
    BRTInst& operator=(const BRTInst&) = delete;
    BRTInst& operator=(BRTInst&&) noexcept = default;
  
    std::shared_ptr<Inst> getCond();
    std::shared_ptr<BasicBlock> getTargetSuccess();
    std::shared_ptr<BasicBlock> getTargetFailed();
  
    virtual std::string getString() override;
    virtual void push_user(std::shared_ptr<Inst> user) override;
    virtual void setup_def_use();
    virtual std::vector<std::shared_ptr<Inst>>& getOperands() override;
    virtual std::shared_ptr<BasicBlock> getBlock() override;
    virtual InstType getInstType() const override;
};

class BRFInst : public Inst
{
private:
    std::shared_ptr<BasicBlock> m_targetSuccess;
    std::shared_ptr<BasicBlock> m_targetFailed;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;
    std::shared_ptr<BasicBlock> m_block;

public:
    BRFInst(std::shared_ptr<Inst> cond, std::shared_ptr<BasicBlock> targetSuccess,
            std::shared_ptr<BasicBlock> targetFailed, std::shared_ptr<BasicBlock> block);
  
    virtual ~BRFInst() = default;
    BRFInst(const BRFInst&) = delete;
    BRFInst(BRFInst&&) noexcept = default;
    BRFInst& operator=(const BRFInst&) = delete;
    BRFInst& operator=(BRFInst&&) noexcept = default;
  
    std::shared_ptr<Inst> getCond();
    std::shared_ptr<BasicBlock> getTargetSuccess();
    std::shared_ptr<BasicBlock> getTargetFailed();
  
    virtual std::string getString() override;
    virtual void push_user(std::shared_ptr<Inst> user) override;
    virtual void setup_def_use();
    virtual std::vector<std::shared_ptr<Inst>>& getOperands() override;
    virtual std::shared_ptr<BasicBlock> getBlock() override;
    virtual InstType getInstType() const override;
};

class PutInst : public Inst
{
private:
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;
    std::shared_ptr<BasicBlock> m_block;

public:
    PutInst(std::shared_ptr<Inst> operand, std::shared_ptr<BasicBlock> block);
  
    virtual ~PutInst() = default;
    PutInst(const PutInst&) = delete;
    PutInst(PutInst&&) noexcept = default;
    PutInst& operator=(const PutInst&) = delete;
    PutInst& operator=(PutInst&&) noexcept = default;
  
    std::shared_ptr<Inst> getOperand();
  
    virtual std::string getString() override;
    virtual void push_user(std::shared_ptr<Inst> user) override;
    virtual void setup_def_use();
    virtual std::vector<std::shared_ptr<Inst>>& getOperands() override;
    virtual std::shared_ptr<BasicBlock> getBlock() override;
    virtual InstType getInstType() const override;
};

class GetInst : public Inst
{
private:
    std::shared_ptr<Inst> m_target;
    std::vector<std::shared_ptr<Inst>> m_users;
    std::shared_ptr<BasicBlock> m_block;

public:
    GetInst(std::shared_ptr<Inst> target, std::shared_ptr<BasicBlock> block);
  
    virtual ~GetInst() = default;
    GetInst(const GetInst&) = delete;
    GetInst(GetInst&&) noexcept = default;
    GetInst& operator=(const GetInst&) = delete;
    GetInst& operator=(GetInst&&) noexcept = default;
  
    std::shared_ptr<Inst> getTarget() override;
  
    virtual std::string getString() override;
    virtual void push_user(std::shared_ptr<Inst> user) override;
    virtual void setup_def_use();
    virtual std::vector<std::shared_ptr<Inst>>& getOperands() override;
    virtual void setTarget(std::shared_ptr<Inst> target);
    virtual std::shared_ptr<BasicBlock> getBlock() override;
    virtual InstType getInstType() const override;
};

class PushInst : public Inst
{
private:
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;
    std::shared_ptr<BasicBlock> m_block;

public:
    PushInst(std::shared_ptr<Inst> operand, std::shared_ptr<BasicBlock> block);

    virtual ~PushInst() = default;
    PushInst(const PushInst&) = delete;
    PushInst(PushInst&&) noexcept = default;
    PushInst& operator=(const PushInst&) = delete;
    PushInst& operator=(PushInst&&) noexcept = default;
  
    std::shared_ptr<Inst> getOperand();
  
    virtual std::string getString() override;
    virtual void push_user(std::shared_ptr<Inst> user) override;
    virtual void setup_def_use();
    virtual std::vector<std::shared_ptr<Inst>>& getOperands() override;
    virtual std::shared_ptr<BasicBlock> getBlock() override;
    virtual InstType getInstType() const override;
};

class PopInst : public Inst
{
private:
    std::shared_ptr<Inst> m_target;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;
    std::shared_ptr<BasicBlock> m_block;

public:
    PopInst(std::shared_ptr<Inst> target, std::shared_ptr<BasicBlock> block);
  
    virtual ~PopInst() = default;
    PopInst(const PopInst&) = delete;
    PopInst(PopInst&&) noexcept = default;
    PopInst& operator=(const PopInst&) = delete;
    PopInst& operator=(PopInst&&) noexcept = default;
  
    virtual std::shared_ptr<Inst> getTarget() override;
  
    virtual std::string getString() override;
    virtual void push_user(std::shared_ptr<Inst> user) override;
    virtual void setup_def_use();
    virtual std::vector<std::shared_ptr<Inst>>& getOperands() override;
    virtual void setTarget(std::shared_ptr<Inst> target);
    virtual std::shared_ptr<BasicBlock> getBlock() override;
    virtual InstType getInstType() const override;
};

class ReturnInst : public Inst
{
private:
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;
    std::shared_ptr<BasicBlock> m_block;

public:
    ReturnInst(std::shared_ptr<BasicBlock> block);
    virtual ~ReturnInst() = default;
    ReturnInst(const ReturnInst&) = delete;
    ReturnInst(ReturnInst&&) noexcept = default;
    ReturnInst& operator=(const ReturnInst&) = delete;
    ReturnInst& operator=(ReturnInst&&) noexcept = default;
  
    virtual std::string getString() override;
    virtual void push_user(std::shared_ptr<Inst> user) override;
    virtual void setup_def_use();
    virtual std::vector<std::shared_ptr<Inst>>& getOperands() override;
    virtual std::shared_ptr<BasicBlock> getBlock() override;
    virtual InstType getInstType() const override;
};

class FuncSignature : public Inst
{
private:
    std::string m_funcName;
    FType m_fType;
    Type m_retType;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;
    std::shared_ptr<BasicBlock> m_block;

public:
    FuncSignature(std::string funcName, FType fType, Type retType,
                  std::vector<std::shared_ptr<Inst>> parameters,
                  std::shared_ptr<BasicBlock> block);
    virtual ~FuncSignature() = default;
    FuncSignature(const FuncSignature&) = delete;
    FuncSignature(FuncSignature&&) noexcept = default;
    FuncSignature& operator=(const FuncSignature&) = delete;
    FuncSignature& operator=(FuncSignature&&) noexcept = default;

    std::string getFuncName();

    virtual std::string getString() override;
    virtual void push_user(std::shared_ptr<Inst> user) override;
    virtual void setup_def_use();
    virtual std::vector<std::shared_ptr<Inst>>& getOperands() override;
    virtual std::shared_ptr<BasicBlock> getBlock() override;
    virtual InstType getInstType() const override;
};

class CallInst : public Inst
{
private:
    std::string m_calleeStr;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;
    std::shared_ptr<BasicBlock> m_block;

public:
    CallInst(std::string calleeStr, std::vector<std::shared_ptr<Inst>> operands, std::shared_ptr<BasicBlock> block);
    virtual ~CallInst() = default;
    CallInst(const CallInst&) = delete;
    CallInst(CallInst&&) noexcept = default;
    CallInst& operator=(const CallInst&) = delete;
    CallInst& operator=(CallInst&&) noexcept = default;
  
    std::string getCalleeStr();
  
    virtual std::string getString() override;
    virtual void push_user(std::shared_ptr<Inst> user) override;
    virtual void setup_def_use();
    virtual std::vector<std::shared_ptr<Inst>>& getOperands() override;
    virtual std::shared_ptr<BasicBlock> getBlock() override;
    virtual InstType getInstType() const override;
};

class PhiInst : public Inst
{
private:
    std::shared_ptr<Inst> m_target;
    std::shared_ptr<BasicBlock> m_block;
    std::vector<std::shared_ptr<Inst>> m_users;

public:
    PhiInst(std::string name, std::shared_ptr<BasicBlock> block);

    virtual ~PhiInst() = default;
    PhiInst(const PhiInst&) = delete;
    PhiInst(PhiInst&&) noexcept = default;
    PhiInst& operator=(const PhiInst&) = delete;
    PhiInst& operator=(PhiInst&&) noexcept = default;

    void appendOperand(std::shared_ptr<Inst> operand);
    
    virtual std::shared_ptr<Inst> getTarget() override;
    virtual std::shared_ptr<BasicBlock> getBlock();
    virtual std::string getString() override;
    virtual void push_user(std::shared_ptr<Inst> user) override;
    virtual void setup_def_use();
    std::vector<std::shared_ptr<Inst>>& get_users();
    virtual std::vector<std::shared_ptr<Inst>>& getOperands() override;
    virtual bool isPhi() override;
    virtual void setTarget(std::shared_ptr<Inst> target);
    virtual InstType getInstType() const override;
};

class UndefInst: public Inst
{
private:
    std::shared_ptr<BasicBlock> m_block;

public:
    UndefInst(std::shared_ptr<BasicBlock> block) : m_block(std::move(block)) {}
    std::string getString();
    virtual std::shared_ptr<BasicBlock> getBlock() override;
    virtual InstType getInstType() const override;
};

class NoopInst : public Inst
{
private:
    std::shared_ptr<BasicBlock> m_block;

public:
    NoopInst(std::shared_ptr<BasicBlock> block) : m_block(std::move(block)) {}

    std::string getString();
    virtual std::shared_ptr<BasicBlock> getBlock() override;
    virtual InstType getInstType() const override;
};
