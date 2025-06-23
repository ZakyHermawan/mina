#pragma once
#include <memory>
#include <string>
#include <vector>
#include <iostream>

#include "Types.hpp"
#include "BasicBlock.hpp"

class BasicBlock;

class Inst : public std::enable_shared_from_this<Inst>
{
protected:
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;
    std::shared_ptr<BasicBlock> m_block;

public:
    Inst() {};
    virtual std::string getString() { return ""; };
    virtual std::shared_ptr<Inst> getTarget() { return shared_from_this(); }
    virtual void push_user(std::shared_ptr<Inst> user) {};
    virtual void setup_def_use() {}
    virtual bool isPhi() { return false; }
    virtual void insert_block(std::shared_ptr<BasicBlock> block)
    {
        m_block = block;
    }
    virtual std::vector<std::shared_ptr<Inst>>& getOperands() { return m_operands; }
    virtual std::shared_ptr<BasicBlock> getBlock() { return m_block; }
};

class IntConstInst : public Inst
{
private:
    int m_val;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;
    std::shared_ptr<BasicBlock> m_block;

public:
    IntConstInst(int val);

    virtual ~IntConstInst() = default;
    IntConstInst(const IntConstInst&) = delete;
    IntConstInst(IntConstInst&&) noexcept = default;
    IntConstInst& operator=(const IntConstInst&) = delete;
    IntConstInst& operator=(IntConstInst&&) noexcept = default;

    int getVal();
    std::string getString();
    void push_user(std::shared_ptr<Inst> user) override;
    void setup_def_use();
    std::vector<std::shared_ptr<Inst>>& getOperands() override { return m_operands; }
    
};

class BoolConstInst : public Inst
{
private:
    bool m_val;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;

public:
    BoolConstInst(bool val);

    virtual ~BoolConstInst() = default;
    BoolConstInst(const BoolConstInst&) = delete;
    BoolConstInst(BoolConstInst&&) noexcept = default;
    BoolConstInst& operator=(const BoolConstInst&) = delete;
    BoolConstInst& operator=(BoolConstInst&&) noexcept = default;

    bool getVal();
    std::string getString();
    void push_user(std::shared_ptr<Inst> user) override;
    void setup_def_use();
    std::vector<std::shared_ptr<Inst>>& getOperands() override { return m_operands; }
};

class StrConstInst : public Inst
{
private:
    std::string m_val;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;

public:
    StrConstInst(std::string val);

    virtual ~StrConstInst() = default;
    StrConstInst(const StrConstInst&) = delete;
    StrConstInst(StrConstInst&&) noexcept = default;
    StrConstInst& operator=(const StrConstInst&) = delete;
    StrConstInst& operator=(StrConstInst&&) noexcept = default;

    std::string getString();
    void push_user(std::shared_ptr<Inst> user) override;
    void setup_def_use();
    std::vector<std::shared_ptr<Inst>>& getOperands() override { return m_operands; }
};

class IdentInst : public Inst
{
private:
    std::string m_name;
    std::vector<std::shared_ptr<Inst>> m_users;

   public:
    IdentInst(std::string name);

    virtual ~IdentInst() = default;
    IdentInst(const IdentInst&) = delete;
    IdentInst(IdentInst&&) noexcept = default;
    IdentInst& operator=(const IdentInst&) = delete;
    IdentInst& operator=(IdentInst&&) noexcept = default;

    std::string getString();
    void push_user(std::shared_ptr<Inst> user) override;
    void setup_def_use();
    std::vector<std::shared_ptr<Inst>>& getOperands() override { return m_operands; }
    
};

class AddInst: public Inst
{
private:
    std::shared_ptr<Inst> m_target, m_operand1, m_operand2;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;

public:
    AddInst(std::shared_ptr<Inst> target, std::shared_ptr<Inst> operand1, std::shared_ptr<Inst> operand2);

