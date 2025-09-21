#include "Ast.hpp"
#include "InstIR.hpp"
#include "BasicBlock.hpp"
#include <iostream>

#include <stdexcept>

IntConstInst::IntConstInst(int val, std::shared_ptr<BasicBlock> block)
    : m_val(val), m_block(std::move(block))
{
}
int IntConstInst::getVal() { return m_val; }
std::string IntConstInst::getString()
{
    //return "IntConst(" + std::to_string(m_val) + ")";
    return std::to_string(m_val);
}
void IntConstInst::push_user(std::shared_ptr<Inst> user)
{
    m_users.push_back(user);
}
void IntConstInst::setup_def_use()
{
}
std::vector<std::shared_ptr<Inst>>& IntConstInst::getOperands()
{
    return m_operands;
}
bool IntConstInst::canBeRenamed() const { return false; }
std::shared_ptr<BasicBlock> IntConstInst::getBlock() { return m_block; };
InstType IntConstInst::getInstType() const { return InstType::IntConst; }

BoolConstInst::BoolConstInst(bool val, std::shared_ptr<BasicBlock> block)
    : m_val(val), m_block(std::move(block))
{
}
bool BoolConstInst::getVal() { return m_val; }
std::string BoolConstInst::getString()
{
    if (m_val)
    {
        return "true";
    }
    else
    {
        return "false";
    }
}
void BoolConstInst::push_user(std::shared_ptr<Inst> user)
{
    m_users.push_back(user);
}
void BoolConstInst::setup_def_use() {}
std::vector<std::shared_ptr<Inst>>& BoolConstInst::getOperands()
{
    return m_operands;
}
bool BoolConstInst::canBeRenamed() const { return false; }
std::shared_ptr<BasicBlock> BoolConstInst::getBlock() { return m_block; };
InstType BoolConstInst::getInstType() const { return InstType::BoolConst; }

StrConstInst::StrConstInst(std::string val, std::shared_ptr<BasicBlock> block)
    : m_val(val), m_block(std::move(block))
{
}
std::string StrConstInst::getString()
{
    return m_val;
}
void StrConstInst::push_user(std::shared_ptr<Inst> user)
{
    m_users.push_back(user);
}
void StrConstInst::setup_def_use() {}
std::vector<std::shared_ptr<Inst>>& StrConstInst::getOperands()
{
    return m_operands;
}
bool StrConstInst::canBeRenamed() const { return false; }
std::shared_ptr<BasicBlock> StrConstInst::getBlock() { return m_block; };
InstType StrConstInst::getInstType() const { return InstType::StrConst; }

IdentInst::IdentInst(std::string name, std::shared_ptr<BasicBlock> block)
    : m_name(std::move(name)), m_block(std::move(block))
{
}
std::string IdentInst::getString() { return m_name; }
void IdentInst::push_user(std::shared_ptr<Inst> user)
{
    m_users.push_back(user);
}
void IdentInst::setup_def_use()
{
}
std::vector<std::shared_ptr<Inst>>& IdentInst::getOperands()
{
    return m_operands;
}
std::shared_ptr<BasicBlock> IdentInst::getBlock() { return m_block; };
InstType IdentInst::getInstType() const { return InstType::Ident; }

AddInst::AddInst(std::shared_ptr<Inst> target, std::shared_ptr<Inst> operand1,
                 std::shared_ptr<Inst> operand2,
                 std::shared_ptr<BasicBlock> block)
    : m_target(std::move(target)),
      m_operand1(std::move(operand1)),
      m_operand2(std::move(operand2)),
      m_block(std::move(block))
{
    m_operands = std::vector<std::shared_ptr<Inst>>{m_operand1, m_operand2};
}
std::shared_ptr<Inst> AddInst::getTarget() { return m_target; }
std::shared_ptr<Inst> AddInst::getOperand1() {return m_operand1;}
std::shared_ptr<Inst> AddInst::getOperand2() { return m_operand2; }
std::string AddInst::getString()
{
    auto target = m_target->getTarget()->getString();

    std::string res = target + " <- Add(";
    for (int i = 0; i < m_operands.size(); ++i)
    {
        if (i)
        {
            res += ", ";
        }
        res += m_operands[i]->getTarget()->getString();
    }
    res += ")";
    return res;
}
void AddInst::push_user(std::shared_ptr<Inst> user)
{
    m_users.push_back(user);
}
void AddInst::setup_def_use()
{
    m_operand1->push_user(shared_from_this());
    m_operand2->push_user(shared_from_this());
}
std::vector<std::shared_ptr<Inst>>& AddInst::getOperands()
{
    return m_operands;
}
void AddInst::setTarget(std::shared_ptr<Inst> target) { m_target = target; }
std::shared_ptr<BasicBlock> AddInst::getBlock() { return m_block; };
InstType AddInst::getInstType() const { return InstType::Add; }

SubInst::SubInst(std::shared_ptr<Inst> target, std::shared_ptr<Inst> operand1,
                 std::shared_ptr<Inst> operand2,
                 std::shared_ptr<BasicBlock> block)
    : m_target(std::move(target)),
      m_operand1(std::move(operand1)),
      m_operand2(std::move(operand2)),
      m_block(std::move(block))
{
    m_operands = std::vector<std::shared_ptr<Inst>>{m_operand1, m_operand2};
}
std::shared_ptr<Inst> SubInst::getTarget() { return m_target; }
std::shared_ptr<Inst> SubInst::getOperand1() { return m_operand1; }
std::shared_ptr<Inst> SubInst::getOperand2() { return m_operand2; }
std::string SubInst::getString()
{
    auto target = m_target->getTarget()->getString();

    std::string res = target + " <- Sub(";
    for (int i = 0; i < m_operands.size(); ++i)
    {
        if (i)
        {
            res += ", ";
        }
        res += m_operands[i]->getTarget()->getString();
    }
    res += ")";
    return res;
}
void SubInst::push_user(std::shared_ptr<Inst> user)
{
    m_users.push_back(user);
}
void SubInst::setup_def_use()
{
    m_operand1->push_user(shared_from_this());
    m_operand2->push_user(shared_from_this());
}
std::vector<std::shared_ptr<Inst>>& SubInst::getOperands()
{
    return m_operands;
}
void SubInst::setTarget(std::shared_ptr<Inst> target) { m_target = target; }
std::shared_ptr<BasicBlock> SubInst::getBlock() { return m_block; };
InstType SubInst::getInstType() const { return InstType::Sub; }

