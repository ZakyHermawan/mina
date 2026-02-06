#include "RegisterAllocator.hpp"

#include "MachineIR.hpp"

#include <map>
#include <set>
#include <cmath>
#include <deque>
#include <limits>
#include <vector>
#include <memory>
#include <string>
#include <utility>
#include <iostream>
#include <algorithm>
#include <functional>
#include <unordered_set>

namespace mina
{

IGNode::IGNode(std::shared_ptr<Register> inst,
               std::vector<std::shared_ptr<Register>> neighbors,
               double spill_cost, int color, bool pruned)
	: m_inst(std::move(inst)),
      m_neighbors(std::move(neighbors)),
	  m_spill_cost(spill_cost),
	  m_color(color),
	  m_pruned(pruned)
{
}

bool IGNode::isNeighborWith(const std::shared_ptr<IGNode>& other) const
{
    if (!other) return false;
    int targetID = other->getReg()->getID();

    return std::any_of(m_neighbors.begin(), m_neighbors.end(),
        [targetID](const std::shared_ptr<Register>& n) {
            return n->getID() == targetID;
        });
}

void IGNode::setSpillCost(double cost) { m_spill_cost = cost; }

double IGNode::getSpillCost() const { return m_spill_cost; }

std::shared_ptr<Register> IGNode::getReg()
{
	return m_inst;
}

std::vector<std::shared_ptr<Register>>& IGNode::getNeighbors()
{
	return m_neighbors;
}

InferenceGraph::InferenceGraph(std::vector<std::shared_ptr<IGNode>>&& nodes)
	: m_nodes(std::move(nodes))
{
}

void InferenceGraph::printAdjMatrix() const
{
    auto getSafeRegName = [&](int id) -> std::string
    {
        if (id >= 0 && id < (int)RegID::COUNT)
        {
             return mina::getReg(static_cast<mina::RegID>(id))->get64BitName();
        }

        for(auto& node : m_nodes)
        {
            if(node->getReg()->getID() == id) return node->getReg()->getString();
        }
        return "v_tmp" + std::to_string(id);
    };
    // Print Header Row
    std::cout << "\t";
    for (const auto& colNode : m_nodes)
    {
        std::cout << getSafeRegName(colNode->getReg()->getID()) << "\t";
    }
    std::cout << "\n";

    // Print Data Rows
    for (const auto& rowNode : m_nodes)
    {
        std::cout << getSafeRegName(rowNode->getReg()->getID()) << "\t";
        for (const auto& colNode : m_nodes)
        {
            if (rowNode->getReg()->getID() == colNode->getReg()->getID())
            {
                std::cout << "-\t"; 
            }
            else if (rowNode->isNeighborWith(colNode))
            {
                std::cout << "1\t";
            }
            else
            {
                std::cout << "0\t";
            }
        }
        std::cout << "\n";
    }
    std::cout << "\n";
}

void InferenceGraph::printAdjList() const
{
    auto getSafeRegName = [&](int id) -> std::string
    {
        if (id >= 0 && id < (int)RegID::COUNT)
        {
             return mina::getReg(static_cast<mina::RegID>(id))->get64BitName();
        }

        for(auto& node : m_nodes)
        {
            if(node->getReg()->getID() == id) return node->getReg()->getString();
        }
        return "v_tmp" + std::to_string(id);
    };

    for (const auto& node : m_nodes)
    {
        std::cout << getSafeRegName(node->getReg()->getID()) << ": ";

        auto& neighbors = node->getNeighbors();
        for (const auto& neighborReg : neighbors)
        {
            if (neighborReg)
            {
                std::cout << getSafeRegName(neighborReg->getID()) << " ";
            }
        }
        std::cout << "\n";
    }
    std::cout << "\n";
}

bool InferenceGraph::isNodePresent(const std::shared_ptr<IGNode>& node) const
{
	return std::any_of(m_nodes.begin(), m_nodes.end(),
		[&node](const std::shared_ptr<IGNode>& currentNode)
		{
			return currentNode->getReg()->getID() == node->getReg()->getID();
		});
}

void InferenceGraph::addNode(const std::shared_ptr<IGNode>& node)
{
	// Only add unique node
	if (!isNodePresent(node))
	{
		m_nodes.push_back(node);
	}
}

void InferenceGraph::addEdge(const std::shared_ptr<Register>& r1, const std::shared_ptr<Register>& r2)
{
    if (!r1 || !r2 || r1->getID() == r2->getID()) return;

    std::shared_ptr<IGNode> node1 = nullptr;
    std::shared_ptr<IGNode> node2 = nullptr;

    // Search existing nodes by ID
    for (auto& node : m_nodes)
    {
        if (node->getReg()->getID() == r1->getID()) node1 = node;
        if (node->getReg()->getID() == r2->getID()) node2 = node;
    }

    if (node1 && node2)
    {
        // Use ID check inside isNeighborWith to prevent duplicates
        if (!node1->isNeighborWith(node2))
        {
            // CRITICAL: Add the original register pointers stored in the nodes
            node1->getNeighbors().push_back(node2->getReg());
            node2->getNeighbors().push_back(node1->getReg());
        }
    }
}


std::vector<std::shared_ptr<IGNode>>& InferenceGraph::getNodes()
{
    return m_nodes;
}

RegisterAllocator::RegisterAllocator(
    std::vector<std::shared_ptr<BasicBlockMIR>>&& MIRBlocks)
    : m_MIRBlocks(std::move(MIRBlocks))
{
	allocateRegisters();
}

void RegisterAllocator::allocateRegisters()
{
    if (!m_MIRBlocks.empty())
    {
        calculateLoopDepths(m_MIRBlocks[0]);
    }
	auto inferenceGraph = buildGraph();
	addSpillCost(inferenceGraph);
    printSpillCosts(inferenceGraph);
	//colorGraph(inferenceGraph);
	//auto registerMap = createRegisterMap(inferenceGraph);
	//auto transformedInstructions = replaceVirtualRegisters(registerMap);
	//return transformedInstructions;
}

std::shared_ptr<InferenceGraph> RegisterAllocator::buildGraph()
{
	// Build the interference graph from m_MIRBlocks
    auto inferenceGraph = constructBaseGraph();
	addAllRegistersAsNodes(inferenceGraph);

	livenessAnalysis();
	addEdgesBasedOnLiveness(inferenceGraph);
    std::cout << "Adjacency List:\n";
	inferenceGraph->printAdjList();
    inferenceGraph->printAdjMatrix();

	return inferenceGraph;
}

void RegisterAllocator::addSpillCost(std::shared_ptr<InferenceGraph> graph)
{
    std::map<int, double> costs;

    for (const auto& block : m_MIRBlocks)
    {
        double weight = std::pow(10, block->getLoopDepth());
        for (const auto& inst : block->getInstructions())
        {
            for (const auto& op : inst->getOperands())
            {
                if (op->getMIRType() == MIRType::Reg)
                {
                    int id = std::dynamic_pointer_cast<Register>(op)->getID();
                    costs[id] += weight;
                }
            }
        }
    }

    for (auto& node : graph->getNodes())
    {
        int id = node->getReg()->getID();

        // Ensure physical registers cannot be spilled
        if (id < (int)RegID::COUNT)
        {
            node->setSpillCost(std::numeric_limits<double>::infinity());
        }
        else
        {
            node->setSpillCost(costs[id]);
        }
    }
}

void RegisterAllocator::colorGraph(std::shared_ptr<InferenceGraph> graph)
{
	// Implement graph coloring algorithm to assign colors (registers) to nodes
}

std::map<int, std::shared_ptr<Register>> RegisterAllocator::createRegisterMap(
	std::shared_ptr<InferenceGraph> graph)
{
	return {};
}

std::vector<std::shared_ptr<BasicBlockMIR>> RegisterAllocator::replaceVirtualRegisters(
	std::map<int, std::shared_ptr<Register>> registerMap)
{
    return {};
}

std::shared_ptr<InferenceGraph> RegisterAllocator::constructBaseGraph()
{
	auto& rax = getReg(RegID::RAX);
	auto& rbx = getReg(RegID::RBX);
	auto& rcx = getReg(RegID::RCX);
	auto& rdx = getReg(RegID::RDX);
	auto& rdi = getReg(RegID::RDI);
	auto& rsi = getReg(RegID::RSI);
	auto& r8 = getReg(RegID::R8);
	auto& r9 = getReg(RegID::R9);
	auto& r12 = getReg(RegID::R12);
	auto& r13 = getReg(RegID::R13);
	auto& r14 = getReg(RegID::R14);

	auto raxNode = std::make_shared<IGNode>(rax,
		std::vector<std::shared_ptr<Register>>{
		rbx, rcx, rdx, rdi, rsi, r8, r9, r12, r13, r14},
		0.0, -1, false);

	auto rbxNode = std::make_shared<IGNode>(rbx,
		std::vector<std::shared_ptr<Register>>{
		rax, rcx, rdx, rdi, rsi, r8, r9, r12, r13, r14},
		0.0, -1, false);

	auto rcxNode = std::make_shared<IGNode>(rcx,
		std::vector<std::shared_ptr<Register>>{
		rax, rbx, rdx, rdi, rsi, r8, r9, r12, r13, r14},
		0.0, -1, false);

	auto rdxNode = std::make_shared<IGNode>(rdx,
		std::vector<std::shared_ptr<Register>>{
		rax, rbx, rcx, rdi, rsi, r8, r9, r12, r13, r14},
		0.0, -1, false);

	auto rdiNode = std::make_shared<IGNode>(rdi,
		std::vector<std::shared_ptr<Register>>{
		rax, rbx, rcx, rdx, rsi, r8, r9, r12, r13, r14},
		0.0, -1, false);

	auto rsiNode = std::make_shared<IGNode>(rsi,
		std::vector<std::shared_ptr<Register>>{
		rax, rbx, rcx, rdx, rdi, r8, r9, r12, r13, r14},
		0.0, -1, false);

	auto r8Node = std::make_shared<IGNode>(r8,
		std::vector<std::shared_ptr<Register>>{
		rax, rbx, rcx, rdx, rdi, rsi, r9, r12, r13, r14},
		0.0, -1, false);

	auto r9Node = std::make_shared<IGNode>(r9,
		std::vector<std::shared_ptr<Register>>{
		rax, rbx, rcx, rdx, rdi, rsi, r8, r12, r13, r14},
		0.0, -1, false);

	auto r12Node = std::make_shared<IGNode>(r12,
		std::vector<std::shared_ptr<Register>>{
		rax, rbx, rcx, rdx, rdi, rsi, r8, r9, r13, r14},
		0.0, -1, false);

	auto r13Node = std::make_shared<IGNode>(r13,
		std::vector<std::shared_ptr<Register>>{
		rax, rbx, rcx, rdx, rdi, rsi, r8, r9, r12, r14},
		0.0, -1, false);

	auto r14Node = std::make_shared<IGNode>(r14,
		std::vector<std::shared_ptr<Register>>{
		rax, rbx, rcx, rdx, rdi, rsi, r8, r9, r12, r13},
		0.0, -1, false);

	std::vector<std::shared_ptr<IGNode>> nodes;
	nodes.push_back(raxNode);
	nodes.push_back(rbxNode);
	nodes.push_back(rcxNode);
	nodes.push_back(rdxNode);
	nodes.push_back(rdiNode);
	nodes.push_back(rsiNode);
	nodes.push_back(r8Node);
	nodes.push_back(r9Node);
	nodes.push_back(r12Node);
	nodes.push_back(r13Node);
	nodes.push_back(r14Node);
	
    std::shared_ptr<InferenceGraph> ig =
        std::make_shared<InferenceGraph>(std::move(nodes));
	return ig;
}

void RegisterAllocator::addEdgesBasedOnLiveness(std::shared_ptr<InferenceGraph> graph)
{
    for (const auto& block : m_MIRBlocks)
    {
        // Ensure liveNow starts with the actual registers live at the exit of the block
        std::set<int> liveNow = block->getLiveOut();
        auto& insts = block->getInstructions();

        for (auto it = insts.rbegin(); it != insts.rend(); ++it)
        {
            const auto& inst = *it;
            auto mirType = inst->getMIRType();
            auto& operands = inst->getOperands();

            std::set<int> instDefs;
            std::set<int> instUses;

            auto getID = [](const std::shared_ptr<MachineIR>& op)
            {
                return std::dynamic_pointer_cast<Register>(op)->getID();
            };

            // Identify DEFs and USEs based on instruction type
            switch (mirType)
            {
            case MIRType::Mov:
            case MIRType::Lea:
            case MIRType::Movzx:
            {
                if (!operands.empty() && operands[0]->getMIRType() == MIRType::Reg)
                {
                    instDefs.insert(getID(operands[0]));
                }
                if (operands.size() > 1)
                {
                    if (operands[1]->getMIRType() == MIRType::Reg)
                    {
                        instUses.insert(getID(operands[1]));
                    }
                    else if (operands[1]->getMIRType() == MIRType::Memory)
                    {
                        auto memOp = std::dynamic_pointer_cast<MemoryMIR>(operands[1]);
                        if (memOp && memOp->getBaseRegister())
                        {
                            instUses.insert(memOp->getBaseRegister()->getID());
                        }
                    }
                }
                break;
            }
            case MIRType::Add:
            case MIRType::Sub:
            {
                if (!operands.empty() && operands[0]->getMIRType() == MIRType::Reg)
                {
                    int id = getID(operands[0]);
                    instDefs.insert(id);
                    instUses.insert(id);
                }
                if (operands.size() > 1 && operands[1]->getMIRType() == MIRType::Reg)
                {
                    instUses.insert(getID(operands[1]));
                }
                break;
            }
            case MIRType::Call:
            {
                // Calls clobber RAX, RCX, RDX. These must interfere with everything currently live.
                instDefs.insert(to_int(RegID::RAX));
                instDefs.insert(to_int(RegID::RCX));
                instDefs.insert(to_int(RegID::RDX));
                
                // Arguments are used
                instUses.insert(to_int(RegID::RCX));
                instUses.insert(to_int(RegID::RDX));
                break;
            }
            case MIRType::Ret:
            {
                instUses.insert(to_int(RegID::RAX));
                break;
            }
            default:
                break;
            }

            // For each defined register, add edges to everything currently live
            for (int a : instDefs)
            {
                for (int b : liveNow)
                {
                    if (a != b)
                    {
                        // Optimization: Skip interference for move-related instructions
                        bool isMove = (mirType == MIRType::Mov || mirType == MIRType::Movzx);
                        bool isSource = instUses.count(b);

                        if (isMove && isSource)
                        {
                            continue;
                        }
                        graph->addEdge(std::make_shared<Register>(a), std::make_shared<Register>(b));
                    }
                }
            }

            // Update liveNow: In = Use U (Out - Def)
            for (int def : instDefs)
            {
                liveNow.erase(def);
            }
            for (int use : instUses)
            {
                liveNow.insert(use);
            }
        }
    }
}


void RegisterAllocator::addAllRegistersAsNodes(std::shared_ptr<InferenceGraph> graph)
{
    for (const auto& block : m_MIRBlocks)
    {
        for (const auto& inst : block->getInstructions())
        {
            for (const auto& operand : inst->getOperands())
            {
                if (operand->getMIRType() == MIRType::Reg)
                {
                    auto regOperand = std::dynamic_pointer_cast<Register>(operand);
                    auto igNode = std::make_shared<IGNode>(regOperand,
                        std::vector<std::shared_ptr<Register>>{},
                        0.0, -1, false);

                    if (!graph->isNodePresent(igNode))
                    {
                        graph->addNode(igNode);
                    }
                }
                // Catch Base Registers hidden in Memory operands
                else if (operand->getMIRType() == MIRType::Memory)
                {
                    auto memOp = std::dynamic_pointer_cast<MemoryMIR>(operand);
                    if (memOp && memOp->getBaseRegister())
                    {
                        auto& baseReg = memOp->getBaseRegister();
                        // Create a node for the base register (e.g., rbp, rip)
                        auto igNode = std::make_shared<IGNode>(baseReg,
                            std::vector<std::shared_ptr<Register>>{},
                            0.0, -1, false);

                        if (!graph->isNodePresent(igNode))
                        {
                            graph->addNode(igNode);
                        }
                    }
                }
            }
        }
    }
}

void RegisterAllocator::livenessAnalysis()
{
    // Initial Setup: Def-Use and clear existing sets
    for (auto& block : m_MIRBlocks)
    {
        block->generateDefUse();
        block->getLiveIn().clear();
        block->getLiveOut().clear();
    }

    // Initialize Worklist
    // Using deque for FIFO processing; unordered_set to track membership
    std::deque<std::shared_ptr<BasicBlockMIR>> worklist;
    std::unordered_set<std::string> inWorklist;

    // Initialize with all blocks. Iterating backwards helps liveness converge faster.
    for (auto it = m_MIRBlocks.rbegin(); it != m_MIRBlocks.rend(); ++it)
    {
        worklist.push_back(*it);
        inWorklist.insert((*it)->getName());
    }

    // Process Worklist
    while (!worklist.empty())
    {
        auto& block = worklist.front();
        worklist.pop_front();
        inWorklist.erase(block->getName());

        std::set<int> oldLiveIn = block->getLiveIn();

        // Update LiveOut: Union of successors' LiveIn
        block->getLiveOut().clear();
        for (const auto& succ : block->getSuccessors())
        {
            for (int reg : succ->getLiveIn())
            {
                block->insertLiveOut(reg);
            }
        }

        // Update LiveIn: Use union (LiveOut - Def)
        block->getLiveIn() = block->getUse();
        for (int reg : block->getLiveOut())
        {
            if (block->getDef().find(reg) == block->getDef().end())
            {
                block->insertLiveIn(reg);
            }
        }

        // If LiveIn changed, predecessors must be re-evaluated
        if (block->getLiveIn() != oldLiveIn)
        {
            for (const auto& pred : block->getPredecessors())
            {
                if (inWorklist.find(pred->getName()) == inWorklist.end())
                {
                    worklist.push_back(pred);
                    inWorklist.insert(pred->getName());
                }
            }
        }
    }

    for (auto& block : m_MIRBlocks)
    {
        block->printLivenessSets();
    }
}

// We can implement this using dominator tree for better accuracy
void RegisterAllocator::calculateLoopDepths(std::shared_ptr<BasicBlockMIR> entry) {
    std::map<std::string, bool> visited;
    std::map<std::string, bool> onStack;

    std::function<void(std::shared_ptr<BasicBlockMIR>, int)> dfs =
        [&](std::shared_ptr<BasicBlockMIR> block, int currentDepth)
    {

        visited[block->getName()] = true;
        onStack[block->getName()] = true;
        block->setLoopDepth(currentDepth);

        for (auto& succ : block->getSuccessors())
        {
            if (onStack[succ->getName()])
            {
                // We found a back-edge! This successor is a loop header.
                // In a real compiler, we'd mark the header and re-scan,
                // but for a heuristic, we increment the depth of the current path.
                block->setLoopDepth(currentDepth + 1);
            }
            else if (!visited[succ->getName()])
            {
                dfs(succ, currentDepth);
            }
        }
        onStack[block->getName()] = false;
    };

    dfs(entry, 0);
}

void RegisterAllocator::printSpillCosts(std::shared_ptr<InferenceGraph> graph)
{
    auto getSafeRegName = [&](int id) -> std::string
    {
        if (id >= 0 && id < (int)RegID::COUNT)
        {
             return mina::getReg(static_cast<mina::RegID>(id))->get64BitName();
        }

        for(auto& node : graph->getNodes())
        {
            if(node->getReg()->getID() == id) return node->getReg()->getString();
        }
        return "v_tmp" + std::to_string(id);
    };
    std::cout << "--- Register Spill Costs ---\n";
    std::cout << "Register\tCost\t\tDegree\tRatio (Cost/Deg)\n";
    std::cout << "------------------------------------------------\n";

    for (const auto& node : graph->getNodes())
    {
        double cost = node->getSpillCost();
        int degree = node->getNeighbors().size();
        std::string regName = getSafeRegName(node->getReg()->getID());

        std::cout << regName << "\t\t";

        if (cost == std::numeric_limits<double>::infinity())
        {
            std::cout << "INF";
        }
        else
        {
            std::cout << cost;
        }

        std::cout << "\t\t" << degree << "\t";

        if (cost == std::numeric_limits<double>::infinity())
        {
            std::cout << "N/A";
        }
        else if (degree > 0)
        {
            std::cout << (cost / (double)degree);
        }
        else
        {
            std::cout << "inf";
        }
        std::cout << "\n";
    }
    std::cout << "------------------------------------------------\n\n";
}

}  // namespace mina
