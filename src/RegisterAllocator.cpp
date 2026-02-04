#include "RegisterAllocator.hpp"

#include "MachineIR.hpp"

#include <map>
#include <utility>
#include <vector>
#include <memory>
#include <iostream>
#include <algorithm>

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
        [&targetReg](const std::shared_ptr<Register>& neighborReg) {
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
    for (const auto& colNode : m_nodes) {
        std::cout << colNode->getReg()->get64BitName() << "\t";
    }
    std::cout << "\n";

    for (const auto& rowNode : m_nodes) {
        std::cout << rowNode->getReg()->get64BitName() << "\t";

		for (const auto& colNode : m_nodes) {
            if (rowNode == colNode) {
                std::cout << "-\t"; 
            } else if (rowNode->isNeighborWith(colNode)) {
                std::cout << "1\t";
            } else {
                std::cout << "0\t";
            }
        }
        std::cout << "\n";
    }
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
}

bool InferenceGraph::isNodePresent(const std::shared_ptr<IGNode>& node) const
{
	return std::any_of(m_nodes.begin(), m_nodes.end(),
		[&node](const std::shared_ptr<IGNode>& currentNode)
		{
			return currentNode->getReg()->getID() == node->getReg()->getID();
		});
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
	// Construct the base interference graph without spill costs or coloring	
	auto rax = std::make_shared<Register>(0, "rax", "eax", "ax", "ah", "al");
	auto rbx = std::make_shared<Register>(1, "rbx", "ebx", "bx", "bh", "bl");
	auto rcx = std::make_shared<Register>(2, "rcx", "ecx", "cx", "ch", "cl");
	auto rdx = std::make_shared<Register>(3, "rdx", "edx", "dx", "dh", "dl");
	auto rdi = std::make_shared<Register>(4, "rdi", "edi", "di", "dil", "");
	auto rsi = std::make_shared<Register>(5, "rsi", "esi", "si", "sil", "");
	auto r8  = std::make_shared<Register>(6, "r8",  "r8d",  "r8w",  "r8b",  "");
	auto r9  = std::make_shared<Register>(7, "r9",  "r9d",  "r9w",  "r9b",  "");
	auto r12 = std::make_shared<Register>(8, "r12", "r12d", "r12w", "r12b", "");
	auto r13 = std::make_shared<Register>(9, "r13", "r13d", "r13w", "r13b", "");
	
	auto raxNode = std::make_shared<IGNode>(rax,
		std::vector<std::shared_ptr<Register>>{
		rbx, rcx, rdx, rdi, rsi, r8, r9, r12, r13},
		0.0, -1, false);
	
	auto rbxNode = std::make_shared<IGNode>(rbx,
		std::vector<std::shared_ptr<Register>>{
		rax, rcx, rdx, rdi, rsi, r8, r9, r12, r13},
		0.0, -1, false);

	auto rcxNode = std::make_shared<IGNode>(rcx,
		std::vector<std::shared_ptr<Register>>{
		rax, rbx, rdx, rdi, rsi, r8, r9, r12, r13},
		0.0, -1, false);

	auto rdxNode = std::make_shared<IGNode>(rdx,
		std::vector<std::shared_ptr<Register>>{
		rax, rbx, rcx, rdi, rsi, r8, r9, r12, r13},
		0.0, -1, false);

	auto rdiNode = std::make_shared<IGNode>(rdi,
		std::vector<std::shared_ptr<Register>>{
		rax, rbx, rcx, rdx, rsi, r8, r9, r12, r13},
		0.0, -1, false);

	auto rsiNode = std::make_shared<IGNode>(rsi,
		std::vector<std::shared_ptr<Register>>{
		rax, rbx, rcx, rdx, rdi, r8, r9, r12, r13},
		0.0, -1, false);

	auto r8Node = std::make_shared<IGNode>(r8,
		std::vector<std::shared_ptr<Register>>{
		rax, rbx, rcx, rdx, rdi, rsi, r9, r12, r13},
		0.0, -1, false);

	auto r9Node = std::make_shared<IGNode>(r9,
		std::vector<std::shared_ptr<Register>>{
		rax, rbx, rcx, rdx, rdi, rsi, r8, r12, r13},
		0.0, -1, false);

	auto r12Node = std::make_shared<IGNode>(r12,
		std::vector<std::shared_ptr<Register>>{
		rax, rbx, rcx, rdx, rdi, rsi, r8, r9, r13},
		0.0, -1, false);

	auto r13Node = std::make_shared<IGNode>(r13,
		std::vector<std::shared_ptr<Register>>{
		rax, rbx, rcx, rdx, rdi, rsi, r8, r9, r12},
		0.0, -1, false);
	
	std::vector<std::shared_ptr<IGNode>> nodes;
	nodes.reserve(10);

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
	
	InferenceGraph ig(std::move(nodes));
	return ig;
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
					std::cout << "Skipping: " << operand->getString() << std::endl;
					continue;
				}
				std::cout << operand->getString() << std::endl;
			}
		}
	}
}