    virtual ~AddInst() = default;
    AddInst(const AddInst&) = delete;
    AddInst(AddInst&&) noexcept = default;
    AddInst& operator=(const AddInst&) = delete;
    AddInst& operator=(AddInst&&) noexcept = default;

    std::shared_ptr<Inst> getTarget() override;
    std::shared_ptr<Inst> getOperand1();
    std::shared_ptr<Inst> getOperand2();

    std::string getString();
    void push_user(std::shared_ptr<Inst> user) override;
    void setup_def_use();
    std::vector<std::shared_ptr<Inst>>& getOperands() override { return m_operands; }
};

class SubInst: public Inst
{
private:
    std::shared_ptr<Inst> m_target, m_operand1, m_operand2;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;

public:
    SubInst(std::shared_ptr<Inst> target, std::shared_ptr<Inst> operand1,
            std::shared_ptr<Inst> operand2);

    virtual ~SubInst() = default;
    SubInst(const SubInst&) = delete;
    SubInst(SubInst&&) noexcept = default;
    SubInst& operator=(const SubInst&) = delete;
    SubInst& operator=(SubInst&&) noexcept = default;

    std::shared_ptr<Inst> getTarget() override;
    std::shared_ptr<Inst> getOperand1();
    std::shared_ptr<Inst> getOperand2();

    std::string getString();
    void push_user(std::shared_ptr<Inst> user) override;
    void setup_def_use();
    std::vector<std::shared_ptr<Inst>>& getOperands() override { return m_operands; }
};

class MulInst: public Inst
{
private:
    std::shared_ptr<Inst> m_target, m_operand1, m_operand2;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;

public:
    MulInst(std::shared_ptr<Inst> target, std::shared_ptr<Inst> operand1,
            std::shared_ptr<Inst> operand2);

    virtual ~MulInst() = default;
    MulInst(const MulInst&) = delete;
    MulInst(MulInst&&) noexcept = default;
    MulInst& operator=(const MulInst&) = delete;
    MulInst& operator=(MulInst&&) noexcept = default;

    std::shared_ptr<Inst> getTarget() override;
    std::shared_ptr<Inst> getOperand1();
    std::shared_ptr<Inst> getOperand2();

    std::string getString();
    void push_user(std::shared_ptr<Inst> user) override;
    void setup_def_use();
    std::vector<std::shared_ptr<Inst>>& getOperands() override { return m_operands; }
};

class DivInst: public Inst
{
private:
    std::shared_ptr<Inst> m_target, m_operand1, m_operand2;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;

public:
    DivInst(std::shared_ptr<Inst> target, std::shared_ptr<Inst> operand1,
            std::shared_ptr<Inst> operand2);

    virtual ~DivInst() = default;
    DivInst(const DivInst&) = delete;
    DivInst(DivInst&&) noexcept = default;
    DivInst& operator=(const DivInst&) = delete;
    DivInst& operator=(DivInst&&) noexcept = default;

    std::shared_ptr<Inst> getTarget() override;
    std::shared_ptr<Inst> getOperand1();
    std::shared_ptr<Inst> getOperand2();

    std::string getString();
    void push_user(std::shared_ptr<Inst> user) override;
    void setup_def_use();
    std::vector<std::shared_ptr<Inst>>& getOperands() override { return m_operands; }
};

class NotInst: public Inst
{
private:
    std::shared_ptr<Inst> m_target, m_operand;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;

public:
    NotInst(std::shared_ptr<Inst> target, std::shared_ptr<Inst> operand);

    virtual ~NotInst() = default;
    NotInst(const NotInst&) = delete;
    NotInst(NotInst&&) noexcept = default;
    NotInst& operator=(const NotInst&) = delete;
    NotInst& operator=(NotInst&&) noexcept = default;

    std::shared_ptr<Inst> getTarget() override;
    std::shared_ptr<Inst> getOperand();

    std::string getString();
    void push_user(std::shared_ptr<Inst> user) override;
    void setup_def_use();
    std::vector<std::shared_ptr<Inst>>& getOperands() override { return m_operands; }
};

