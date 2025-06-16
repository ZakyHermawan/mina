#include "Inst.hpp"
#include "BasicBlock.hpp"

#include <stdexcept>

IntConstInst::IntConstInst(int val) : m_val(val)
{
}
int IntConstInst::getVal() { return m_val; }
std::string IntConstInst::getString()
{
    //return "IntConst(" + std::to_string(m_val) + ")";
    return std::to_string(m_val);
}

BoolConstInst::BoolConstInst(bool val) : m_val(val) {}
bool BoolConstInst::getVal() { return m_val; }
std::string BoolConstInst::getString()
{
    //return "BoolConst(" + std::to_string(m_val) + ")";
    if (m_val)
    {
        return "true";
    }
    else
    {
        return "false";
    }
}

StrConstInst::StrConstInst(std::string val) : m_val(val) {}
std::string StrConstInst::getString()
{
    //return "StrConst(" + m_val + ")";
    return m_val;
}


IdentInst::IdentInst(std::string name) : m_name(std::move(name)) {}
std::string IdentInst::getString() { return m_name; }

AddInst::AddInst(std::shared_ptr<Inst> target, std::shared_ptr<Inst> operand1,
    std::shared_ptr<Inst> operand2)
    : m_target(std::move(target)),
    m_operand1(std::move(operand1)),
    m_operand2(std::move(operand2))
{
}
std::shared_ptr<Inst> AddInst::getTarget() {return m_target;}
std::shared_ptr<Inst> AddInst::getOperand1() {return m_operand1;}
std::shared_ptr<Inst> AddInst::getOperand2() { return m_operand2; }
std::string AddInst::getString()
{
    auto target = m_target->getTarget()->getString();
    auto operand1 = m_operand1->getTarget()->getString();
    auto operand2 = m_operand2->getTarget()->getString();

    return target + " <- Add(" + operand1 + ", " + operand2 + ")";
}

SubInst::SubInst(std::shared_ptr<Inst> target, std::shared_ptr<Inst> operand1,
    std::shared_ptr<Inst> operand2)
    : m_target(std::move(target)),
    m_operand1(std::move(operand1)),
    m_operand2(std::move(operand2))
{
}
std::shared_ptr<Inst> SubInst::getTarget() { return m_target; }
std::shared_ptr<Inst> SubInst::getOperand1() { return m_operand1; }
std::shared_ptr<Inst> SubInst::getOperand2() { return m_operand2; }
std::string SubInst::getString()
{
    auto target = m_target->getTarget()->getString();
    auto operand1 = m_operand1->getTarget()->getString();
    auto operand2 = m_operand2->getTarget()->getString();

    return target + " <- Sub(" + operand1 + ", " + operand2 + ")";
}

MulInst::MulInst(std::shared_ptr<Inst> target, std::shared_ptr<Inst> operand1,
    std::shared_ptr<Inst> operand2)
    : m_target(std::move(target)),
    m_operand1(std::move(operand1)),
    m_operand2(std::move(operand2))
{
}
std::shared_ptr<Inst> MulInst::getTarget() { return m_target; }
std::shared_ptr<Inst> MulInst::getOperand1() { return m_operand1; }
std::shared_ptr<Inst> MulInst::getOperand2() { return m_operand2; }
std::string MulInst::getString()
{
    auto target = m_target->getTarget()->getString();
    auto operand1 = m_operand1->getTarget()->getString();
    auto operand2 = m_operand2->getTarget()->getString();

    return target + " <- Mul(" + operand1 + ", " + operand2 + ")";
}

DivInst::DivInst(std::shared_ptr<Inst> target, std::shared_ptr<Inst> operand1,
    std::shared_ptr<Inst> operand2)
    : m_target(std::move(target)),
    m_operand1(std::move(operand1)),
    m_operand2(std::move(operand2))
{
}
std::shared_ptr<Inst> DivInst::getTarget() { return m_target; }
std::shared_ptr<Inst> DivInst::getOperand1() { return m_operand1; }
std::shared_ptr<Inst> DivInst::getOperand2() { return m_operand2; }
std::string DivInst::getString()
{
    auto target = m_target->getTarget()->getString();
    auto operand1 = m_operand1->getTarget()->getString();
    auto operand2 = m_operand2->getTarget()->getString();


    return target + " <- Div(" + operand1 + ", " + operand2 + ")";
}

