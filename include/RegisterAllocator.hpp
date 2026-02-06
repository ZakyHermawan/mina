#pragma once

#include "MachineIR.hpp"

#include <map>
#include <memory>
#include <vector>

namespace mina
{

// Interference Graph Node
class IGNode
{
public:
    // Each node represents a Register
    // We keep track of its neighbours, spill cost, color and whether it is pruned
    // We represent color with an integer from 0 to k-1, -1 means uncolored
    IGNode(std::shared_ptr<Register> inst,
        std::vector<std::shared_ptr<Register>> neighbors, double spill_cost,
        int color, bool pruned);
	virtual ~IGNode() = default;
    std::shared_ptr<Register> getReg();
    std::vector<std::shared_ptr<Register>>& getNeighbors();
    bool isNeighborWith(const std::shared_ptr<IGNode>& other) const;

    void setSpillCost(double cost);
    double getSpillCost() const;

private:
    std::shared_ptr<Register> m_inst;
    std::vector<std::shared_ptr<Register>> m_neighbors;
    double m_spill_cost;
    int m_color;
    bool m_pruned;
};

class InferenceGraph
{
public:
    InferenceGraph(std::vector<std::shared_ptr<IGNode>>&& nodes);
    virtual ~InferenceGraph() = default;

    InferenceGraph(const InferenceGraph&) = delete;
    InferenceGraph& operator=(const InferenceGraph&) = delete;
    InferenceGraph(InferenceGraph&&) = default;
    InferenceGraph& operator=(InferenceGraph&&) = default;

    void printAdjMatrix() const;
    void printAdjList() const;
    bool isNodePresent(const std::shared_ptr<IGNode>& node) const;
    void addNode(const std::shared_ptr<IGNode>& node);
    void addEdge(const std::shared_ptr<Register>& r1,
                 const std::shared_ptr<Register>& r2);
    std::vector<std::shared_ptr<IGNode>>& getNodes();

private:
    std::vector<std::shared_ptr<IGNode>> m_nodes;
};

class RegisterAllocator
{
public:
    RegisterAllocator(std::vector<std::shared_ptr<BasicBlockMIR>>&& MIRBlocks);
    virtual ~RegisterAllocator() = default;

    RegisterAllocator(const RegisterAllocator&) = delete;
    RegisterAllocator& operator=(const RegisterAllocator&) = delete;
    RegisterAllocator(RegisterAllocator&&) = default;
    RegisterAllocator& operator=(RegisterAllocator&&) = default;

private:
    std::vector<std::shared_ptr<BasicBlockMIR>> m_MIRBlocks;

    void allocateRegisters();
    std::shared_ptr<InferenceGraph> buildGraph();
    void addSpillCost(std::shared_ptr<InferenceGraph> graph);
    void colorGraph(std::shared_ptr<InferenceGraph> graph);
    std::map<int, std::shared_ptr<Register>> createRegisterMap(
        std::shared_ptr<InferenceGraph> graph);
    std::vector<std::shared_ptr<BasicBlockMIR>> replaceVirtualRegisters(
        std::map<int, std::shared_ptr<Register>> registerMap);

    std::shared_ptr<InferenceGraph> constructBaseGraph();
    void addEdgesBasedOnLiveness(std::shared_ptr<InferenceGraph> graph);
    void addAllRegistersAsNodes(std::shared_ptr<InferenceGraph> graph);

    /**
     * Liveness Analysis Data-Flow Equations:
     * * 1. Live-Out Equation (Union of Successors):
     * Out[B] = Union { In[S] | S is a successor of B }
     * // A register is live exiting block B if it is needed by any
     * // of the blocks that can execute immediately after B.
     * * 2. Live-In Equation (Transfer Function):
     * In[B] = Use[B] U (Out[B] - Def[B])
     * // A register is live entering block B if:
     * // a) It is used in B before being redefined (Use[B]).
     * // b) It was live exiting B and was NOT overwritten by B (Out[B] -
     * Def[B]).
     * * Note: Analysis converges when In[B] and Out[B] sets reach a fixed point
     * for all blocks in the Control Flow Graph (CFG).
     */
    void livenessAnalysis();

    void calculateLoopDepths(
        std::shared_ptr<BasicBlockMIR> entry);
    void printSpillCosts(std::shared_ptr<InferenceGraph> graph);
};

}  // namespace mina