class AndInst: public Inst
{
private:
    std::shared_ptr<Inst> m_target, m_operand1, m_operand2;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;

public:
    AndInst(std::shared_ptr<Inst> target, std::shared_ptr<Inst> operand1,
            std::shared_ptr<Inst> operand2);

    virtual ~AndInst() = default;
    AndInst(const AndInst&) = delete;
    AndInst(AndInst&&) noexcept = default;
    AndInst& operator=(const AndInst&) = delete;
    AndInst& operator=(AndInst&&) noexcept = default;

    std::shared_ptr<Inst> getTarget() override;
    std::shared_ptr<Inst> getOperand1();
    std::shared_ptr<Inst> getOperand2();

    std::string getString();
    void push_user(std::shared_ptr<Inst> user) override;
    void setup_def_use();
    std::vector<std::shared_ptr<Inst>>& getOperands() override { return m_operands; }
};

class OrInst: public Inst
{
private:
    std::shared_ptr<Inst> m_target, m_operand1, m_operand2;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;

public:
    OrInst(std::shared_ptr<Inst> target, std::shared_ptr<Inst> operand1,
            std::shared_ptr<Inst> operand2);

    virtual ~OrInst() = default;
    OrInst(const OrInst&) = delete;
    OrInst(OrInst&&) noexcept = default;
    OrInst& operator=(const OrInst&) = delete;
    OrInst& operator=(OrInst&&) noexcept = default;

    std::shared_ptr<Inst> getTarget() override;
    std::shared_ptr<Inst> getOperand1();
    std::shared_ptr<Inst> getOperand2();

    std::string getString();
    void push_user(std::shared_ptr<Inst> user) override;
    void setup_def_use();
    std::vector<std::shared_ptr<Inst>>& getOperands() override { return m_operands; }
};

class AllocaInst : public Inst
{
private:
    std::shared_ptr<Inst> m_target;
    Type m_type;
    unsigned int m_size;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;

public:
    AllocaInst(std::shared_ptr<Inst> target, Type type, unsigned int size);

    virtual ~AllocaInst() = default;
    AllocaInst(const AllocaInst&) = delete;
    AllocaInst(AllocaInst&&) noexcept = default;
    AllocaInst& operator=(const AllocaInst&) = delete;
    AllocaInst& operator=(AllocaInst&&) noexcept = default;
    
    std::shared_ptr<Inst> getTarget() override;
    Type getType();
    unsigned int getSize();
    
    std::string getString();
    void push_user(std::shared_ptr<Inst> user) override;
    void setup_def_use();
    std::vector<std::shared_ptr<Inst>>& getOperands() override { return m_operands; }
};

class ArrAccessInst : public Inst
{
private:
    std::shared_ptr<Inst> m_target, m_source, m_index;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;

public:
    ArrAccessInst(std::shared_ptr<Inst> target, std::shared_ptr<Inst> source,
                  std::shared_ptr<Inst> index);
    
    virtual ~ArrAccessInst() = default;
    ArrAccessInst(const ArrAccessInst&) = delete;
    ArrAccessInst(ArrAccessInst&&) noexcept = default;
    ArrAccessInst& operator=(const ArrAccessInst&) = delete;
    ArrAccessInst& operator=(ArrAccessInst&&) noexcept = default;
    
    std::shared_ptr<Inst> getTarget() override;
    std::shared_ptr<Inst> getSource();
    std::shared_ptr<Inst> getIndex();
    
    std::string getString();
    void push_user(std::shared_ptr<Inst> user) override;
    void setup_def_use();
    std::vector<std::shared_ptr<Inst>>& getOperands() override { return m_operands; }
};

class ArrUpdateInst : public Inst
{
private:
    std::shared_ptr<Inst> m_target, m_source, m_index, m_val;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;

public:
    ArrUpdateInst(std::shared_ptr<Inst> target, std::shared_ptr<Inst> source,
                  std::shared_ptr<Inst> index, std::shared_ptr<Inst> val);

