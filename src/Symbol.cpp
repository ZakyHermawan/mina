#include "Symbol.hpp"
#include "Types.hpp"

#include <vector>
#include <string>

Symbol::Symbol(std::string name)
    : m_name{std::move(name)},
      m_lexicalLevel{-1},
      m_orderNum{-1},
      m_type{Type::UNDEFINED}
{
}

void Symbol::setName(std::string name) { m_name = std::move(name); }
void Symbol::setLexicalLevel(int ll) { m_lexicalLevel = ll; }

void Symbol::setLLON(int ll, int on)
{
    m_lexicalLevel = ll;
    m_orderNum = on;
}

void Symbol::setType(Type type) { m_type = type; }
std::string Symbol::getName() const { return m_name; }
int Symbol::getLexicalLevel() const { return m_lexicalLevel; }
int Symbol::getOrderNum() const { return m_orderNum; }
std::string Symbol::getTypeStr() const { return typeToStr(m_type); }

Bucket::Bucket()
    : m_intVal{0},
      m_arr{arenaVectorInt()},
      m_stackAddr{0}
{
}

Bucket::Bucket(int val, int stackAddr)
    : m_arr{arenaVectorInt()},
      m_intVal{val},
      m_stackAddr{stackAddr}
{
}
Bucket::Bucket(arenaVectorInt &arr, int stackAddr)
    : m_arr{arr},
      m_intVal{0},
      m_stackAddr{stackAddr}
{
}

Bucket::Bucket(arenaVectorInt &&arr, int stackAddr)
    : m_arr{std::move(arr)},
      m_intVal{0},
      m_stackAddr{stackAddr}
{
}

void Bucket::setArrSize(unsigned int size) { m_arr.resize(size); }
size_t Bucket::getArrSize() const { return m_arr.size(); }
void Bucket::setIntVal(int val) { m_intVal = val; }
void Bucket::setArr(arenaVectorInt &arr) { m_arr = arr; }

void Bucket::setArrAtIdx(unsigned int idx, int val)
{
    validateIdx(idx);
    m_arr[idx] = val;
}

int Bucket::getArrAtIdx(unsigned int idx)
{
    validateIdx(idx);
    return m_arr[idx];
}

int Bucket::getStackAddr() const { return m_stackAddr; }
int Bucket::getVal() const { return m_intVal; }

void Bucket::validateIdx(int idx)
{
    if (idx < 0 || idx > (m_arr.size() - 1))
    {
        throw std::runtime_error("index on bucket is out of bound");
    }
}

FunctionBucket::FunctionBucket(
    std::vector<std::string, arena::Allocator<std::string>> &parameters,
    int returnValue)
    : m_parameters{parameters}, m_returnValue{returnValue}
{
    m_ftype = FType::FUNC;
}

FunctionBucket::FunctionBucket(
    std::vector<std::string, arena::Allocator<std::string>> &parameters)
    : m_parameters{parameters}, m_returnValue{0}, m_ftype{FType::PROC}
{
}

FunctionBucket::FunctionBucket(
    std::vector<std::string, arena::Allocator<std::string>> &&parameters,
    int returnValue)
    : m_parameters{std::move(parameters)}, m_returnValue{returnValue}
{
    m_ftype = FType::FUNC;
}

void FunctionBucket::setSymTab(std::string &identifier, Bucket &bucket)
{
    auto it = m_symTab.find(identifier);
    if (it != m_symTab.end())
    {
        throw std::runtime_error("symbol " + identifier + "already being defined");
    }
    m_symTab[identifier] = bucket;
}

Bucket& FunctionBucket::getSymTab(std::string &identifier)
{
    auto it = m_symTab.find(identifier);

    if (it == m_symTab.end())
    {
        throw std::runtime_error("symbol " + identifier + " not defined");
    }
    return m_symTab[identifier];
}

void FunctionBucket::setStartAddr(unsigned int startAddr) { m_start_addr = startAddr; }
void FunctionBucket::setEndtAddr(unsigned int endAddr) { m_end_addr = endAddr; }
void FunctionBucket::setLocalNumVar(unsigned int numVar) { m_localNumVar = numVar; }
unsigned int FunctionBucket::getStartAddr() const { return m_start_addr; }
unsigned int FunctionBucket::getEndAddr() const { return m_end_addr; }
unsigned int FunctionBucket::getLocalNumVar() const { return m_localNumVar; }