NotInst::NotInst(std::shared_ptr<Inst> target, std::shared_ptr<Inst> operand)
    : m_target(std::move(target)),
    m_operand(std::move(operand))
{
}
std::shared_ptr<Inst> NotInst::getTarget() { return m_target; }
std::shared_ptr<Inst> NotInst::getOperand() { return m_operand; }
std::string NotInst::getString()
{
    auto target = m_target->getTarget()->getString();
    auto operand = m_operand->getTarget()->getString();

    return target + " <- Not(" + operand + ")";
}

AndInst::AndInst(std::shared_ptr<Inst> target, std::shared_ptr<Inst> operand1,
    std::shared_ptr<Inst> operand2)
    : m_target(std::move(target)),
    m_operand1(std::move(operand1)),
    m_operand2(std::move(operand2))
{
}
std::shared_ptr<Inst> AndInst::getTarget() { return m_target; }
std::shared_ptr<Inst> AndInst::getOperand1() { return m_operand1; }
std::shared_ptr<Inst> AndInst::getOperand2() { return m_operand2; }
std::string AndInst::getString()
{
    auto target = m_target->getTarget()->getString();
    auto operand1 = m_operand1->getTarget()->getString();
    auto operand2 = m_operand2->getTarget()->getString();

    return target + " <- And(" + operand1 + ", " + operand2 + ")";
}

OrInst::OrInst(std::shared_ptr<Inst> target, std::shared_ptr<Inst> operand1,
    std::shared_ptr<Inst> operand2)
    : m_target(std::move(target)),
    m_operand1(std::move(operand1)),
    m_operand2(std::move(operand2))
{
}
std::shared_ptr<Inst> OrInst::getTarget() { return m_target; }
std::shared_ptr<Inst> OrInst::getOperand1() { return m_operand1; }
std::shared_ptr<Inst> OrInst::getOperand2() { return m_operand2; }
std::string OrInst::getString()
{
    auto target = m_target->getTarget()->getString();
    auto operand1 = m_operand1->getTarget()->getString();
    auto operand2 = m_operand2->getTarget()->getString();

    return target + " <- Or(" + operand1 + ", " + operand2 + ")";
}

AllocaInst::AllocaInst(std::shared_ptr<Inst> target, Type type,
                       unsigned int size)
    : m_target(std::move(target)),
    m_type(type),
    m_size(size)
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

ArrAccessInst::ArrAccessInst(std::shared_ptr<Inst> target,
                             std::shared_ptr<Inst> source,
                             std::shared_ptr<Inst> index)
    : m_target(std::move(target)),
    m_source(std::move(source)),
    m_index(std::move(index))
{
}
std::shared_ptr<Inst> ArrAccessInst::getTarget() { return m_target; }
std::shared_ptr<Inst> ArrAccessInst::getSource() { return m_source; }
std::shared_ptr<Inst> ArrAccessInst::getIndex() { return m_index; }
std::string ArrAccessInst::getString()
{
    auto target = m_target->getTarget()->getString();
    auto source = m_source->getTarget()->getString();
    auto index = m_index->getTarget()->getString();

    return target + " <- Access(" + source + ", " + index + ")";
}

ArrUpdateInst::ArrUpdateInst(std::shared_ptr<Inst> target,
                             std::shared_ptr<Inst> source,
                             std::shared_ptr<Inst> index,
                             std::shared_ptr<Inst> val)
    : m_target(std::move(target)),
      m_source(std::move(source)),
      m_index(std::move(index)),
      m_val(std::move(val))
{
}
std::shared_ptr<Inst> ArrUpdateInst::getTarget() { return m_target; }
std::shared_ptr<Inst> ArrUpdateInst::getSource() { return m_source; }
std::shared_ptr<Inst> ArrUpdateInst::getIndex() { return m_index; }
std::shared_ptr<Inst> ArrUpdateInst::getVal() { return m_val; }
std::string ArrUpdateInst::getString()
{
    auto target = m_target->getTarget()->getString();
    auto source = m_source->getTarget()->getString();
    auto index = m_index->getTarget()->getString();
    auto val = m_val->getTarget()->getString();

    return target + " <- Update(" + source + ", " + index + ", " + val + ")";
}

AssignInst ::AssignInst(std::shared_ptr<Inst> target,
                      std::shared_ptr<Inst> source)
    : m_target(std::move(target)), m_source(std::move(source))
{
}
std::shared_ptr<Inst> AssignInst::getTarget() { return m_target; }
std::shared_ptr<Inst> AssignInst::getSource() { return m_source; }
std::string AssignInst::getString()
{
    auto target = m_target->getTarget()->getString();
    auto source = m_source->getTarget()->getString();

    return target + " <- " + source;
}