    virtual ~ArrUpdateInst() = default;
    ArrUpdateInst(const ArrUpdateInst&) = delete;
    ArrUpdateInst(ArrUpdateInst&&) noexcept = default;
    ArrUpdateInst& operator=(const ArrUpdateInst&) = delete;
    ArrUpdateInst& operator=(ArrUpdateInst&&) noexcept = default;
    
    std::shared_ptr<Inst> getTarget() override;
    std::shared_ptr<Inst> getSource();
    std::shared_ptr<Inst> getIndex();
    std::shared_ptr<Inst> getVal();
    
    std::string getString();
    void push_user(std::shared_ptr<Inst> user) override;
    void setup_def_use();
    std::vector<std::shared_ptr<Inst>>& getOperands() override { return m_operands; }
};

class AssignInst : public Inst
{
private:
    std::shared_ptr<Inst> m_target, m_source;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;

public:
    AssignInst(std::shared_ptr<Inst> target, std::shared_ptr<Inst> source);
  
    virtual ~AssignInst() = default;
    AssignInst(const AssignInst&) = delete;
    AssignInst(AssignInst&&) noexcept = default;
    AssignInst& operator=(const AssignInst&) = delete;
    AssignInst& operator=(AssignInst&&) noexcept = default;
  
    std::shared_ptr<Inst> getTarget() override;
    std::shared_ptr<Inst> getSource();
  
    std::string getString();
    void push_user(std::shared_ptr<Inst> user) override;
    void setup_def_use();
    std::vector<std::shared_ptr<Inst>>& getOperands() override { return m_operands; }
};

class CmpEQInst : public Inst
{
private:
    std::shared_ptr<Inst> m_target, m_operand1, m_operand2;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;

public:
    CmpEQInst(std::shared_ptr<Inst> target, std::shared_ptr<Inst> operand1,
              std::shared_ptr<Inst> operand2);
  
    virtual ~CmpEQInst() = default;
    CmpEQInst(const CmpEQInst&) = delete;
    CmpEQInst(CmpEQInst&&) noexcept = default;
    CmpEQInst& operator=(const CmpEQInst&) = delete;
    CmpEQInst& operator=(CmpEQInst&&) noexcept = default;
  
    std::shared_ptr<Inst> getTarget() override;
    std::shared_ptr<Inst> getOperand1();
    std::shared_ptr<Inst> getOperand2();
  
    std::string getString();
    void push_user(std::shared_ptr<Inst> user) override;
    void setup_def_use();
    std::vector<std::shared_ptr<Inst>>& getOperands() override { return m_operands; }
};

class CmpNEInst : public Inst
{
private:
    std::shared_ptr<Inst> m_target, m_operand1, m_operand2;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;

public:
    CmpNEInst(std::shared_ptr<Inst> target, std::shared_ptr<Inst> operand1,
              std::shared_ptr<Inst> operand2);
  
    virtual ~CmpNEInst() = default;
    CmpNEInst(const CmpNEInst&) = delete;
    CmpNEInst(CmpNEInst&&) noexcept = default;
    CmpNEInst& operator=(const CmpNEInst&) = delete;
    CmpNEInst& operator=(CmpNEInst&&) noexcept = default;
  
    std::shared_ptr<Inst> getTarget() override;
    std::shared_ptr<Inst> getOperand1();
    std::shared_ptr<Inst> getOperand2();
  
    std::string getString();
    void push_user(std::shared_ptr<Inst> user) override;
    void setup_def_use();
    std::vector<std::shared_ptr<Inst>>& getOperands() override { return m_operands; }
};

class CmpLTInst : public Inst
{
private:
    std::shared_ptr<Inst> m_target, m_operand1, m_operand2;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;

public:
    CmpLTInst(std::shared_ptr<Inst> target, std::shared_ptr<Inst> operand1,
              std::shared_ptr<Inst> operand2);
  