MulInst::MulInst(std::shared_ptr<Inst> target, std::shared_ptr<Inst> operand1,
                 std::shared_ptr<Inst> operand2,
                 std::shared_ptr<BasicBlock> block)
    : m_target(std::move(target)),
      m_operand1(std::move(operand1)),
      m_operand2(std::move(operand2)),
      m_block(std::move(block))
{
    m_operands = std::vector<std::shared_ptr<Inst>>{m_operand1, m_operand2};
}
std::shared_ptr<Inst> MulInst::getTarget() { return m_target; }
std::shared_ptr<Inst> MulInst::getOperand1() { return m_operand1; }
std::shared_ptr<Inst> MulInst::getOperand2() { return m_operand2; }
std::string MulInst::getString()
{
    auto target = m_target->getTarget()->getString();

    std::string res = target + " <- Mul(";
    for (int i = 0; i < m_operands.size(); ++i)
    {
        if (i)
        {
            res += ", ";
        }
        res += m_operands[i]->getTarget()->getString();
    }
    res += ")";
    return res;
}
void MulInst::push_user(std::shared_ptr<Inst> user)
{
    m_users.push_back(user);
}
void MulInst::setup_def_use()
{
    m_operand1->push_user(shared_from_this());
    m_operand2->push_user(shared_from_this());
}
std::vector<std::shared_ptr<Inst>>& MulInst::getOperands()
{
    return m_operands;
}
void MulInst::setTarget(std::shared_ptr<Inst> target) { m_target = target; }
std::shared_ptr<BasicBlock> MulInst::getBlock() { return m_block; };
InstType MulInst::getInstType() const { return InstType::Mul; }

DivInst::DivInst(std::shared_ptr<Inst> target, std::shared_ptr<Inst> operand1,
                 std::shared_ptr<Inst> operand2,
                 std::shared_ptr<BasicBlock> block)
    : m_target(std::move(target)),
      m_operand1(std::move(operand1)),
      m_operand2(std::move(operand2)),
      m_block(std::move(block))
{
    m_operands = std::vector<std::shared_ptr<Inst>>{m_operand1, m_operand2};
}
std::shared_ptr<Inst> DivInst::getTarget() { return m_target; }
std::shared_ptr<Inst> DivInst::getOperand1() { return m_operand1; }
std::shared_ptr<Inst> DivInst::getOperand2() { return m_operand2; }
std::string DivInst::getString()
{
    auto target = m_target->getTarget()->getString();

    std::string res = target + " <- Div(";
    for (int i = 0; i < m_operands.size(); ++i)
    {
        if (i)
        {
            res += ", ";
        }
        res += m_operands[i]->getTarget()->getString();
    }
    res += ")";
    return res;
}
void DivInst::push_user(std::shared_ptr<Inst> user)
{
    m_users.push_back(user);
}
void DivInst::setup_def_use()
{
    m_operand1->push_user(shared_from_this());
    m_operand2->push_user(shared_from_this());
}
std::vector<std::shared_ptr<Inst>>& DivInst::getOperands()
{
    return m_operands;
}
void DivInst::setTarget(std::shared_ptr<Inst> target) { m_target = target; }
std::shared_ptr<BasicBlock> DivInst::getBlock() { return m_block; };
InstType DivInst::getInstType() const { return InstType::Div; }

NotInst::NotInst(std::shared_ptr<Inst> target, std::shared_ptr<Inst> operand,
                 std::shared_ptr<BasicBlock> block)
    : m_target(std::move(target)),
      m_operand(std::move(operand)), m_block(std::move(block))
{
    m_operands = std::vector<std::shared_ptr<Inst>>{m_operand};
}
std::shared_ptr<Inst> NotInst::getTarget() { return m_target; }
std::shared_ptr<Inst> NotInst::getOperand() { return m_operand; }
std::string NotInst::getString()
{
    auto target = m_target->getTarget()->getString();

    std::string res = target + " <- Not(";
    for (int i = 0; i < m_operands.size(); ++i)
    {
        if (i)
        {
            res += ", ";
        }
        res += m_operands[i]->getTarget()->getString();
    }
    res += ")";
    return res;
}
void NotInst::push_user(std::shared_ptr<Inst> user)
{
    m_users.push_back(user);
}
void NotInst::setup_def_use() { m_operand->push_user(shared_from_this()); }
std::vector<std::shared_ptr<Inst>>& NotInst::getOperands()
{
    return m_operands;
}
void NotInst::setTarget(std::shared_ptr<Inst> target) { m_target = target; }
std::shared_ptr<BasicBlock> NotInst::getBlock() { return m_block; };
InstType NotInst::getInstType() const { return InstType::Not; }

