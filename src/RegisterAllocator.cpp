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
#include <stdexcept>
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
        [targetID](const std::shared_ptr<Register>& n)
        {
            return n->getID() == targetID;
        });
}

void IGNode::setSpillCost(double cost)
{
    m_spill_cost = cost;
}

double IGNode::getSpillCost() const
{
    return m_spill_cost;
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
    // Print Header Row
    std::cout << "\t";
    for (const auto& colNode : m_nodes)
    {
        std::cout << colNode->getReg()->getString() << "\t";
    }
    std::cout << "\n";

    // Print Data Rows
    for (const auto& rowNode : m_nodes)
    {
        std::cout << rowNode->getReg()->getString() << "\t";
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
    for (const auto& node : m_nodes)
    {
        std::cout << node->getReg()->getString() << ": ";

        auto& neighbors = node->getNeighbors();
        for (const auto& neighborReg : neighbors)
        {
            if (neighborReg)
            {
                std::cout << node->getReg()->getString() << " ";
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
            // Add the original register pointers stored in the nodes
            node1->getNeighbors().push_back(node2->getReg());
            node2->getNeighbors().push_back(node1->getReg());
        }
    }
}


std::vector<std::shared_ptr<IGNode>>& InferenceGraph::getNodes()
{
    return m_nodes;
}

std::string InferenceGraph::getRegName(int id) const
{
    // Try to get from physical registers first
    if (id >= 0 && id < (int)RegID::COUNT)
    {
         return mina::getReg(static_cast<mina::RegID>(id))->get64BitName();
    }

    // Try to get from virtual registers in the graph
    for(auto& node : m_nodes)
    {
        if(node->getReg()->getID() == id) return node->getReg()->getString();
    }

    throw std::runtime_error("Register ID " + std::to_string(id) +
                             " not found in InferenceGraph.");
}

RegisterAllocator::RegisterAllocator(
    std::vector<std::shared_ptr<BasicBlockMIR>>&& MIRBlocks)
    : m_MIRBlocks(std::move(MIRBlocks))
{
	allocateRegisters();
}

std::vector<std::shared_ptr<BasicBlockMIR>>&
RegisterAllocator::getMIRBlocks()
{
    return m_MIRBlocks;
}

unsigned int RegisterAllocator::getOffset() const
{
    return functionOffset;
}

std::set<int> RegisterAllocator::getUsedCalleeSavedRegs() const
{
    return m_usedCalleeSavedRegs;
}

int RegisterAllocator::getSpillAreaSize() const
{
    return m_spillAreaSize;
}

void RegisterAllocator::allocateRegisters()
{
    // Gather liveness data before running the first pass
    livenessAnalysis();

    // Do zero-initialization of uninitialized virtual registers pass
    zeroInitializeUninitializedVirtualRegisters();

    if (!m_MIRBlocks.empty())
    {
        calculateLoopDepths(m_MIRBlocks[0]);
    }

    auto inferenceGraph = buildGraph();
    //inferenceGraph->printAdjList(); // Optional debug
    //inferenceGraph->printAdjMatrix(); // Optional debug

    addSpillCost(inferenceGraph);
    //printSpillCosts(inferenceGraph); // Optional debug

    colorGraph(inferenceGraph);
    //printColoringResults(inferenceGraph); // Optional debug

    auto registerMap = createRegisterMap(inferenceGraph);
    //printRegisterMappingResults(inferenceGraph, registerMap); // Optional debug

    auto transformedInstructions = replaceVirtualRegisters(registerMap);
    //printRegisterReplacements(transformedInstructions); // Optional debug

    m_MIRBlocks = std::move(transformedInstructions);
    return;
}

void RegisterAllocator::zeroInitializeUninitializedVirtualRegisters()
{
    auto& entryBlock = m_MIRBlocks[0];
    std::vector<int> uninitRegs;

    for (int liveReg : entryBlock->getLiveIn())
    {
        // If it is a Virtual Register (ID >= 16)
        if (liveReg >= (int)RegID::COUNT)
        {
            uninitRegs.push_back(liveReg);
        }
    }

    if (!uninitRegs.empty())
    {
        // We must insert instructions at the VERY BEGINNING of the block
        // to define these variables.
        std::vector<std::shared_ptr<MachineIR>> initInstrs;

        for (int regID : uninitRegs)
        {
            // mov v_reg, 0
            auto vReg = std::make_shared<Register>(regID, "v_" + std::to_string(regID)); // Name helps debug
            auto zero = std::make_shared<ConstMIR>(0);
            auto movZero = std::make_shared<MovMIR>(std::vector<std::shared_ptr<MachineIR>>{ vReg, zero });

            initInstrs.push_back(movZero);
        }

        // Prepend initialization instructions
        auto& insts = entryBlock->getInstructions();
        insts.insert(insts.begin(), initInstrs.begin(), initInstrs.end());
    }
}

std::shared_ptr<InferenceGraph> RegisterAllocator::buildGraph()
{
	// Build the interference graph from m_MIRBlocks
    auto inferenceGraph = constructBaseGraph();
	addAllRegistersAsNodes(inferenceGraph);

	livenessAnalysis();
    //printLivenessData(inferenceGraph); // Optional debug
	addEdgesBasedOnLiveness(inferenceGraph);

	return inferenceGraph;
}

void RegisterAllocator::addSpillCost(std::shared_ptr<InferenceGraph> graph)
{
    std::map<int, double> costs;

    // Helper to identify reserved registers (RBP, RSP, RIP, R10, R11)
    auto isReserved = [](int id)
    {
        return (id >= 11 && id <= 15);
    };

    for (const auto& block : m_MIRBlocks)
    {
        double weight = std::pow(10, block->getLoopDepth());

        for (const auto& inst : block->getInstructions())
        {
            for (const auto& op : inst->getOperands())
            {
                int id = -1;

                // Check Register Operand
                if (op->getMIRType() == MIRType::Reg)
                {
                    id = std::dynamic_pointer_cast<Register>(op)->getID();
                }
                // Check Memory Operand (Base Register)
                else if (op->getMIRType() == MIRType::Memory)
                {
                    auto memOp = std::dynamic_pointer_cast<MemoryMIR>(op);
                    if (memOp && memOp->getBaseRegister())
                    {
                        id = memOp->getBaseRegister()->getID();
                    }
                }

                // Accumulate cost only if valid and NOT reserved
                if (id != -1 && !isReserved(id))
                {
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
            // If the register wasn't used (e.g. dead code), default to 0
            node->setSpillCost(costs[id]);
        }
    }
}


void RegisterAllocator::colorGraph(std::shared_ptr<InferenceGraph> graph)
{
    auto& nodes = graph->getNodes();
    std::vector<std::shared_ptr<IGNode>> pruningStack;

    // SIMPLIFY & SPILL (Pruning Virtuals Only)
    while (true)
    {
        bool all_virtual_pruned = true;
        std::shared_ptr<IGNode> chosen_node = nullptr;

        for (auto& node : nodes)
        {
            // Skip already pruned nodes OR Physical Registers (SpillCost is INF)
            if (node->isPruned() || node->getSpillCost() == std::numeric_limits<double>::infinity()) 
            {
                continue;
            }

            all_virtual_pruned = false;

            int current_degree = 0;
            for (auto& neighborReg : node->getNeighbors())
            {
                for (auto& potentialNeighbor : nodes)
                {
                    if (potentialNeighbor->getReg()->getID() == neighborReg->getID() && !potentialNeighbor->isPruned())
                    {
                        current_degree++;
                        break;
                    }
                }
            }

            if (current_degree < m_K_COLORS)
            {
                chosen_node = node;
                break;
            }
        }

        // Potential Spill Logic for Virtual Registers
        if (chosen_node == nullptr && !all_virtual_pruned)
        {
            double best_spill_metric = std::numeric_limits<double>::infinity();
            for (auto& node : nodes)
            {
                if (node->isPruned() || node->getSpillCost() == std::numeric_limits<double>::infinity())
                {
                    continue;
                }

                int degree = 0;
                for (auto& neighborReg : node->getNeighbors())
                {
                    for (auto& n : nodes)
                    {
                        if (n->getReg()->getID() == neighborReg->getID() && !n->isPruned())
                        {
                            degree++;
                            break;
                        }
                    }
                }

                double spill_metric = node->getSpillCost() / static_cast<double>(std::max(1, degree));
                if (spill_metric < best_spill_metric)
                {
                    best_spill_metric = spill_metric;
                    chosen_node = node;
                }
            }
        }

        if (all_virtual_pruned || chosen_node == nullptr)
        {
            break;
        }

        chosen_node->setPruned(true);
        pruningStack.push_back(chosen_node);
    }

    // SELECT (Coloring)
    for (auto it = pruningStack.rbegin(); it != pruningStack.rend(); ++it)
    {
        auto& node = *it;
        node->setPruned(false);

        std::set<int> available_colors;
        for (int i = 0; i < m_K_COLORS; ++i)
        {
            available_colors.insert(i);
        }

        for (auto& neighborReg : node->getNeighbors())
        {
            for (auto& potentialNeighbor : nodes)
            {
                if (potentialNeighbor->getReg()->getID() == neighborReg->getID())
                {
                    int neighbor_color = potentialNeighbor->getColor();
                    if (neighbor_color != -1)
                    {
                        available_colors.erase(neighbor_color);
                    }
                    break;
                }
            }
        }

        if (!available_colors.empty())
        {
            int regID = node->getReg()->getID();
            if (regID == to_int(RegID::R12) || regID == to_int(RegID::R13) || regID == to_int(RegID::R14))
            {
                node->setColor(*available_colors.rbegin());
            }
            else
            {
                node->setColor(*available_colors.begin());
            }
        }
        else
        {
            node->setColor(-1);
        }
    }
}

void RegisterAllocator::printColoringResults(std::shared_ptr<InferenceGraph> graph)
{
    std::cout << "--- Register Coloring Results ---\n";
    std::cout << "Virtual Reg\tColor ID\tAssigned Phys Reg\n";
    std::cout << "------------------------------------------------\n";

    auto& nodes = graph->getNodes();
    for (const auto& node : nodes)
    {
        int id = node->getReg()->getID();
        int color = node->getColor();

        std::string vRegName = node->getReg()->getString();
        std::cout << vRegName << "\t\t";

        if (color == -1)
        {
            std::cout << "SPILL\t\t[MEM SLOT REQUIRED]\n";
        }
        else
        {
            // Translate the color index (0-10) to the physical register name
            std::string physRegName = "Unknown";
            if (color >= 0 && color < (int)RegID::COUNT)
            {
                physRegName = getReg(static_cast<RegID>(color))->get64BitName();
            }

            std::cout << color << "\t\t" << physRegName << "\n";
        }
    }
    std::cout << "------------------------------------------------\n\n";
}

std::map<int, std::shared_ptr<Register>> RegisterAllocator::createRegisterMap(std::shared_ptr<InferenceGraph> graph)
{
    std::map<int, std::shared_ptr<Register>> registerMap;

    // Reset the tracker for this allocation pass
    m_usedCalleeSavedRegs.clear();

    // Define Windows x64 Callee-Saved Register IDs
    // RBX(1), RDI(4), RSI(5), R12(8), R13(9), R14(10)
    std::set<int> calleeSavedIDs = {to_int(RegID::RBX), to_int(RegID::RDI),
                                    to_int(RegID::RSI), to_int(RegID::R12),
                                    to_int(RegID::R13), to_int(RegID::R14)};

    auto& nodes = graph->getNodes();

    for (const auto& node : nodes)
    {
        int color = node->getColor();

        // If node.color is null (spilled), continue
        if (color == -1)
        {
            continue;
        }

        auto regNode = node->getReg();
        int regID = regNode->getID();

        // If the assigned physical register (color) is Callee-Saved (Non-Volatile),
        // we must record it in 'm_usedCalleeSavedRegs'.
        // The CodeGen will use this set to emit specific 'push' and 'pop'
        // instructions in the function prologue/epilogue to preserve the caller's state.
        if (calleeSavedIDs.count(color))
        {
            m_usedCalleeSavedRegs.insert(color);
        }

        // Map the Virtual Register ID to the actual Physical Register object.
        registerMap[regID] = getReg(static_cast<RegID>(color));
    }

    return registerMap;
}

void RegisterAllocator::printRegisterMappingResults(
    std::shared_ptr<InferenceGraph> graph,
    const std::map<int, std::shared_ptr<Register>>& registerMap)
{
    std::cout << "\n--- Final Register Mapping (Virtual -> Physical) ---\n";
    if (registerMap.empty())
    {
        std::cout << "  (Empty: No virtual registers mapped)\n";
    }
    else
    {
        auto& nodes = graph->getNodes();

        // Iterate over the map of assignments
        for (const auto& pair : registerMap)
        {
            int vRegID = pair.first;
            const auto& physReg = pair.second;
            std::string vRegName = "v" + std::to_string(vRegID); // Fallback

            // Find the original name in the graph
            for (const auto& node : nodes)
            {
                if (node->getReg()->getID() == vRegID)
                {
                    vRegName = node->getReg()->getString();
                    break;
                }
            }

            std::cout << "  " << vRegName << " -> " << physReg->get64BitName() << "\n";
        }
    }

    std::cout << "\n--- Callee-Saved Registers To Preserve ---\n";
    if (m_usedCalleeSavedRegs.empty())
    {
        std::cout << "  (None)\n";
    }
    else
    {
        std::cout << "  [ ";
        for (int regID : m_usedCalleeSavedRegs)
        {
            std::cout << getReg(static_cast<RegID>(regID))->get64BitName() << " ";
        }
        std::cout << "]\n";
    }
    std::cout << "------------------------------------------------\n\n";
}

std::vector<std::shared_ptr<BasicBlockMIR>> RegisterAllocator::replaceVirtualRegisters(
    const std::map<int, std::shared_ptr<Register>>& registerMap)
{
    std::vector<std::shared_ptr<BasicBlockMIR>> newBlocks;

    // Map<VirtualRegisterID, Offset>
    std::map<int, int> spillOffsets;
    int currentSpillBytes = 0;

    std::function<std::shared_ptr<MachineIR>(std::shared_ptr<MachineIR>)> resolveOperand;
    resolveOperand = [&](std::shared_ptr<MachineIR> op) -> std::shared_ptr<MachineIR>
    {
        if (op->getMIRType() == MIRType::Reg)
        {
            auto regObj = std::dynamic_pointer_cast<Register>(op);
            int regID = regObj->getID();

            if (regID < (int)RegID::COUNT) return op; // Physical

            if (registerMap.count(regID))
            {
                return registerMap.at(regID); // Mapped
            }

            // Spill
            if (spillOffsets.find(regID) == spillOffsets.end())
            {
                currentSpillBytes += 8;
                spillOffsets[regID] = currentSpillBytes;
            }

            int finalOffset = 64 + spillOffsets[regID];

            // return [rbp - offset]
            return std::make_shared<MemoryMIR>(
                getReg(RegID::RBP),
                std::make_shared<ConstMIR>(-finalOffset)
            );
        }
        else if (op->getMIRType() == MIRType::Memory)
        {
            auto memOp = std::dynamic_pointer_cast<MemoryMIR>(op);
            if (memOp && memOp->getBaseRegister())
            {
                auto newBase = resolveOperand(memOp->getBaseRegister());

                // If base is a Register, reconstruct MemoryMIR
                if (newBase->getMIRType() == MIRType::Reg)
                {
                    auto newBaseReg = std::dynamic_pointer_cast<Register>(newBase);
                    auto disp = memOp->getOffsetOrLiteral();

                    // EXPLICIT CASTING for constructors
                    if (auto constDisp = std::dynamic_pointer_cast<ConstMIR>(disp))
                    {
                        return std::make_shared<MemoryMIR>(newBaseReg, constDisp);
                    }
                    else if (auto litDisp = std::dynamic_pointer_cast<LiteralMIR>(disp))
                    {
                        return std::make_shared<MemoryMIR>(newBaseReg, litDisp);
                    }
                    else
                    {
                        return std::make_shared<MemoryMIR>(newBaseReg, 0);
                    }
                }
            }
            return op;
        }

        return op; // Immediate/Label
    };

    const auto& r10 = getReg(RegID::R10);
    const auto& r11 = getReg(RegID::R11);

    for (const auto& oldBlock : m_MIRBlocks)
    {
        auto newBlock = std::make_shared<BasicBlockMIR>(oldBlock->getName());

        for (const auto& inst : oldBlock->getInstructions())
        {
            auto mirType = inst->getMIRType();
            const auto& oldOps = inst->getOperands();

            // Resolve Operands
            std::vector<std::shared_ptr<MachineIR>> newOps;
            for (auto& op : oldOps)
            {
                newOps.push_back(resolveOperand(op));
            }

            // Legalize Mem-Mem
            bool op0Mem = (newOps.size() > 0 && newOps[0]->getMIRType() == MIRType::Memory);
            bool op1Mem = (newOps.size() > 1 && newOps[1]->getMIRType() == MIRType::Memory);

            if (op0Mem && op1Mem && mirType != MIRType::Mov)
            {
                // mov r10, src -> op dest, r10
                newBlock->addInstruction(std::make_shared<MovMIR>(
                    std::vector<std::shared_ptr<MachineIR>>{ r10, newOps[1] }
                ));
                newOps[1] = r10;
            }

            // Emit Instructions
            switch (mirType)
            {
            // Binary Ops (Constructors take vector<MachineIR>)
            case MIRType::Mov:   newBlock->addInstruction(std::make_shared<MovMIR>(newOps)); break;
            case MIRType::Add:   newBlock->addInstruction(std::make_shared<AddMIR>(newOps)); break;
            case MIRType::Sub:   newBlock->addInstruction(std::make_shared<SubMIR>(newOps)); break;
            case MIRType::Mul:   newBlock->addInstruction(std::make_shared<MulMIR>(newOps)); break;
            case MIRType::And:   newBlock->addInstruction(std::make_shared<AndMIR>(newOps)); break;
            case MIRType::Or:    newBlock->addInstruction(std::make_shared<OrMIR>(newOps)); break;
            case MIRType::Cmp:   newBlock->addInstruction(std::make_shared<CmpMIR>(newOps)); break;
            case MIRType::Lea:   newBlock->addInstruction(std::make_shared<LeaMIR>(newOps)); break;

            // Unary Ops (Need Casting)
            case MIRType::Not:
            {
                auto regOp = std::dynamic_pointer_cast<Register>(newOps[0]);
                if (regOp)
                {
                    newBlock->addInstruction(std::make_shared<NotMIR>(regOp));
                }
                else
                {
                    // Fixup: NOT [mem] -> MOV R11, [mem]; NOT R11; MOV [mem], R11
                    newBlock->addInstruction(std::make_shared<MovMIR>(std::vector<std::shared_ptr<MachineIR>>{ r11, newOps[0] }));
                    newBlock->addInstruction(std::make_shared<NotMIR>(r11));
                    newBlock->addInstruction(std::make_shared<MovMIR>(std::vector<std::shared_ptr<MachineIR>>{ newOps[0], r11 }));
                }
                break;
            }

            case MIRType::Div:
            {
                if (auto mem = std::dynamic_pointer_cast<MemoryMIR>(newOps[0]))
                {
                    newBlock->addInstruction(std::make_shared<DivMIR>(mem));
                }
                else
                {
                    throw std::runtime_error("DivMIR constructor must accept MemoryMIR.");
                }
                break;
            }

            // SetCC Instructions (Require Register)
            // Macro to handle cast + fixup
            #define HANDLE_SETCC(CLASS_NAME) \
                { \
                    auto regOp = std::dynamic_pointer_cast<Register>(newOps[0]); \
                    if (regOp) { \
                        newBlock->addInstruction(std::make_shared<CLASS_NAME>(regOp)); \
                    } else { \
                        /* Fixup: SETcc r11b; MOV [mem], r11 */ \
                        newBlock->addInstruction(std::make_shared<CLASS_NAME>(r11)); \
                        newBlock->addInstruction(std::make_shared<MovMIR>(std::vector<std::shared_ptr<MachineIR>>{ newOps[0], r11 })); \
                    } \
                }

            case MIRType::Sete:  HANDLE_SETCC(SeteMIR); break;
            case MIRType::Setne: HANDLE_SETCC(SetneMIR); break;
            case MIRType::Setl:  HANDLE_SETCC(SetlMIR); break;
            case MIRType::Setle: HANDLE_SETCC(SetleMIR); break;
            case MIRType::Setg:  HANDLE_SETCC(SetgMIR); break;
            case MIRType::Setge: HANDLE_SETCC(SetgeMIR); break;

            // Movzx (Requires Register Destination)
            case MIRType::Movzx:
            {
                auto oldMovzx = std::dynamic_pointer_cast<MovzxMIR>(inst);
                auto& op = newOps[0];
                auto regOp = std::dynamic_pointer_cast<Register>(op);

                if (regOp)
                {
                    newBlock->addInstruction(std::make_shared<MovzxMIR>(
                        regOp,
                        oldMovzx->getToRegSize(),
                        oldMovzx->getFromRegSize(),
                        oldMovzx->isFromRegLow()
                    ));
                }
                else
                {
                    // Spill Fixup: MOVZX r11, r11 -> MOV [mem], r11
                    // Load Spill -> R11
                    newBlock->addInstruction(std::make_shared<MovMIR>(
                        std::vector<std::shared_ptr<MachineIR>>{ r11, op }
                    ));
                    // Ext R11 -> R11
                    newBlock->addInstruction(std::make_shared<MovzxMIR>(
                        r11,
                        oldMovzx->getToRegSize(),
                        oldMovzx->getFromRegSize(),
                        oldMovzx->isFromRegLow()
                    ));
                    // Store R11 -> Spill
                    newBlock->addInstruction(std::make_shared<MovMIR>(
                        std::vector<std::shared_ptr<MachineIR>>{ op, r11 }
                    ));
                }
                break;
            }

            // Test (Requires Registers)
            case MIRType::Test:
            {
                auto r1 = std::dynamic_pointer_cast<Register>(newOps[0]);
                auto r2 = std::dynamic_pointer_cast<Register>(newOps[1]);

                // Load spilled operands into scratch regs if necessary
                if (!r1)
                {
                    newBlock->addInstruction(std::make_shared<MovMIR>(std::vector<std::shared_ptr<MachineIR>>{ r10, newOps[0] }));
                    r1 = r10;
                }
                if (!r2)
                {
                    newBlock->addInstruction(std::make_shared<MovMIR>(std::vector<std::shared_ptr<MachineIR>>{ r11, newOps[1] }));
                    r2 = r11;
                }

                newBlock->addInstruction(std::make_shared<TestMIR>(r1, r2));
                break;
            }

            // Control Flow
            case MIRType::Call:
            case MIRType::Ret:
            case MIRType::Jmp:
            case MIRType::Jz:
            case MIRType::Jnz:
            case MIRType::Cqo:
                newBlock->addInstruction(inst);
                break;

            default:
                newBlock->addInstruction(inst);
                break;
            }
        }
        newBlocks.push_back(newBlock);
    }

    m_spillAreaSize = currentSpillBytes;
    return newBlocks;
}

void RegisterAllocator::printRegisterReplacements(
    const std::vector<std::shared_ptr<BasicBlockMIR>>& finalBlocks) const
{
    std::cout << "\n=== REGISTER REPLACEMENT DIAGNOSTICS ===\n";

    int totalInsts = 0;
    int memoryOps = 0;
    int illegalOps = 0;
    int remainingVirtuals = 0;

    for (const auto& block : finalBlocks)
    {
        for (const auto& inst : block->getInstructions())
        {
            totalInsts++;
            const auto& ops = inst->getOperands();

            // Check 1: Are there any Virtual Registers left?
            for (const auto& op : ops)
            {
                if (op->getMIRType() == MIRType::Reg)
                {
                    auto reg = std::dynamic_pointer_cast<Register>(op);
                    if (reg->getID() >= (int)RegID::COUNT)
                    {
                        std::cout << "[ERROR] Found remaining Virtual Register v"
                                  << reg->getID() << " in block " << block->getName() << "\n";
                        remainingVirtuals++;
                    }
                }
                else if (op->getMIRType() == MIRType::Memory)
                {
                    memoryOps++;
                }
            }

            // Check 2: Illegal Memory-to-Memory Operations
            // (Exclude Mov because we fixed it, but check binary ops like Add, Sub, Cmp)
            if (ops.size() >= 2 && 
                ops[0]->getMIRType() == MIRType::Memory &&
                ops[1]->getMIRType() == MIRType::Memory)
            {
                // Move instructions can technically handle mem-mem via scratch, 
                // but if we see it in the final output as a single instruction, it's illegal.
                if (inst->getMIRType() != MIRType::Mov)
                {
                     std::cout << "[ERROR] Illegal Mov Mem-Mem operation detected in block "
                               << block->getName() << "\n";
                     illegalOps++;
                }
            }
        }
    }

    std::cout << "Total Instructions: " << totalInsts << "\n";
    std::cout << "Memory Operands (Spills/Locals): " << memoryOps << "\n";
    std::cout << "Spill Area Size: " << m_spillAreaSize << " bytes\n";

    if (remainingVirtuals == 0 && illegalOps == 0)
    {
        std::cout << "[SUCCESS] All virtual registers replaced and instructions legalized.\n";
    }
    else
    {
        std::cout << "[FAILURE] Issues detected in code generation.\n";
    }
    std::cout << "========================================\n\n";
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

    // List of registers to build the initial clique
    std::vector<std::shared_ptr<Register>> physRegs =
    {
        rax, rbx, rcx, rdx, rdi, rsi, r8, r9, r12, r13, r14
    };

    // We assign fixed color indices 0-10 to the physical registers.
    // This prevents the allocator from thinking they are uncolored (-1).
    auto raxNode = std::make_shared<IGNode>(rax, std::vector<std::shared_ptr<Register>>{}, 0.0, 0, false);
    auto rbxNode = std::make_shared<IGNode>(rbx, std::vector<std::shared_ptr<Register>>{}, 0.0, 1, false);
    auto rcxNode = std::make_shared<IGNode>(rcx, std::vector<std::shared_ptr<Register>>{}, 0.0, 2, false);
    auto rdxNode = std::make_shared<IGNode>(rdx, std::vector<std::shared_ptr<Register>>{}, 0.0, 3, false);
    auto rdiNode = std::make_shared<IGNode>(rdi, std::vector<std::shared_ptr<Register>>{}, 0.0, 4, false);
    auto rsiNode = std::make_shared<IGNode>(rsi, std::vector<std::shared_ptr<Register>>{}, 0.0, 5, false);
    auto r8Node  = std::make_shared<IGNode>(r8,  std::vector<std::shared_ptr<Register>>{}, 0.0, 6, false);
    auto r9Node  = std::make_shared<IGNode>(r9,  std::vector<std::shared_ptr<Register>>{}, 0.0, 7, false);
    auto r12Node = std::make_shared<IGNode>(r12, std::vector<std::shared_ptr<Register>>{}, 0.0, 8, false);
    auto r13Node = std::make_shared<IGNode>(r13, std::vector<std::shared_ptr<Register>>{}, 0.0, 9, false);
    auto r14Node = std::make_shared<IGNode>(r14, std::vector<std::shared_ptr<Register>>{}, 0.0, 10, false);

    std::vector<std::shared_ptr<IGNode>> nodes =
    {
        raxNode, rbxNode, rcxNode, rdxNode, rdiNode, rsiNode,
        r8Node, r9Node, r12Node, r13Node, r14Node
    };

    std::shared_ptr<InferenceGraph> ig =
        std::make_shared<InferenceGraph>(std::move(nodes));

    // Make physical registers interfere with each other so they don't share colors.
    // This creates a "Clique" where every physical register has a different color.
    for (size_t i = 0; i < physRegs.size(); ++i)
    {
        for (size_t j = i + 1; j < physRegs.size(); ++j)
        {
            ig->addEdge(physRegs[i], physRegs[j]);
        }
    }

    return ig;
}

void RegisterAllocator::addEdgesBasedOnLiveness(std::shared_ptr<InferenceGraph> graph)
{
    for (const auto& block : m_MIRBlocks)
    {
        // Start with registers live at the end of the block
        // Note: Should copy, because we will mutate it
        // And we did not want to change the block's actual liveOut set
        std::set<int> liveNow = block->getLiveOut();
        auto& insts = block->getInstructions();

        // Iterate backward
        for (auto it = insts.rbegin(); it != insts.rend(); ++it)
        {
            const auto& inst = *it;
            auto mirType = inst->getMIRType();
            auto& operands = inst->getOperands();

            std::set<int> instDefs;
            std::set<int> instUses;

            // Helper to get ID safely
            auto getID = [](const std::shared_ptr<MachineIR>& op)
            {
                return std::dynamic_pointer_cast<Register>(op)->getID();
            };

            // Identify DEFs and USEs
            switch (mirType)
            {
                case MIRType::Mov:
                case MIRType::Lea:
                case MIRType::Movzx:
                {
                    // Destination (Index 0)
                    if (!operands.empty())
                    {
                        if (operands[0]->getMIRType() == MIRType::Reg)
                        {
                            // Writing to register -> DEF
                            instDefs.insert(getID(operands[0]));
                        }
                        else if (operands[0]->getMIRType() == MIRType::Memory)
                        {
                            // Writing to memory [reg] -> USE of base register
                            auto memOp = std::dynamic_pointer_cast<MemoryMIR>(operands[0]);
                            if (memOp && memOp->getBaseRegister())
                            {
                                instUses.insert(memOp->getBaseRegister()->getID());
                            }
                        }
                    }

                    // Source (Index 1)
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
                case MIRType::And:
                case MIRType::Or:
                case MIRType::Not:
                {
                    if (!operands.empty() && operands[0]->getMIRType() == MIRType::Reg)
                    {
                        int id = getID(operands[0]);
                        instDefs.insert(id);
                        instUses.insert(id); // Read-Modify-Write
                    }
                    if (operands.size() > 1 && operands[1]->getMIRType() == MIRType::Reg)
                    {
                        instUses.insert(getID(operands[1]));
                    }
                    break;
                }

                case MIRType::Mul:
                {
                    // Implicit RDX:RAX = RAX * op
                    instUses.insert(to_int(RegID::RAX));
                    instDefs.insert(to_int(RegID::RAX));
                    instDefs.insert(to_int(RegID::RDX));
                    if (!operands.empty() && operands[0]->getMIRType() == MIRType::Reg)
                    {
                        instUses.insert(getID(operands[0]));
                    }
                    break;
                }

                case MIRType::Div:
                {
                    // Implicit RDX:RAX / op
                    instUses.insert(to_int(RegID::RAX));
                    instUses.insert(to_int(RegID::RDX));
                    instDefs.insert(to_int(RegID::RAX));
                    instDefs.insert(to_int(RegID::RDX));
                    if (!operands.empty() && operands[0]->getMIRType() == MIRType::Reg)
                    {
                        instUses.insert(getID(operands[0]));
                    }
                    break;
                }

                case MIRType::Call:
                {
                    // Defs: Caller-saved registers (Clobbers)
                    instDefs.insert(to_int(RegID::RAX));
                    instDefs.insert(to_int(RegID::RCX));
                    instDefs.insert(to_int(RegID::RDX));
                    instDefs.insert(to_int(RegID::R8));
                    instDefs.insert(to_int(RegID::R9));

                    // Uses: Arguments
                    const auto callInst =
                        std::dynamic_pointer_cast<CallMIR>(inst);
                    unsigned int numArgs = callInst->getNumArgs();

                    if (numArgs >= 1) instUses.insert(to_int(RegID::RCX));
                    if (numArgs >= 2) instUses.insert(to_int(RegID::RDX));
                    if (numArgs >= 3) instUses.insert(to_int(RegID::R8));
                    if (numArgs >= 4) instUses.insert(to_int(RegID::R9));
                    break;
                }

                case MIRType::Ret:
                {
                    instUses.insert(to_int(RegID::RAX));
                    break;
                }

                default:
                {
                    break;
                }
            }

            // Add Interference Edges
            for (int def : instDefs)
            {
                for (int live : liveNow)
                {
                    if (def == live)
                    {
                        continue;
                    }

                    // Move Optimization: Don't add edge if live reg is the source of the move
                    bool isMove = (mirType == MIRType::Mov || mirType == MIRType::Movzx);
                    bool isSource = false;

                    if (isMove && operands.size() > 1 && operands[1]->getMIRType() == MIRType::Reg)
                    {
                        auto srcReg = std::dynamic_pointer_cast<Register>(operands[1]);
                        if (srcReg && srcReg->getID() == live)
                        {
                            isSource = true;
                        }
                    }

                    if (isMove && isSource)
                    {
                        continue;
                    }

                    graph->addEdge(std::make_shared<Register>(def), std::make_shared<Register>(live));
                }
            }

            // Update Liveness (LiveNow = (LiveNow - Defs) + Uses)
            for (int def : instDefs)
            {
                liveNow.erase(def);
            }
            for (int use : instUses)
            {
                // Filter reserved regs from being tracked in liveness (RBP, RSP, RIP, R10, R11)
                if (!(use >= 11 && use <= 15))
                {
                    liveNow.insert(use);
                }
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
                    auto id = regOperand->getID();
                    if (id >= m_RESERVED_START && id <= m_RESERVED_END) continue;

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
                        int id = baseReg->getID();
                        if (id >= m_RESERVED_START && id <= m_RESERVED_END) continue;

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
}

void RegisterAllocator::printLivenessData(
    std::shared_ptr<InferenceGraph> graph) const
{
    for (const auto& block : m_MIRBlocks)
    {
        std::cout << "Liveness data for block: " << block->getName() << "\n";

        auto printSet = [&](const std::string& label, const std::set<int>& regSet)
        {
            std::cout << "  " << label << ": ";
            if (regSet.empty())
            {
                std::cout << "(empty)";
            }
            else
            {
                for (int id : regSet)
                {
                    std::cout << graph->getRegName(id) << " ";
                }
            }
            std::cout << "\n";
        };

        printSet("Defs    ", block->getDef());
        printSet("Uses    ", block->getUse());
        printSet("Live-In ", block->getLiveIn());
        printSet("Live-Out", block->getLiveOut());
        std::cout << "--------------------------------------\n";
    }
}

// We can implement this using dominator tree for better accuracy
void RegisterAllocator::calculateLoopDepths(std::shared_ptr<BasicBlockMIR> entry)
{
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
    std::cout << "--- Register Spill Costs ---\n";
    std::cout << "Register\tCost\t\tDegree\tRatio (Cost/Deg)\n";
    std::cout << "------------------------------------------------\n";

    for (const auto& node : graph->getNodes())
    {
        double cost = node->getSpillCost();
        int degree = node->getNeighbors().size();
        std::string regName = node->getReg()->getString();

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