    virtual ~CmpLTInst() = default;
    CmpLTInst(const CmpLTInst&) = delete;
    CmpLTInst(CmpLTInst&&) noexcept = default;
    CmpLTInst& operator=(const CmpLTInst&) = delete;
    CmpLTInst& operator=(CmpLTInst&&) noexcept = default;
  
    std::shared_ptr<Inst> getTarget() override;
    std::shared_ptr<Inst> getOperand1();
    std::shared_ptr<Inst> getOperand2();
  
    std::string getString();
    void push_user(std::shared_ptr<Inst> user) override;
    void setup_def_use();
    std::vector<std::shared_ptr<Inst>>& getOperands() override { return m_operands; }
};

class CmpLTEInst : public Inst
{
private:
    std::shared_ptr<Inst> m_target, m_operand1, m_operand2;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;

public:
    CmpLTEInst(std::shared_ptr<Inst> target, std::shared_ptr<Inst> operand1,
               std::shared_ptr<Inst> operand2);

    virtual ~CmpLTEInst() = default;
    CmpLTEInst(const CmpLTEInst&) = delete;
    CmpLTEInst(CmpLTEInst&&) noexcept = default;
    CmpLTEInst& operator=(const CmpLTEInst&) = delete;
    CmpLTEInst& operator=(CmpLTEInst&&) noexcept = default;
    
    std::shared_ptr<Inst> getTarget() override;
    std::shared_ptr<Inst> getOperand1();
    std::shared_ptr<Inst> getOperand2();
    
    std::string getString();
    void push_user(std::shared_ptr<Inst> user) override;
    void setup_def_use();
    std::vector<std::shared_ptr<Inst>>& getOperands() override { return m_operands; }
};

class CmpGTInst : public Inst
{
private:
    std::shared_ptr<Inst> m_target, m_operand1, m_operand2;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;

public:
    CmpGTInst(std::shared_ptr<Inst> target, std::shared_ptr<Inst> operand1,
              std::shared_ptr<Inst> operand2);

    virtual ~CmpGTInst() = default;
    CmpGTInst(const CmpGTInst&) = delete;
    CmpGTInst(CmpGTInst&&) noexcept = default;
    CmpGTInst& operator=(const CmpGTInst&) = delete;
    CmpGTInst& operator=(CmpGTInst&&) noexcept = default;
    
    std::shared_ptr<Inst> getTarget() override;
    std::shared_ptr<Inst> getOperand1();
    std::shared_ptr<Inst> getOperand2();
    
    std::string getString();
    void push_user(std::shared_ptr<Inst> user) override;
    void setup_def_use();
    std::vector<std::shared_ptr<Inst>>& getOperands() override { return m_operands; }
};

class CmpGTEInst : public Inst
{
private:
    std::shared_ptr<Inst> m_target, m_operand1, m_operand2;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;

public:
    CmpGTEInst(std::shared_ptr<Inst> target, std::shared_ptr<Inst> operand1,
               std::shared_ptr<Inst> operand2);
  
    virtual ~CmpGTEInst() = default;
    CmpGTEInst(const CmpGTEInst&) = delete;
    CmpGTEInst(CmpGTEInst&&) noexcept = default;
    CmpGTEInst& operator=(const CmpGTEInst&) = delete;
    CmpGTEInst& operator=(CmpGTEInst&&) noexcept = default;
  
    std::shared_ptr<Inst> getTarget() override;
    std::shared_ptr<Inst> getOperand1();
    std::shared_ptr<Inst> getOperand2();
  
    std::string getString();
    void push_user(std::shared_ptr<Inst> user) override;
    void setup_def_use();
    std::vector<std::shared_ptr<Inst>>& getOperands() override { return m_operands; }
};

class JumpInst : public Inst
{
private:
    std::shared_ptr<BasicBlock> m_target;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;

public:
    JumpInst(std::shared_ptr<BasicBlock> target);
  