AndInst::AndInst(std::shared_ptr<Inst> target, std::shared_ptr<Inst> operand1,
                 std::shared_ptr<Inst> operand2,
                 std::shared_ptr<BasicBlock> block)
    : m_target(std::move(target)),
      m_operand1(std::move(operand1)),
      m_operand2(std::move(operand2)),
      m_block(std::move(block))
{
    m_operands = std::vector<std::shared_ptr<Inst>>{m_operand1, m_operand2};
}
std::shared_ptr<Inst> AndInst::getTarget() { return m_target; }
std::shared_ptr<Inst> AndInst::getOperand1() { return m_operand1; }
std::shared_ptr<Inst> AndInst::getOperand2() { return m_operand2; }
std::string AndInst::getString()
{
    auto target = m_target->getTarget()->getString();

    std::string res = target + " <- And(";
    for (int i = 0; i < m_operands.size(); ++i)
    {
        if (i)
        {
            res += ", ";
        }
        res += m_operands[i]->getTarget()->getString();
    }
    res += ")";
    return res;
}
void AndInst::push_user(std::shared_ptr<Inst> user)
{
    m_users.push_back(user);
}
void AndInst::setup_def_use()
{
    m_operand1->push_user(shared_from_this());
    m_operand2->push_user(shared_from_this());
}
std::vector<std::shared_ptr<Inst>>& AndInst::getOperands()
{
    return m_operands;
}
void AndInst::setTarget(std::shared_ptr<Inst> target) { m_target = target; }
std::shared_ptr<BasicBlock> AndInst::getBlock() { return m_block; };
InstType AndInst::getInstType() const { return InstType::And; }

OrInst::OrInst(std::shared_ptr<Inst> target, std::shared_ptr<Inst> operand1,
               std::shared_ptr<Inst> operand2,
               std::shared_ptr<BasicBlock> block)
    : m_target(std::move(target)),
      m_operand1(std::move(operand1)),
      m_operand2(std::move(operand2)),
      m_block(std::move(block))
{
    m_operands = std::vector<std::shared_ptr<Inst>>{m_operand1, m_operand1};
}
std::shared_ptr<Inst> OrInst::getTarget() { return m_target; }
std::shared_ptr<Inst> OrInst::getOperand1() { return m_operand1; }
std::shared_ptr<Inst> OrInst::getOperand2() { return m_operand2; }
std::string OrInst::getString()
{
    auto target = m_target->getTarget()->getString();

    std::string res = target + " <- Or(";
    for (int i = 0; i < m_operands.size(); ++i)
    {
        if (i)
        {
            res += ", ";
        }
        res += m_operands[i]->getTarget()->getString();
    }
    res += ")";
    return res;
}
void OrInst::push_user(std::shared_ptr<Inst> user)
{
    m_users.push_back(user);
}
void OrInst::setup_def_use()
{
    m_operand1->push_user(shared_from_this());
    m_operand2->push_user(shared_from_this());
}
std::vector<std::shared_ptr<Inst>>& OrInst::getOperands()
{
    return m_operands;
}
void OrInst::setTarget(std::shared_ptr<Inst> target) { m_target = target; }
std::shared_ptr<BasicBlock> OrInst::getBlock() { return m_block; };
InstType OrInst::getInstType() const { return InstType::Or; }

AllocaInst::AllocaInst(std::shared_ptr<Inst> target, Type type,
                       unsigned int size, std::shared_ptr<BasicBlock> block)
    : m_target(std::move(target)),
      m_type(type),
      m_size(size), m_block(block)
{
    if (m_type == Type::UNDEFINED)
    {
        throw std::runtime_error("Alloca type should not be undefined!");
    }
}
std::shared_ptr<Inst> AllocaInst::getTarget() { return m_target; }
Type AllocaInst::getType() { return m_type; }
unsigned int AllocaInst::getSize() { return m_size; }
std::string AllocaInst::getString()
{
    auto target = m_target->getTarget()->getString();
    auto type = m_type;
    auto size = m_size;
    if (type == Type::INTEGER)
    {
        return target + " <- Alloca(int, " + std::to_string(size) + ")";
    }
    else
    {
        return target + " <- Alloca(bool, " + std::to_string(size) + ")";
    }
}
void AllocaInst::push_user(std::shared_ptr<Inst> user)
{
    m_users.push_back(user);
}
void AllocaInst::setup_def_use() {}
std::vector<std::shared_ptr<Inst>>& AllocaInst::getOperands()
{
    return m_operands;
}
void AllocaInst::setTarget(std::shared_ptr<Inst> target) { m_target = target; }
std::shared_ptr<BasicBlock> AllocaInst::getBlock() { return m_block; };
InstType AllocaInst::getInstType() const { return InstType::Alloca; }

ArrAccessInst::ArrAccessInst(std::shared_ptr<Inst> target,
                             std::shared_ptr<Inst> source,
                             std::shared_ptr<Inst> index,
                             std::shared_ptr<BasicBlock> block, Type type)
    : m_target(std::move(target)),
      m_source(std::move(source)),
      m_index(std::move(index)),
      m_block(std::move(block)),
      m_type(type)
{
    m_operands = std::vector<std::shared_ptr<Inst>>{m_source, m_index};
}
std::shared_ptr<Inst> ArrAccessInst::getTarget() { return m_target; }
std::shared_ptr<Inst> ArrAccessInst::getSource() { return m_source; }
std::shared_ptr<Inst> ArrAccessInst::getIndex() { return m_index; }
std::string ArrAccessInst::getString()
{
    auto target = m_target->getTarget()->getString();

    std::string res = target + " <- Access(";
    for (int i = 0; i < m_operands.size(); ++i)
    {
        if (i)
        {
            res += ", ";
        }
        res += m_operands[i]->getTarget()->getString();
    }
    res += ")";
    return res;
}
void ArrAccessInst::push_user(std::shared_ptr<Inst> user)
{
    m_users.push_back(user);
}
void ArrAccessInst::setup_def_use()
{
    m_source->push_user(shared_from_this());
    m_index->push_user(shared_from_this());
}
std::vector<std::shared_ptr<Inst>>& ArrAccessInst::getOperands()
{
    return m_operands;
}
void ArrAccessInst::setTarget(std::shared_ptr<Inst> target)
{
    m_target = target;
}
std::shared_ptr<BasicBlock> ArrAccessInst::getBlock() { return m_block; };
InstType ArrAccessInst::getInstType() const { return InstType::ArrAccess; }

