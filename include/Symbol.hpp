#pragma once

#include "Types.hpp"

#include <string>
#include "arena_alloc.hpp"
#include <vector>
#include <unordered_map>
#include <stdexcept>

using arenaVectorInt = std::vector<int, arena::Allocator<int>>;

class Bucket
{
private:
    int m_intVal;
    int m_stackAddr;  // relative to the stack frame on current lexical level
    arenaVectorInt m_arr;
    Type m_type;

    void validateIdx(int idx);

public:
    Bucket();
    ~Bucket() = default;
    
    Bucket(int val, int stackAddr, Type type);
    Bucket(arenaVectorInt &arr, int stackAddr, Type type);
    Bucket(arenaVectorInt &&arr, int stackAddr, Type type);
    
    void setArrSize(unsigned int size);
    size_t getArrSize() const;
    
    void setIntVal(int val);
    void setArr(arenaVectorInt &arr);
    void setArrAtIdx(unsigned int idx, int val);
    int getArrAtIdx(unsigned int idx);
    int getStackAddr() const;

    Type getType() const;   
    int getVal() const;
};

class FunctionBucket
{
private:
    std::vector<std::string, arena::Allocator<std::string>> m_parameters;
    int m_returnValue;
    FType m_ftype;
    std::unordered_map<std::string, Bucket> m_symTab;
    
    unsigned int m_start_addr = 0;
    unsigned int m_end_addr = 0;
    unsigned int m_localNumVar = 0;

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