    virtual ~JumpInst() = default;
    JumpInst(const JumpInst&) = delete;
    JumpInst(JumpInst&&) noexcept = default;
    JumpInst& operator=(const JumpInst&) = delete;
    JumpInst& operator=(JumpInst&&) noexcept = default;
  
    std::shared_ptr<BasicBlock> getJumpTarget();
  
    std::string getString();
    void push_user(std::shared_ptr<Inst> user) override;
    void setup_def_use();
    std::vector<std::shared_ptr<Inst>>& getOperands() override { return m_operands; }
};

class BRTInst : public Inst
{
private:
    std::shared_ptr<Inst> m_cond;
    std::shared_ptr<BasicBlock> m_targetSuccess;
    std::shared_ptr<BasicBlock> m_targetFailed;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;

public:
    BRTInst(std::shared_ptr<Inst> cond, std::shared_ptr<BasicBlock> targetSuccess,
            std::shared_ptr<BasicBlock> targetFailed);
  
    virtual ~BRTInst() = default;
    BRTInst(const BRTInst&) = delete;
    BRTInst(BRTInst&&) noexcept = default;
    BRTInst& operator=(const BRTInst&) = delete;
    BRTInst& operator=(BRTInst&&) noexcept = default;
  
    std::shared_ptr<Inst> getCond();
    std::shared_ptr<BasicBlock> getTargetSuccess();
    std::shared_ptr<BasicBlock> getTargetFailed();
  
    std::string getString();
    void push_user(std::shared_ptr<Inst> user) override;
    void setup_def_use();
    std::vector<std::shared_ptr<Inst>>& getOperands() override { return m_operands; }
};

class BRFInst : public Inst
{
private:
    std::shared_ptr<Inst> m_cond;
    std::shared_ptr<BasicBlock> m_targetSuccess;
    std::shared_ptr<BasicBlock> m_targetFailed;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;

public:
    BRFInst(std::shared_ptr<Inst> cond, std::shared_ptr<BasicBlock> targetSuccess,
            std::shared_ptr<BasicBlock> targetFailed);
  
    virtual ~BRFInst() = default;
    BRFInst(const BRFInst&) = delete;
    BRFInst(BRFInst&&) noexcept = default;
    BRFInst& operator=(const BRFInst&) = delete;
    BRFInst& operator=(BRFInst&&) noexcept = default;
  
    std::shared_ptr<Inst> getCond();
    std::shared_ptr<BasicBlock> getTargetSuccess();
    std::shared_ptr<BasicBlock> getTargetFailed();
  
    std::string getString();
    void push_user(std::shared_ptr<Inst> user) override;
    void setup_def_use();
    std::vector<std::shared_ptr<Inst>>& getOperands() override { return m_operands; }
};

class PutInst : public Inst
{
private:
    std::shared_ptr<Inst> m_operand;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;

public:
    PutInst(std::shared_ptr<Inst> operand);
  
    virtual ~PutInst() = default;
    PutInst(const PutInst&) = delete;
    PutInst(PutInst&&) noexcept = default;
    PutInst& operator=(const PutInst&) = delete;
    PutInst& operator=(PutInst&&) noexcept = default;
  
    std::shared_ptr<Inst> getOperand();
  
    std::string getString();
    void push_user(std::shared_ptr<Inst> user) override;
    void setup_def_use();
    std::vector<std::shared_ptr<Inst>>& getOperands() override { return m_operands; }
};

class GetInst : public Inst
{
private:
    std::shared_ptr<Inst> m_target;
    std::vector<std::shared_ptr<Inst>> m_users;

public:
    GetInst(std::shared_ptr<Inst> target);
  
    virtual ~GetInst() = default;
    GetInst(const GetInst&) = delete;
    GetInst(GetInst&&) noexcept = default;
    GetInst& operator=(const GetInst&) = delete;
    GetInst& operator=(GetInst&&) noexcept = default;
  
    std::shared_ptr<Inst> getTarget() override;
  
    std::string getString();
    void push_user(std::shared_ptr<Inst> user) override;
    void setup_def_use();
    std::vector<std::shared_ptr<Inst>>& getOperands() override { return m_operands; }
};