ArrUpdateInst::ArrUpdateInst(std::shared_ptr<Inst> target,
                             std::shared_ptr<Inst> source,
                             std::shared_ptr<Inst> index,
                             std::shared_ptr<Inst> val,
                             std::shared_ptr<BasicBlock> block, Type type)
    : m_target(std::move(target)),
      m_source(std::move(source)),
      m_index(std::move(index)),
      m_val(std::move(val)),
      m_block(std::move(block)),
      m_type(type)
{
    m_operands = std::vector<std::shared_ptr<Inst>>{m_source, m_index, m_val};
}
std::shared_ptr<Inst> ArrUpdateInst::getTarget() { return m_target; }
std::shared_ptr<Inst> ArrUpdateInst::getSource() { return m_source; }
std::shared_ptr<Inst> ArrUpdateInst::getIndex() { return m_index; }
std::shared_ptr<Inst> ArrUpdateInst::getVal() { return m_val; }
std::string ArrUpdateInst::getString()
{
    auto target = m_target->getTarget()->getString();

    std::string res = target + " <- Update(";
    for (int i = 0; i < m_operands.size(); ++i)
    {
        if (i)
        {
            res += ", ";
        }
        res += m_operands[i]->getTarget()->getString();
    }
    res += ")";
    return res;
}
void ArrUpdateInst::push_user(std::shared_ptr<Inst> user)
{
    m_users.push_back(user);
}
void ArrUpdateInst::setup_def_use()
{
    m_source->push_user(shared_from_this());
    m_index->push_user(shared_from_this());
    m_val->push_user(shared_from_this());
}
std::vector<std::shared_ptr<Inst>>& ArrUpdateInst::getOperands()
{
    return m_operands;
}
void ArrUpdateInst::setTarget(std::shared_ptr<Inst> target)
{
    m_target = target;
}
std::shared_ptr<BasicBlock> ArrUpdateInst::getBlock() { return m_block; };
InstType ArrUpdateInst::getInstType() const { return InstType::ArrUpdate; }

AssignInst ::AssignInst(std::shared_ptr<Inst> target,
                        std::shared_ptr<Inst> source,
                        std::shared_ptr<BasicBlock> block)
    : m_target(std::move(target)), m_source(std::move(source)), m_block(std::move(block))
{
    m_operands = std::vector<std::shared_ptr<Inst>>{m_source};
}
std::shared_ptr<Inst> AssignInst::getTarget() { return m_target; }
std::shared_ptr<Inst> AssignInst::getSource() { return m_source; }
std::string AssignInst::getString()
{
    auto target = m_target->getTarget()->getString();

    std::string res = target + " <- ";
    for (int i = 0; i < m_operands.size(); ++i)
    {
        if (i)
        {
            res += ", ";
        }
        res += m_operands[i]->getTarget()->getString();
    }
    return res;
}

void AssignInst::push_user(std::shared_ptr<Inst> user)
{
    m_users.push_back(user);
}
void AssignInst::setup_def_use() { m_source->push_user(shared_from_this()); }
std::vector<std::shared_ptr<Inst>>& AssignInst::getOperands()
{
    return m_operands;
}
void AssignInst::setTarget(std::shared_ptr<Inst> target)
{
    m_target = target;
}
std::shared_ptr<BasicBlock> AssignInst::getBlock() { return m_block; };
InstType AssignInst::getInstType() const { return InstType::Assign; }

CmpEQInst::CmpEQInst(std::shared_ptr<Inst> target,
                     std::shared_ptr<Inst> operand1,
                     std::shared_ptr<Inst> operand2,
                     std::shared_ptr<BasicBlock> block)
    : m_target(std::move(target)),
      m_operand1(std::move(operand1)),
      m_operand2(std::move(operand2)),
      m_block(std::move(block))
{
    m_operands = std::vector<std::shared_ptr<Inst>>{m_operand1, m_operand2};
}
std::shared_ptr<Inst> CmpEQInst::getTarget() { return m_target; }
std::shared_ptr<Inst> CmpEQInst::getOperand1() { return m_operand1; }
std::shared_ptr<Inst> CmpEQInst::getOperand2() { return m_operand2; }
std::string CmpEQInst::getString()
{
    auto target = m_target->getTarget()->getString();

    std::string res = target + " <- Cmp_EQ(";
    for (int i = 0; i < m_operands.size(); ++i)
    {
        if (i)
        {
            res += ", ";
        }
        res += m_operands[i]->getTarget()->getString();
    }
    res += ")";
    return res;
}
void CmpEQInst::push_user(std::shared_ptr<Inst> user)
{
    m_users.push_back(user);
}
void CmpEQInst::setup_def_use()
{
    m_operand1->push_user(shared_from_this());
    m_operand2->push_user(shared_from_this());
}
std::vector<std::shared_ptr<Inst>>& CmpEQInst::getOperands()
{
    return m_operands;
}
void CmpEQInst::setTarget(std::shared_ptr<Inst> target) { m_target = target; }
std::shared_ptr<BasicBlock> CmpEQInst::getBlock() { return m_block; };
InstType CmpEQInst::getInstType() const { return InstType::CmpEq; }

