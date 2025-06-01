#pragma once

#include "Types.hpp"

#include <string>
#include "arena_alloc.hpp"
#include <vector>
#include <unordered_map>
#include <stdexcept>

using arenaVectorInt = std::vector<int, arena::Allocator<int>>;

class Symbol
{
private:
    std::string m_name;
    int m_lexicalLevel;
    int m_orderNum;
    Type m_type;

 public:
    Symbol(std::string name); 
    Symbol() = default;
    Symbol(Symbol &&) = default;
    Symbol(const Symbol &) = default;
    Symbol &operator=(Symbol &&) = default;
    Symbol &operator=(const Symbol &) = default;
    ~Symbol() = default;
    
    void setName(std::string name);
    void setLexicalLevel(int ll);
    
    // set lexical level and order number
    void setLLON(int ll, int on);
    void setType(Type type);
    std::string getName() const;
    int getLexicalLevel() const;
    int getOrderNum() const;
    std::string getTypeStr() const;

};

class Bucket
{
private:
    int m_intVal;
    int m_stackAddr;  // relative to the stack frame on current lexical level
    arenaVectorInt m_arr;

    void validateIdx(int idx);

public:
    Bucket();
    ~Bucket() = default;
    
    Bucket(int val, int stackAddr);
    Bucket(arenaVectorInt &arr, int stackAddr);
    Bucket(arenaVectorInt &&arr, int stackAddr);
    
    void setArrSize(unsigned int size);
    size_t getArrSize() const;
    
    void setIntVal(int val);
    void setArr(arenaVectorInt &arr);
    void setArrAtIdx(unsigned int idx, int val);
    int getArrAtIdx(unsigned int idx);
    int getStackAddr() const;
    
    int getVal() const;
};

class FunctionBucket
{
private:
    std::vector<std::string, arena::Allocator<std::string>> m_parameters;
    int m_returnValue;
    FType m_ftype;
    std::unordered_map<std::string, Bucket> m_symTab;
    
    unsigned int m_start_addr;
    unsigned int m_end_addr;
    unsigned int m_localNumVar;

public:
    FunctionBucket(
        std::vector<std::string, arena::Allocator<std::string>> &parameters,
        int returnValue);
    FunctionBucket(
        std::vector<std::string, arena::Allocator<std::string>> &parameters);
    FunctionBucket(
        std::vector<std::string, arena::Allocator<std::string>> &&parameters,
        int returnValue);

    FunctionBucket() = default;

    void setSymTab(std::string &identifier, Bucket &bucket);
    Bucket &getSymTab(std::string &identifier);

    void setStartAddr(unsigned int startAddr);
    void setEndtAddr(unsigned int endAddr);
    void setLocalNumVar(unsigned int numVar);
    unsigned int getStartAddr() const;
    unsigned int getEndAddr() const;
    unsigned int getLocalNumVar() const;
};
