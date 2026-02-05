#include "RegisterAllocator.hpp"

#include "MachineIR.hpp"

#include <map>
#include <set>
#include <vector>
#include <memory>
#include <utility>
#include <string>
#include <iostream>
#include <algorithm>

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
    
    // Get the actual register we are looking for
    const auto& targetReg = other->getReg();

    // Check if that register exists in our neighbor list
    return std::any_of(m_neighbors.begin(), m_neighbors.end(),
        [&targetReg](const std::shared_ptr<Register>& neighborReg)
		{
            return neighborReg == targetReg;
        });
}

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
    std::cout << "\t";
    for (const auto& colNode : m_nodes)
	{
        std::cout << colNode->getReg()->get64BitName() << "\t";
    }
    std::cout << "\n";

    for (const auto& rowNode : m_nodes)
	{
        std::cout << rowNode->getReg()->get64BitName() << "\t";

		for (const auto& colNode : m_nodes)
		{
            if (rowNode == colNode)
			{
                std::cout << "-\t"; 
            } else if (rowNode->isNeighborWith(colNode))
			{
                std::cout << "1\t";
            } else
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
	for(const auto& node : m_nodes)
	{
		std::cout << node->getReg()->get64BitName() << ": ";
		for (const auto& neighbor : node->getNeighbors())
		{
			std::cout << neighbor->get64BitName() << " ";
		}
		std::cout << std::endl;
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

    // Find the nodes associated with these registers
    for (auto& node : m_nodes) {
        if (node->getReg()->getID() == r1->getID()) node1 = node;
        if (node->getReg()->getID() == r2->getID()) node2 = node;
    }

    // Only add if both nodes exist and aren't already neighbors
    if (node1 && node2) {
        if (!node1->isNeighborWith(node2)) {
            node1->getNeighbors().push_back(r2);
            node2->getNeighbors().push_back(r1);
        }
    }
}

RegisterAllocator::RegisterAllocator(
    std::vector<std::shared_ptr<BasicBlockMIR>>&& MIRBlocks)
    : m_MIRBlocks(std::move(MIRBlocks))
{
	allocateRegisters();
}

void RegisterAllocator::allocateRegisters()
{
	auto inferenceGraph = buildGraph();
	//addSpillCost(inferenceGraph);
	//colorGraph(inferenceGraph);
	//auto registerMap = createRegisterMap(inferenceGraph);
	//auto transformedInstructions = replaceVirtualRegisters(registerMap);
	//return transformedInstructions;
}

std::shared_ptr<InferenceGraph> RegisterAllocator::buildGraph()
{
	// Build the interference graph from m_MIRBlocks
    auto inferenceGraph = constructBaseGraph();
	inferenceGraph.printAdjList();
	inferenceGraph.printAdjMatrix();
	addAllRegistersAsNodes(inferenceGraph);

	livenessAnalysis();
	addEdgesBasedOnLiveness(inferenceGraph);

	return nullptr;
}

void RegisterAllocator::addSpillCost(std::shared_ptr<InferenceGraph> graph)
{
	// Calculate and add spill costs to each node in the graph
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

InferenceGraph RegisterAllocator::constructBaseGraph()
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
	
	InferenceGraph ig(std::move(nodes));
	return ig;
}

void RegisterAllocator::addEdgesBasedOnLiveness(InferenceGraph& graph)
{
	// Iterate through m_MIRBlocks and add edges based on liveness information
	for(const auto& block : m_MIRBlocks)
	{
		auto& defs = block->getDef();
		auto& liveOut = block->getLiveOut();
		for(int def: defs)
		{
			for (int out : liveOut)
			{
				if (out == def) continue;
				std::cout << "there is edge between  "<< def << " and " << out << std::endl;

				// Fix this later after generate code with virtual registers
				// graph.addEdge(std::make_shared<Register>(def),
				//	std::make_shared<Register>(out));
			}
		}
	}
}

void RegisterAllocator::addAllRegistersAsNodes(InferenceGraph& graph)
{
	// Iterate through m_MIRBlocks and add all virtual registers as nodes in the graph
	for(const auto& block : m_MIRBlocks)
	{
		for(const auto& inst : block->getInstructions())
		{
			auto& operands = inst->getOperands();
			for(const auto& operand : operands)
			{
				auto operandType = operand->getMIRType();
				if(operandType != MIRType::Reg)
				{
					continue; // Only interested in registers
				}

				auto regOperand = std::dynamic_pointer_cast<Register>(operand);
				std::shared_ptr<IGNode> igNode(std::make_shared<IGNode>(regOperand,
					std::vector<std::shared_ptr<Register>>{},
					0.0, -1, false));

				if(graph.isNodePresent(igNode))
				{
					//std::cout << "Skipping: " << operand->getString() << std::endl;
					continue;
				}
				graph.addNode(igNode);
				std::cout << operand->getString() << std::endl;
			}
		}
	}
}

void RegisterAllocator::livenessAnalysis()
{
	// Create def-use sets for each block
	for(auto& block: m_MIRBlocks)
	{
		block->generateDefUse();
	}

	// Create live-in and live-out sets
	for(auto& block: m_MIRBlocks)
	{
		block->getLiveIn().clear();
		block->getLiveOut().clear();
	}

	// Fixed-point iteration
	bool changed;
	do
	{
		changed = false;
		// Iterate backwards through blocks for faster convergence in forward-flow problems
		for (size_t i = m_MIRBlocks.size(); i > 0; --i)
		{
			auto& block = m_MIRBlocks[i - 1];

			std::set<int> oldLiveIn = block->getLiveIn();
			std::set<int> oldLiveOut = block->getLiveOut();

			block->getLiveOut().clear();			
			for (const auto& succ : block->getSuccessors())
			{
				for (int reg : succ->getLiveIn())
				{
					block->insertLiveOut(reg);
				}
			}

			block->getLiveIn() = block->getUse(); 
			for (const auto& reg : block->getLiveOut())
			{
				if (block->getDef().find(reg) == block->getDef().end())
				{
					block->insertLiveIn(reg);
				}
			}

			if (block->getLiveIn() != oldLiveIn || block->getLiveOut() != oldLiveOut)
			{
				changed = true;
			}
		}

	} while (changed);

	for(auto& block: m_MIRBlocks)
	{
		block->printLivenessSets();
	}
}

}  // namespace mina