CmpNEInst::CmpNEInst(std::shared_ptr<Inst> target,
                     std::shared_ptr<Inst> operand1,
                     std::shared_ptr<Inst> operand2,
                     std::shared_ptr<BasicBlock> block)
    : m_target(std::move(target)),
      m_operand1(std::move(operand1)),
      m_operand2(std::move(operand2)),
      m_block(std::move(block))
{
    m_operands = std::vector<std::shared_ptr<Inst>>{m_operand1, m_operand2};
}
std::shared_ptr<Inst> CmpNEInst::getTarget() { return m_target; }
std::shared_ptr<Inst> CmpNEInst::getOperand1() { return m_operand1; }
std::shared_ptr<Inst> CmpNEInst::getOperand2() { return m_operand2; }
std::string CmpNEInst::getString()
{
    auto target = m_target->getTarget()->getString();

    std::string res = target + " <- Cmp_NE(";
    for (int i = 0; i < m_operands.size(); ++i)
    {
        if (i)
        {
            res += ", ";
        }
        res += m_operands[i]->getTarget()->getString();
    }
    res += ")";
    return res;
}

void CmpNEInst::push_user(std::shared_ptr<Inst> user)
{
    m_users.push_back(user);
}
void CmpNEInst::setup_def_use()
{
    m_operand1->push_user(shared_from_this());
    m_operand2->push_user(shared_from_this());
}
std::vector<std::shared_ptr<Inst>>& CmpNEInst::getOperands()
{
    return m_operands;
}
void CmpNEInst::setTarget(std::shared_ptr<Inst> target) { m_target = target; }
std::shared_ptr<BasicBlock> CmpNEInst::getBlock() { return m_block; };
InstType CmpNEInst::getInstType() const { return InstType::CmpNE; }

CmpLTInst::CmpLTInst(std::shared_ptr<Inst> target,
                     std::shared_ptr<Inst> operand1,
                     std::shared_ptr<Inst> operand2,
                     std::shared_ptr<BasicBlock> block)
    : m_target(std::move(target)),
      m_operand1(std::move(operand1)),
      m_operand2(std::move(operand2)),
      m_block(std::move(block))
{
    m_operands = std::vector<std::shared_ptr<Inst>>{m_operand1, m_operand2};
}
std::shared_ptr<Inst> CmpLTInst::getTarget() { return m_target; }
std::shared_ptr<Inst> CmpLTInst::getOperand1() { return m_operand1; }
std::shared_ptr<Inst> CmpLTInst::getOperand2() { return m_operand2; }
std::string CmpLTInst::getString()
{
    auto target = m_target->getTarget()->getString();

    std::string res = target + " <- Cmp_LT(";
    for (int i = 0; i < m_operands.size(); ++i)
    {
        if (i)
        {
            res += ", ";
        }
        res += m_operands[i]->getTarget()->getString();
    }
    res += ")";
    return res;
}
void CmpLTInst::push_user(std::shared_ptr<Inst> user)
{
    m_users.push_back(user);
}
void CmpLTInst::setup_def_use()
{
    m_operand1->push_user(shared_from_this());
    m_operand2->push_user(shared_from_this());
}
std::vector<std::shared_ptr<Inst>>& CmpLTInst::getOperands()
{
    return m_operands;
}
void CmpLTInst::setTarget(std::shared_ptr<Inst> target) { m_target = target; }
std::shared_ptr<BasicBlock> CmpLTInst::getBlock() { return m_block; };
InstType CmpLTInst::getInstType() const { return InstType::CmpLT; }

CmpLTEInst::CmpLTEInst(std::shared_ptr<Inst> target,
                       std::shared_ptr<Inst> operand1,
                       std::shared_ptr<Inst> operand2,
                       std::shared_ptr<BasicBlock> block)
    : m_target(std::move(target)),
      m_operand1(std::move(operand1)),
      m_operand2(std::move(operand2)),
      m_block(std::move(block))
{
    m_operands = std::vector<std::shared_ptr<Inst>>{m_operand1, m_operand2};
}
std::shared_ptr<Inst> CmpLTEInst::getTarget() { return m_target; }
std::shared_ptr<Inst> CmpLTEInst::getOperand1() { return m_operand1; }
std::shared_ptr<Inst> CmpLTEInst::getOperand2() { return m_operand2; }
std::string CmpLTEInst::getString()
{
    auto target = m_target->getTarget()->getString();

    std::string res = target + " <- Cmp_LTE(";
    for (int i = 0; i < m_operands.size(); ++i)
    {
        if (i)
        {
            res += ", ";
        }
        res += m_operands[i]->getTarget()->getString();
    }
    res += ")";
    return res;
}
void CmpLTEInst::push_user(std::shared_ptr<Inst> user)
{
    m_users.push_back(user);
}
void CmpLTEInst::setup_def_use()
{
    m_operand1->push_user(shared_from_this());
    m_operand2->push_user(shared_from_this());
}
std::vector<std::shared_ptr<Inst>>& CmpLTEInst::getOperands()
{
    return m_operands;
}
void CmpLTEInst::setTarget(std::shared_ptr<Inst> target) { m_target = target; }
std::shared_ptr<BasicBlock> CmpLTEInst::getBlock() { return m_block; };
InstType CmpLTEInst::getInstType() const { return InstType::CmpLTE; }