CmpEQInst::CmpEQInst(std::shared_ptr<Inst> target,
                     std::shared_ptr<Inst> operand1,
    std::shared_ptr<Inst> operand2)
    : m_target(std::move(target)),
    m_operand1(std::move(operand1)),
    m_operand2(std::move(operand2))
{
}
std::shared_ptr<Inst> CmpEQInst::getTarget() { return m_target; }
std::shared_ptr<Inst> CmpEQInst::getOperand1() { return m_operand1; }
std::shared_ptr<Inst> CmpEQInst::getOperand2() { return m_operand2; }
std::string CmpEQInst::getString()
{
    auto target = m_target->getTarget()->getString();
    auto operand1 = m_operand1->getTarget()->getString();
    auto operand2 = m_operand2->getTarget()->getString();

    return target + " <- Cmp_EQ(" + operand1 + ", " + operand2 + ")";
}

CmpNEInst::CmpNEInst(std::shared_ptr<Inst> target,
                     std::shared_ptr<Inst> operand1,
    std::shared_ptr<Inst> operand2)
    : m_target(std::move(target)),
    m_operand1(std::move(operand1)),
    m_operand2(std::move(operand2))
{
}
std::shared_ptr<Inst> CmpNEInst::getTarget() { return m_target; }
std::shared_ptr<Inst> CmpNEInst::getOperand1() { return m_operand1; }
std::shared_ptr<Inst> CmpNEInst::getOperand2() { return m_operand2; }
std::string CmpNEInst::getString()
{
    auto target = m_target->getTarget()->getString();
    auto operand1 = m_operand1->getTarget()->getString();
    auto operand2 = m_operand2->getTarget()->getString();

    return target + " <- Cmp_NE(" + operand1 + ", " + operand2 + ")";
}

CmpLTInst::CmpLTInst(std::shared_ptr<Inst> target,
                     std::shared_ptr<Inst> operand1,
    std::shared_ptr<Inst> operand2)
    : m_target(std::move(target)),
    m_operand1(std::move(operand1)),
    m_operand2(std::move(operand2))
{
}
std::shared_ptr<Inst> CmpLTInst::getTarget() { return m_target; }
std::shared_ptr<Inst> CmpLTInst::getOperand1() { return m_operand1; }
std::shared_ptr<Inst> CmpLTInst::getOperand2() { return m_operand2; }
std::string CmpLTInst::getString()
{
    auto target = m_target->getTarget()->getString();
    auto operand1 = m_operand1->getTarget()->getString();
    auto operand2 = m_operand2->getTarget()->getString();

    return target + " <- Cmp_LT(" + operand1 + ", " + operand2 + ")";
}

CmpLTEInst::CmpLTEInst(std::shared_ptr<Inst> target,
                       std::shared_ptr<Inst> operand1,
    std::shared_ptr<Inst> operand2)
    : m_target(std::move(target)),
    m_operand1(std::move(operand1)),
    m_operand2(std::move(operand2))
{
}
std::shared_ptr<Inst> CmpLTEInst::getTarget() { return m_target; }
std::shared_ptr<Inst> CmpLTEInst::getOperand1() { return m_operand1; }
std::shared_ptr<Inst> CmpLTEInst::getOperand2() { return m_operand2; }
std::string CmpLTEInst::getString()
{
    auto target = m_target->getTarget()->getString();
    auto operand1 = m_operand1->getTarget()->getString();
    auto operand2 = m_operand2->getTarget()->getString();

    return target + " <- Cmp_LTE(" + operand1 + ", " + operand2 + ")";
}

CmpGTInst::CmpGTInst(std::shared_ptr<Inst> target,
                     std::shared_ptr<Inst> operand1,
    std::shared_ptr<Inst> operand2)
    : m_target(std::move(target)),
    m_operand1(std::move(operand1)),
    m_operand2(std::move(operand2))
{
}
std::shared_ptr<Inst> CmpGTInst::getTarget() { return m_target; }
std::shared_ptr<Inst> CmpGTInst::getOperand1() { return m_operand1; }
std::shared_ptr<Inst> CmpGTInst::getOperand2() { return m_operand2; }
std::string CmpGTInst::getString()
{
    auto target = m_target->getTarget()->getString();
    auto operand1 = m_operand1->getTarget()->getString();
    auto operand2 = m_operand2->getTarget()->getString();

    return target + " <- Cmp_GT(" + operand1 + ", " + operand2 + ")";
}