class PushInst : public Inst
{
private:
    std::shared_ptr<Inst> m_operand;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;

public:
    PushInst(std::shared_ptr<Inst> operand);
  
    virtual ~PushInst() = default;
    PushInst(const PushInst&) = delete;
    PushInst(PushInst&&) noexcept = default;
    PushInst& operator=(const PushInst&) = delete;
    PushInst& operator=(PushInst&&) noexcept = default;
  
    std::shared_ptr<Inst> getOperand();
  
    std::string getString();
    void push_user(std::shared_ptr<Inst> user) override;
    void setup_def_use();
    std::vector<std::shared_ptr<Inst>>& getOperands() override { return m_operands; }
};

class PopInst : public Inst
{
private:
    std::shared_ptr<Inst> m_target;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;

public:
    PopInst(std::shared_ptr<Inst> target);
  
    virtual ~PopInst() = default;
    PopInst(const PopInst&) = delete;
    PopInst(PopInst&&) noexcept = default;
    PopInst& operator=(const PopInst&) = delete;
    PopInst& operator=(PopInst&&) noexcept = default;
  
    std::shared_ptr<Inst> getTarget() override;
  
    std::string getString();
    void push_user(std::shared_ptr<Inst> user) override;
    void setup_def_use();
    std::vector<std::shared_ptr<Inst>>& getOperands() override { return m_operands; }
};

class ReturnInst : public Inst
{
private:
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;

public:
    ReturnInst();
    virtual ~ReturnInst() = default;
    ReturnInst(const ReturnInst&) = delete;
    ReturnInst(ReturnInst&&) noexcept = default;
    ReturnInst& operator=(const ReturnInst&) = delete;
    ReturnInst& operator=(ReturnInst&&) noexcept = default;
  
    std::string getString();
    void push_user(std::shared_ptr<Inst> user) override;
    void setup_def_use();
    std::vector<std::shared_ptr<Inst>>& getOperands() override { return m_operands; }
};

class CallInst : public Inst
{
private:
    std::string m_calleeStr;
    std::vector<std::shared_ptr<Inst>> m_users, m_operands;

public:
    CallInst(std::string calleeStr);
    virtual ~CallInst() = default;
    CallInst(const CallInst&) = delete;
    CallInst(CallInst&&) noexcept = default;
    CallInst& operator=(const CallInst&) = delete;
    CallInst& operator=(CallInst&&) noexcept = default;
  
    std::string getCalleeStr();
  
    std::string getString();
    void push_user(std::shared_ptr<Inst> user) override;
    void setup_def_use();
    std::vector<std::shared_ptr<Inst>>& getOperands() override { return m_operands; }
};

class PhiInst : public Inst
{
private:
    std::shared_ptr<Inst> m_target;
    std::shared_ptr<BasicBlock> m_block;
    std::vector<std::shared_ptr<Inst>> m_users;

public:
    PhiInst(std::string name, std::shared_ptr<BasicBlock> m_block);

    virtual ~PhiInst() = default;
    PhiInst(const PhiInst&) = delete;
    PhiInst(PhiInst&&) noexcept = default;
    PhiInst& operator=(const PhiInst&) = delete;
    PhiInst& operator=(PhiInst&&) noexcept = default;

    void appendOperand(std::shared_ptr<Inst> operand);
    
    std::shared_ptr<Inst> getTarget() override;
    std::shared_ptr<BasicBlock> getBlock();
    std::string getString();
    void push_user(std::shared_ptr<Inst> user) override;
    void setup_def_use();
    std::vector<std::shared_ptr<Inst>>& get_users();
    std::vector<std::shared_ptr<Inst>>& getOperands() override { return m_operands; }
    bool isPhi() { return true; }
};

class UndefInst: public Inst
{
public:
    std::string getString() { return "Undef"; }
};

class Noop : public Inst
{
public:
    std::string getString() { return "noop"; }
};