CmpGTInst::CmpGTInst(std::shared_ptr<Inst> target,
                     std::shared_ptr<Inst> operand1,
                     std::shared_ptr<Inst> operand2,
                     std::shared_ptr<BasicBlock> block)
    : m_target(std::move(target)),
      m_operand1(std::move(operand1)),
      m_operand2(std::move(operand2)),
      m_block(std::move(block))
{
    m_operands = std::vector<std::shared_ptr<Inst>>{m_operand1, m_operand2};
}
std::shared_ptr<Inst> CmpGTInst::getTarget() { return m_target; }
std::shared_ptr<Inst> CmpGTInst::getOperand1() { return m_operand1; }
std::shared_ptr<Inst> CmpGTInst::getOperand2() { return m_operand2; }
std::string CmpGTInst::getString()
{
    auto target = m_target->getTarget()->getString();

    std::string res = target + " <- Cmp_GT(";
    for (int i = 0; i < m_operands.size(); ++i)
    {
        if (i)
        {
            res += ", ";
        }
        res += m_operands[i]->getTarget()->getString();
    }
    res += ")";
    return res;
}
void CmpGTInst::push_user(std::shared_ptr<Inst> user)
{
    m_users.push_back(user);
}
void CmpGTInst::setup_def_use()
{
    m_operand1->push_user(shared_from_this());
    m_operand2->push_user(shared_from_this());
}
std::vector<std::shared_ptr<Inst>>& CmpGTInst::getOperands()
{
    return m_operands;
}
void CmpGTInst::setTarget(std::shared_ptr<Inst> target) { m_target = target; }
std::shared_ptr<BasicBlock> CmpGTInst::getBlock() { return m_block; };
InstType CmpGTInst::getInstType() const { return InstType::CmpGT; }

CmpGTEInst::CmpGTEInst(std::shared_ptr<Inst> target,
                       std::shared_ptr<Inst> operand1,
                       std::shared_ptr<Inst> operand2,
                       std::shared_ptr<BasicBlock> block)
    : m_target(std::move(target)),
      m_operand1(std::move(operand1)),
      m_operand2(std::move(operand2)),
      m_block(std::move(block))
{
    m_operands = std::vector<std::shared_ptr<Inst>>{m_operand1, m_operand2};
}
std::shared_ptr<Inst> CmpGTEInst::getTarget() { return m_target; }
std::shared_ptr<Inst> CmpGTEInst::getOperand1() { return m_operand1; }
std::shared_ptr<Inst> CmpGTEInst::getOperand2() { return m_operand2; }
std::string CmpGTEInst::getString()
{
    auto target = m_target->getTarget()->getString();

    std::string res = target + " <- Cmp_GTE(";
    for (int i = 0; i < m_operands.size(); ++i)
    {
        if (i)
        {
            res += ", ";
        }
        res += m_operands[i]->getTarget()->getString();
    }
    res += ")";
    return res;
}

void CmpGTEInst::push_user(std::shared_ptr<Inst> user)
{
    m_users.push_back(user);
}
void CmpGTEInst::setup_def_use()
{
    m_operand1->push_user(shared_from_this());
    m_operand2->push_user(shared_from_this());
}
std::vector<std::shared_ptr<Inst>>& CmpGTEInst::getOperands()
{
    return m_operands;
}
void CmpGTEInst::setTarget(std::shared_ptr<Inst> target) { m_target = target; }
std::shared_ptr<BasicBlock> CmpGTEInst::getBlock() { return m_block; };
InstType CmpGTEInst::getInstType() const { return InstType::CmpGTE; }

JumpInst::JumpInst(std::shared_ptr<BasicBlock> target)
    : m_target(std::move(target))
{
}
std::shared_ptr<BasicBlock> JumpInst::getJumpTarget() { return m_target; }
std::string JumpInst::getString()
{
    auto target = m_target->getName();
    return "Jump " + target;
}
void JumpInst::push_user(std::shared_ptr<Inst> user)
{
    m_users.push_back(user);
}
void JumpInst::setup_def_use() {}
std::vector<std::shared_ptr<Inst>>& JumpInst::getOperands()
{
    return m_operands;
}
std::shared_ptr<BasicBlock> JumpInst::getBlock() { return m_block; };
InstType JumpInst::getInstType() const { return InstType::Jump; }

BRTInst::BRTInst(std::shared_ptr<Inst> cond,
                 std::shared_ptr<BasicBlock> targetSuccess,
                 std::shared_ptr<BasicBlock> targetFailed,
                 std::shared_ptr<BasicBlock> block)
    : m_targetSuccess(std::move(targetSuccess)),
      m_targetFailed(std::move(targetFailed)),
      m_block(std::move(block))
{
    m_operands = std::vector<std::shared_ptr<Inst>>{cond};
}
std::shared_ptr<Inst> BRTInst::getCond() { return m_operands[0]; }
std::shared_ptr<BasicBlock> BRTInst::getTargetSuccess() { return m_targetSuccess; }
std::shared_ptr<BasicBlock> BRTInst::getTargetFailed() { return m_targetFailed; }
std::string BRTInst::getString()
{
    auto targetSuccess = m_targetSuccess->getName();
    auto targetFailed = m_targetFailed->getName();

    return "BRT(" + m_operands[0]->getTarget()->getString() + ", " +
           targetSuccess + ", " +
           targetFailed +
           ")";
}
void BRTInst::push_user(std::shared_ptr<Inst> user)
{
    m_users.push_back(user);
}
void BRTInst::setup_def_use() { m_operands[0]->push_user(shared_from_this()); }
std::vector<std::shared_ptr<Inst>>& BRTInst::getOperands()
{
    return m_operands;
}
std::shared_ptr<BasicBlock> BRTInst::getBlock() { return m_block; };
InstType BRTInst::getInstType() const { return InstType::BRT; }