CmpGTEInst::CmpGTEInst(std::shared_ptr<Inst> target,
                       std::shared_ptr<Inst> operand1,
             std::shared_ptr<Inst> operand2)
    : m_target(std::move(target)),
      m_operand1(std::move(operand1)),
      m_operand2(std::move(operand2))
{
}
std::shared_ptr<Inst> CmpGTEInst::getTarget() { return m_target; }
std::shared_ptr<Inst> CmpGTEInst::getOperand1() { return m_operand1; }
std::shared_ptr<Inst> CmpGTEInst::getOperand2() { return m_operand2; }
std::string CmpGTEInst::getString()
{
    auto target = m_target->getTarget()->getString();
    auto operand1 = m_operand1->getTarget()->getString();
    auto operand2 = m_operand2->getTarget()->getString();

    return target + " <- Cmp_GTE(" + operand1 + ", " + operand2 + ")";
}

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

BRTInst::BRTInst(std::shared_ptr<Inst> cond,
                 std::shared_ptr<BasicBlock> targetSuccess,
                 std::shared_ptr<BasicBlock> targetFailed)
    : m_cond(std::move(cond)),
      m_targetSuccess(std::move(targetSuccess)),
      m_targetFailed(std::move(targetFailed))
{
}
std::shared_ptr<Inst> BRTInst::getCond() { return m_cond; }
std::shared_ptr<BasicBlock> BRTInst::getTargetSuccess() { return m_targetSuccess; }
std::shared_ptr<BasicBlock> BRTInst::getTargetFailed() { return m_targetFailed; }
std::string BRTInst::getString()
{
    auto cond = m_cond->getTarget()->getString();
    auto targetSuccess = m_targetSuccess->getName();
    auto targetFailed = m_targetFailed->getName();

    return "BRT(" + cond + ", " + targetSuccess + ", " + targetFailed + ")";
}

BRFInst::BRFInst(std::shared_ptr<Inst> cond,
                 std::shared_ptr<BasicBlock> targetSuccess,
                 std::shared_ptr<BasicBlock> targetFailed)
    : m_cond(std::move(cond)),
      m_targetSuccess(std::move(targetSuccess)),
      m_targetFailed(std::move(targetFailed))
{
}
std::shared_ptr<Inst> BRFInst::getCond() { return m_cond; }
std::shared_ptr<BasicBlock> BRFInst::getTargetSuccess() { return m_targetSuccess; }
std::shared_ptr<BasicBlock> BRFInst::getTargetFailed() { return m_targetFailed; }
std::string BRFInst::getString()
{
    auto cond = m_cond->getTarget()->getString();
    auto targetSuccess = m_targetSuccess->getName();
    auto targetFailed = m_targetFailed->getName();

    return "BRF(" + cond + ", " + targetSuccess + ", " + targetFailed + ")";
}

PutInst::PutInst(std::shared_ptr<Inst> operand)
    : m_operand(std::move(operand))
{
}
std::shared_ptr<Inst> PutInst::getOperand() { return m_operand; }
std::string PutInst::getString()
{
    auto operand = m_operand->getTarget()->getString();
    return "Put(" + operand + ")";
}

GetInst::GetInst(std::shared_ptr<Inst> target)
    : m_target(std::move(target))
{
}
std::shared_ptr<Inst> GetInst::getTarget() { return m_target; }
std::string GetInst::getString()
{
    auto target = m_target->getTarget()->getString();
    return target + " <- Get()";
}

PushInst::PushInst(std::shared_ptr<Inst> operand)
    : m_operand(std::move(operand))
{
}
std::shared_ptr<Inst> PushInst::getOperand() { return m_operand; }
std::string PushInst::getString()
{
    auto operand = m_operand->getTarget()->getString();
    return "Push(" + operand + ")";
}

PopInst::PopInst(std::shared_ptr<Inst> target)
    : m_target(std::move(target))
{
}
std::shared_ptr<Inst> PopInst::getTarget() { return m_target; }
std::string PopInst::getString()
{
    auto target = m_target->getTarget()->getString();
    return target + " <- Pop()";
}

ReturnInst::ReturnInst()
{
}
std::string ReturnInst::getString()
{
    return "Return";
}

CallInst::CallInst(std::string calleeStr)
    : m_calleeStr(std::move(calleeStr))
{
}
std::string CallInst::getCalleeStr() { return m_calleeStr; }
std::string CallInst::getString() { return "call(" + m_calleeStr + ")"; }

PhiInst::PhiInst(std::string name, std::shared_ptr<BasicBlock> block)
    : m_target(std::make_shared<IdentInst>(name)), m_block(std::move(block))
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
        res += m_operands[i]->getTarget()->getString();
    }
    res += ")";
    return res;
}
