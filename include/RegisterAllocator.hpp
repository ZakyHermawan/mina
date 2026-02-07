#pragma once

#include "MachineIR.hpp"

#include <map>
#include <memory>
#include <set>
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

    void setColor(int color) { m_color = color; }
    int getColor() const { return m_color; }
    void setPruned(bool pruned) { m_pruned = pruned; }
    bool isPruned() const { return m_pruned; }

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

    std::vector<std::shared_ptr<BasicBlockMIR>>& getMIRBlocks();
    unsigned int getOffset() const;

    std::set<int> getUsedCalleeSavedRegs() const;

private:
    std::vector<std::shared_ptr<BasicBlockMIR>> m_MIRBlocks;
    unsigned int functionOffset = 0;

    std::vector<std::shared_ptr<IGNode>> m_pruningStack;
    const int m_K_COLORS = 11; // RAX, RBX, RCX, RDX, RDI, RSI, R8, R9, R12, R13, R14

    const int m_RESERVED_START = 11;
    const int m_RESERVED_END = 15;

    std::set<int> m_usedCalleeSavedRegs;

    void allocateRegisters();
    std::shared_ptr<InferenceGraph> buildGraph();
    void addSpillCost(std::shared_ptr<InferenceGraph> graph);

    /**
     * @brief Performs Chaitin-Briggs Graph Coloring for Register Allocation.
     *
     * This function assigns physical registers (colors) to virtual registers
     * based on the interference graph. It operates in two distinct phases to
     * ensure that variables with overlapping live ranges are assigned different
     * physical registers.
     *
     * Constraints:
     * - K = 11: The number of available physical registers (RAX, RBX, RCX, RDX,
     * RDI, RSI, R8-R9, R12-R14).
     * - Physical Registers: Treated as pre-colored nodes with Infinite Spill
     * Cost. They are never pruned and act as permanent boundary conditions in
     * the graph.
     *
     * Phase 1: SIMPLIFY & SPILL (Pruning)
     * Iteratively removes (prunes) virtual registers from the graph and pushes
     * them onto a stack.
     * 1. Simplify: Finds a node with degree < K. This node is guaranteed to be
     * colorable because it has fewer neighbors than available colors.
     * 2. Spill Heuristic: If no node < K exists, selects a candidate for
     * potential spilling. The metric used is (SpillCost / Degree), prioritizing
     * nodes that are expensive to spill or interfere with many neighbors.
     *
     * Phase 2: SELECT (Coloring)
     * Rebuilds the graph by popping nodes from the stack (Last-In-First-Out).
     * 1. For each node, identifies colors used by its currently active
     * (unpruned) neighbors.
     * 2. Assigns the first available color from the pool [0..10].
     * 3. Optimization: Biases assignment towards Callee-Saved registers
     * (R12-R14) for specific nodes to optimize prologue/epilogue overhead.
     * 4. Actual Spill: If no color is available, the node is marked with color
     * -1, indicating it must be stored in memory (stack).
     *
     */
    void colorGraph(std::shared_ptr<InferenceGraph> graph);
    void printColoringResults(std::shared_ptr<InferenceGraph> graph);

    std::map<int, std::shared_ptr<Register>> createRegisterMap(
        std::shared_ptr<InferenceGraph> graph);
    void printRegisterMappingResults(
        std::shared_ptr<InferenceGraph> graph,
        const std::map<int, std::shared_ptr<Register>>& registerMap);

    std::vector<std::shared_ptr<BasicBlockMIR>> replaceVirtualRegisters(
        std::map<int, std::shared_ptr<Register>> registerMap);

    // Create all physical registers as nodes in the graph
    // and return the base graph with physical registers interconnected
    // We need to interfere all physical registers to form a clique
    // so they never get the same color
    std::shared_ptr<InferenceGraph> constructBaseGraph();

    /**
     * @brief Builds the Interference Graph edges by analyzing instruction-level
     * liveness.
     * * This function performs a "Backward Data-Flow Analysis" on every basic
     * block. It determines exactly which variables are live at every single
     * instruction to draw interference edges between variables that overlap.
     * * Logic:
     * 1. Start at the bottom of the block with the variables known to be live
     * at the exit (LiveOut).
     * 2. Walk backward instruction by instruction.
     * 3. At each instruction:
     * a. A variable defined (written) here interferes with everything currently
     * live. b. Update liveness:
     * - Remove defined variables (they are dead above this point).
     * - Add used variables (they must be alive above this point).
     */
    void addEdgesBasedOnLiveness(std::shared_ptr<InferenceGraph> graph);
    void addAllRegistersAsNodes(std::shared_ptr<InferenceGraph> graph);

    /**
     * @brief Get Live-Out, Live-In sets for each BasicBlock using
     * def/use sets.
     *
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
    void livenessAnalysis(std::shared_ptr<InferenceGraph> graph);
    void printLivenessData(std::shared_ptr<InferenceGraph> graph) const;

    void calculateLoopDepths(
        std::shared_ptr<BasicBlockMIR> entry);
    void printSpillCosts(std::shared_ptr<InferenceGraph> graph);
};

}  // namespace mina