BRFInst::BRFInst(std::shared_ptr<Inst> cond,
                 std::shared_ptr<BasicBlock> targetSuccess,
                 std::shared_ptr<BasicBlock> targetFailed,
                 std::shared_ptr<BasicBlock> block)
    : m_targetSuccess(std::move(targetSuccess)),
      m_targetFailed(std::move(targetFailed)),
      m_block(std::move(block))
{
    m_operands = std::vector<std::shared_ptr<Inst>>{cond};
}
std::shared_ptr<Inst> BRFInst::getCond() { return m_operands[0]; }
std::shared_ptr<BasicBlock> BRFInst::getTargetSuccess() { return m_targetSuccess; }
std::shared_ptr<BasicBlock> BRFInst::getTargetFailed() { return m_targetFailed; }
std::string BRFInst::getString()
{
    auto targetSuccess = m_targetSuccess->getName();
    auto targetFailed = m_targetFailed->getName();

    return "BRT(" + m_operands[0]->getTarget()->getString() + ", " +
           targetSuccess + ", " + targetFailed + ")";
}
void BRFInst::push_user(std::shared_ptr<Inst> user)
{
    m_users.push_back(user);
}
void BRFInst::setup_def_use() { m_operands[0]->push_user(shared_from_this()); }
std::vector<std::shared_ptr<Inst>>& BRFInst::getOperands()
{
    return m_operands;
}
std::shared_ptr<BasicBlock> BRFInst::getBlock() { return m_block; };
InstType BRFInst::getInstType() const { return InstType::BRF; }

PutInst::PutInst(std::shared_ptr<Inst> operand,
                 std::shared_ptr<BasicBlock> block)
    : m_block(std::move(block))
{
    m_operands = std::vector<std::shared_ptr<Inst>>{operand};
}
std::shared_ptr<Inst> PutInst::getOperand() { return m_operands[0]; }
std::string PutInst::getString()
{
    auto& operand = m_operands[0]->getTarget()->getString();
    return "Put(" + operand + ")";
}
void PutInst::push_user(std::shared_ptr<Inst> user)
{
    m_users.push_back(user);
}
void PutInst::setup_def_use() { m_operands[0]->push_user(shared_from_this()); }
std::vector<std::shared_ptr<Inst>>& PutInst::getOperands()
{
    return m_operands;
}
std::shared_ptr<BasicBlock> PutInst::getBlock() { return m_block; };
InstType PutInst::getInstType() const { return InstType::Put; }

GetInst::GetInst(std::shared_ptr<Inst> target,
                 std::shared_ptr<BasicBlock> block)
    : m_target(std::move(target)), m_block(std::move(block))
{
}
std::shared_ptr<Inst> GetInst::getTarget() { return m_target; }
std::string GetInst::getString()
{
    auto target = m_target->getTarget()->getString();
    return target + " <- Get()";
}
void GetInst::push_user(std::shared_ptr<Inst> user)
{
    m_users.push_back(user);
}
void GetInst::setup_def_use() {}
std::vector<std::shared_ptr<Inst>>& GetInst::getOperands()
{
    return m_operands;
}
void GetInst::setTarget(std::shared_ptr<Inst> target) { m_target = target; }
std::shared_ptr<BasicBlock> GetInst::getBlock() { return m_block; };
InstType GetInst::getInstType() const { return InstType::Get; }

PushInst::PushInst(std::shared_ptr<Inst> operand,
                   std::shared_ptr<BasicBlock> block)
    : m_block(std::move(block))
{
    m_operands = std::vector<std::shared_ptr<Inst>>{operand};
}
std::shared_ptr<Inst> PushInst::getOperand() { return m_operands[0]; }
std::string PushInst::getString()
{
    auto operand = m_operands[0]->getTarget()->getString();
    return "Push(" + operand + ")";
}
void PushInst::push_user(std::shared_ptr<Inst> user)
{
    m_users.push_back(user);
}
void PushInst::setup_def_use() { m_operands[0]->push_user(shared_from_this()); }
std::vector<std::shared_ptr<Inst>>& PushInst::getOperands()
{
    return m_operands;
}
std::shared_ptr<BasicBlock> PushInst::getBlock() { return m_block; };
InstType PushInst::getInstType() const { return InstType::Push; }

PopInst::PopInst(std::shared_ptr<Inst> target,
                 std::shared_ptr<BasicBlock> block)
    : m_target(std::move(target)), m_block(std::move(block))
{
}
std::shared_ptr<Inst> PopInst::getTarget() { return m_target; }
std::string PopInst::getString()
{
    auto target = m_target->getTarget()->getString();
    return target + " <- Pop()";
}
void PopInst::push_user(std::shared_ptr<Inst> user)
{
    m_users.push_back(user);
}
void PopInst::setup_def_use() {}
std::vector<std::shared_ptr<Inst>>& PopInst::getOperands()
{
    return m_operands;
}
void PopInst::setTarget(std::shared_ptr<Inst> target) { m_target = target; }
std::shared_ptr<BasicBlock> PopInst::getBlock() { return m_block; };
InstType PopInst::getInstType() const { return InstType::Pop; }

ReturnInst::ReturnInst(std::shared_ptr<BasicBlock> block)
    : m_block(std::move(block))
{
}
std::string ReturnInst::getString()
{
    return "Return";
}
void ReturnInst::push_user(std::shared_ptr<Inst> user)
{
    m_users.push_back(user);
}
void ReturnInst::setup_def_use() {}
std::vector<std::shared_ptr<Inst>>& ReturnInst::getOperands()
{
    return m_operands;
}
std::shared_ptr<BasicBlock> ReturnInst::getBlock() { return m_block; };
InstType ReturnInst::getInstType() const { return InstType::Return; }

