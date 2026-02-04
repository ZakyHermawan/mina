#pragma once

#include "MachineIR.hpp"

#include <map>
#include <memory>
#include <vector>

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
    std::shared_ptr<Register> getInst();
    std::vector<std::shared_ptr<Register>>& getNeighbors();
    bool isNeighborWith(const std::shared_ptr<IGNode>& other) const;

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

    void constructBaseGraph();
};
