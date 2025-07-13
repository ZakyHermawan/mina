#pragma once

#include "Visitors.hpp"
#include "BasicBlock.hpp"
#include "Types.hpp"
#include "InstIR.hpp"

#include <stack>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include <asmjit/asmjit.h>


void printInt();
void printBool();
void printChar();
void printWithParam(int param);

class IRVisitor : public Visitor
{
private:
    int m_tempCounter;
    int m_labelCounter;
    int m_currBBCtr;
    std::vector<std::shared_ptr<Inst>> m_arguments;
    std::vector<std::string> m_argNames;
    std::string m_currBBNameWithoutCtr;
    std::stack<std::string> m_temp;
    std::stack<std::shared_ptr<Inst>> m_instStack;
    std::stack<std::string> m_labels;
    
    std::shared_ptr<BasicBlock> m_cfg, m_lowerCFG;
    std::shared_ptr<BasicBlock> m_currentBB; // current basic block

    std::unordered_map<std::string, int> m_nameCtr;
    std::unordered_map<std::string, std::shared_ptr<BasicBlock>> m_funcBB;

    std::unordered_set<std::shared_ptr<BasicBlock>> m_sealedBlocks;
    
    using subMap = std::unordered_map<std::string, std::shared_ptr<Inst>>;
    using subPhi = std::unordered_map<std::string, std::shared_ptr<PhiInst>>;

    std::unordered_map<std::shared_ptr<BasicBlock>, subMap> m_currDef;
    std::unordered_map<std::shared_ptr<BasicBlock>, subPhi> m_incompletePhis;

    asmjit::JitRuntime m_jitRuntime;
    asmjit::CodeHolder m_codeHolder;
    asmjit::x86::Assembler m_assembler;
    asmjit::FileLogger m_logger;  // Logger should always survive CodeHolder.

    asmjit::x86::Gp m_tmp;

public:
    IRVisitor();
    void visit(StatementsAST& v) override;
    void visit(NumberAST& v) override;
    void visit(BoolAST& v) override;
    void visit(StringAST& v) override;
    void visit(VariableAST& v) override;
    void visit(ProgramAST& v) override;
    void visit(ScopeAST& v) override;
    void visit(ScopedExprAST& v) override;
    void visit(AssignmentAST& v) override;
    void visit(OutputAST& v) override;
    void visit(OutputsAST& v) override;
    void visit(InputAST& v) override;
    void visit(InputsAST& v) override;
    void visit(IfAST& v) override;
    void visit(RepeatUntilAST& v) override;
    void visit(LoopAST& v) override;
    void visit(ExitAST& v) override;
    void visit(ReturnAST& v) override;
    void visit(ParameterAST& v) override;
    void visit(ParametersAST& v) override;
    void visit(ArrAccessAST& v) override;
    void visit(ArgumentsAST& v) override;
    void visit(CallAST& v) override;
    void visit(FactorAST& v) override;
    void visit(FactorsAST& v) override;
    void visit(TermAST& v) override;
    void visit(SimpleExprAST& v) override;
    void visit(TermsAST& v) override;
    void visit(OptRelationAST& v) override;
    void visit(ExpressionAST& v) override;
    void visit(VarDeclAST& v) override;
    void visit(ArrDeclAST& v) override;
    void visit(DeclarationsAST& v) override;
    void visit(ProcDeclAST& v) override;
    void visit(FuncDeclAST& v) override;

    std::string getCurrentTemp();
    void generateCurrentTemp();

    void pushCurrentTemp();
    std::string popTemp();
    std::string getLastTemp();

    std::shared_ptr<Inst> popInst();

    std::string baseNameToSSA(const std::string& name);
    std::string getCurrentSSAName(const std::string& name);
    void printCFG();
    std::string getBaseName(std::string name);

    void writeVariable(std::string varName, std::shared_ptr<BasicBlock> block,
                       std::shared_ptr<Inst> value);
    std::shared_ptr<Inst> readVariable(std::string varName,
                                       std::shared_ptr<BasicBlock> block);
    std::shared_ptr<Inst> readVariableRecursive(std::string varName,
                                       std::shared_ptr<BasicBlock> block);
    std::shared_ptr<Inst> addPhiOperands(std::string varName,
                                         std::shared_ptr<PhiInst> phi);
    std::shared_ptr<Inst> tryRemoveTrivialPhi(std::shared_ptr<PhiInst> phi);
    void sealBlock(std::shared_ptr<BasicBlock> block);
    void generateX86();
};

class DisjointSetUnion {
private:
    std::unordered_map<std::string, std::string> parent;

public:
    // Adds a new variable to the DSU, initially in its own set.
    void make_set(const std::string& v) {
        if (parent.find(v) == parent.end()) {
            parent[v] = v;
        }
    }

    // Finds the representative (root) of the set containing variable v.
    std::string find(const std::string& v) {
        // If v is not in the map, add it.
        make_set(v);
        // Find the root with path compression for efficiency.
        if (parent[v] == v) {
            return v;
        }
        return parent[v] = find(parent[v]);
    }

    // Merges the sets containing variables u and v.
    void unite(const std::string& u, const std::string& v) {
        std::string root_u = find(u);
        std::string root_v = find(v);
        if (root_u != root_v) {
            // Point the root of v's set to the root of u's set.
            parent[root_v] = root_u;
        }
    }
};