FuncSignature::FuncSignature(std::string funcName, FType fType, Type retType,
    std::vector<std::shared_ptr<IdentifierAST>> parameters,
    std::shared_ptr<BasicBlock> block)
    :   m_funcName(std::move(funcName)),
        m_fType(fType),
        m_retType(retType),
        m_parameters(std::move(parameters)),
        m_block(std::move(block))
{
}
std::string FuncSignature::getFuncName() { return m_funcName; }
std::string FuncSignature::getString()
{
    std::string res;
    if (m_fType == FType::PROC)
    {
        res += "proc ";
    }
    else
    {
        res += "func ";
    }
    res += m_funcName + "(";

    for (unsigned int i = 0; i < m_parameters.size(); ++i)
    {
        if (i)
        {
            res += ", ";
        }
        std::shared_ptr<IdentifierAST> param = m_parameters[i];
        std::string& paramName = param->getName();
        res += paramName + " : ";
        if (param->getType() == Type::INTEGER)
        {
            res += "integer";
        }
        else if (param->getType() == Type::BOOLEAN)
        {
            res += "boolean";
        }
        else
        {
            throw std::runtime_error("parameter should not have undefined type!");
        }
    }

    res += ")";
    return res;
}
void FuncSignature::push_user(std::shared_ptr<Inst> user)
{
    throw std::runtime_error("function signature should not use anything!\n");
}
void FuncSignature::setup_def_use()
{
    // do nothing, since function signature did not use anything
}
std::shared_ptr<BasicBlock> FuncSignature::getBlock() { return m_block; }
InstType FuncSignature::getInstType() const { return InstType::FuncSignature; }

LowerFunc::LowerFunc(std::string funcName, FType fType, Type retType,
    std::vector<std::shared_ptr<IdentifierAST>> parameters,
    std::shared_ptr<BasicBlock> block)
    :   m_funcName(std::move(funcName)),
        m_fType(fType),
        m_retType(retType),
        m_parameters(std::move(parameters)),
        m_block(std::move(block))
{
}
std::string LowerFunc::getFuncName() { return m_funcName; }
std::vector<std::shared_ptr<IdentifierAST>>& LowerFunc::getParameters()
{
    return m_parameters;
}
std::string LowerFunc::getString()
{
    std::string res = "lower ";
    if (m_fType == FType::PROC)
    {
        res += "proc ";
    }
    else
    {
        res += "func ";
    }
    res += m_funcName + "(";

    for (unsigned int i = 0; i < m_parameters.size(); ++i)
    {
        if (i)
        {
            res += ", ";
        }
        std::shared_ptr<IdentifierAST> param = m_parameters[i];
        std::string& paramName = param->getName();
        res += paramName + " : ";
        if (param->getType() == Type::INTEGER)
        {
            res += "integer";
        }
        else if (param->getType() == Type::BOOLEAN)
        {
            res += "boolean";
        }
        else
        {
            throw std::runtime_error("parameter should not have undefined type!");
        }
    }

    res += ")";
    return res;
}
void LowerFunc::push_user(std::shared_ptr<Inst> user)
{
    throw std::runtime_error("function signature should not use anything!\n");
}
void LowerFunc::setup_def_use()
{
    // do nothing, since function signature did not use anything
}
std::shared_ptr<BasicBlock> LowerFunc::getBlock() { return m_block; }
InstType LowerFunc::getInstType() const { return InstType::LowerFunc; }

CallInst::CallInst(std::string calleeStr,
                   std::vector<std::shared_ptr<Inst>> operands,
                   std::shared_ptr<BasicBlock> block)
    : m_calleeStr(std::move(calleeStr)), m_operands(std::move(operands)), m_block(std::move(block))
{
}
std::string CallInst::getCalleeStr() { return m_calleeStr; }
std::string CallInst::getString()
{

    std::string res = m_calleeStr + "(";
    for (int i = 0; i < m_operands.size(); ++i)
    {
        if (i)
        {
            res += ", ";
        }
        res += m_operands[i]->getTarget()->getString();
    }
    res += ")";
    return res;
}
void CallInst::push_user(std::shared_ptr<Inst> user)
{
    m_users.push_back(user);
}
void CallInst::setup_def_use() {}
std::vector<std::shared_ptr<Inst>>& CallInst::getOperands()
{
    return m_operands;
}
std::shared_ptr<BasicBlock> CallInst::getBlock() { return m_block; };
InstType CallInst::getInstType() const { return InstType::Call; }

PhiInst::PhiInst(std::string name, std::shared_ptr<BasicBlock> block)
    : m_target(std::make_shared<IdentInst>(name, block)), m_block(std::move(block))
{
}
void PhiInst::appendOperand(std::shared_ptr<Inst> operand)
{
    m_operands.push_back(operand);
}
std::shared_ptr<Inst> PhiInst::getTarget() { return m_target; }
std::shared_ptr<BasicBlock> PhiInst::getBlock() { return m_block; }
std::string PhiInst::getString()
{
    auto res = m_target->getTarget()->getString();
    res += " <- Phi(";
    for (unsigned int i=0; i<m_operands.size(); ++i)
    {
        if (i)
        {
            res += ", ";
        }
        if (m_operands[i] == nullptr)
        {
            res += "Undefined";
            continue;
        }
        res += m_operands[i]->getTarget()->getString();
    }
    res += ")";
    return res;
}
void PhiInst::push_user(std::shared_ptr<Inst> user)
{
    m_users.push_back(user);
}
std::vector<std::shared_ptr<Inst>>& PhiInst::get_users()
{
    return m_users;
}
void PhiInst::setup_def_use() {}
std::vector<std::shared_ptr<Inst>>& PhiInst::getOperands()
{
    return m_operands;
}
bool PhiInst::isPhi() { return true; }
void PhiInst::setTarget(std::shared_ptr<Inst> target) { m_target = target; }
InstType PhiInst::getInstType() const { return InstType::Phi; }

std::string UndefInst::getString() { return "Undef"; }
std::shared_ptr<BasicBlock> UndefInst::getBlock() { return m_block; };
InstType UndefInst::getInstType() const { return InstType::Undef; }

std::string NoopInst::getString() { return "noop"; }
std::shared_ptr<BasicBlock> NoopInst::getBlock() { return m_block; };
InstType NoopInst::getInstType() const { return InstType::Noop; }
