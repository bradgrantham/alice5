
#include <iomanip>

#include "basic_types.h"
#include "risc-v.h"
#include "program.h"

// Compute successor instructions.
std::vector<Instruction *> Instruction::succ() const {
    std::vector<Instruction *> instructions;

    if (next) {
        // Not at end of block, just return next instruction.
        instructions.push_back(next.get());
    } else {
        // At end of block. Follow the block's successors.
        assert(list != nullptr);
        assert(list->block != nullptr);
        assert(list->block->function != nullptr);

        for (uint32_t blockId : list->block->succ) {
            Block *block = list->block->function->blocks.at(blockId).get();
            instructions.push_back(block->instructions.head.get());
        }
    }

    return instructions;
}

// Compute predecessor instructions.
std::vector<Instruction *> Instruction::pred() const {
    std::vector<Instruction *> instructions;

    if (prev) {
        // Not at beginning of block, just return previous instruction.
        instructions.push_back(prev.get());
    } else {
        // At beginning of block. Follow the block's predecessors.
        assert(list != nullptr);
        assert(list->block != nullptr);
        assert(list->block->function != nullptr);

        for (uint32_t blockId : list->block->pred) {
            Block *block = list->block->function->blocks.at(blockId).get();
            instructions.push_back(block->instructions.tail.get());
        }
    }

    return instructions;
}

uint32_t Instruction::blockId() const {
    return list->block->blockId;
}

std::string Instruction::pos() const {
    std::ostringstream ss;

    if (list == nullptr) {
        ss << "not in a block";
    } else {
        ss << list->block->blockId << "@";

        // Find the index in the block.
        bool found = false;
        int index;
        std::shared_ptr<Instruction> other;
        for (index = 0, other = list->head; other && !found; index++, other = other->next) {
            if (other.get() == this) {
                ss << index;
                found = true;
            }
        }
        assert(found);
    }

    return ss.str();
}

std::string Function::cleanUpName(std::string name) {
    // Replace "mainImage(vf4;vf2;" with "mainImage$v4f$vf2$"
    for (int i = 0; i < name.length(); i++) {
        if (name[i] == '(' || name[i] == ';') {
            name[i] = '$';
        }
    }

    // Strip trailing dollar sign.
    if (name.length() > 0 && name[name.length() - 1] == '$') {
        name = name.substr(0, name.length() - 1);
    }

    return name;
}

void Function::computeDomTree(bool verbose) {
    std::vector<uint32_t> worklist; // block IDs.

    // Make set of all block IDs.
    std::set<uint32_t> allBlockIds;
    for (auto &[blockId, _] : blocks) {
        allBlockIds.insert(blockId);
    }
    std::set<uint32_t> unreached = allBlockIds;

    // Prepare every block.
    for (auto& [_, block] : blocks) {
        block->dom = allBlockIds;
    }

    // Insert start block.
    worklist.push_back(startBlockId);

    while (!worklist.empty()) {
        // Take any item from worklist.
        uint32_t blockId = worklist.back();
        worklist.pop_back();

        // We can reach this from the start node.
        unreached.erase(blockId);

        Block *block = blocks.at(blockId).get();

        // Intersection of all predecessor doms.
        std::set<uint32_t> dom;
        bool first = true;
        for (auto predBlockId : block->pred) {
            Block *pred = blocks.at(predBlockId).get();

            if (first) {
                dom = pred->dom;
                first = false;
            } else {
                // I love C++.
                std::set<uint32_t> intersection;
                std::set_intersection(dom.begin(), dom.end(),
                        pred->dom.begin(), pred->dom.end(),
                        std::inserter(intersection, intersection.begin()));
                std::swap(intersection, dom);
            }
        }
        // Add ourselves.
        dom.insert(blockId);

        // If the set changed, update it and put the
        // successors in the work queue.
        if (dom != block->dom) {
            block->dom = dom;
            worklist.insert(worklist.end(), block->succ.begin(), block->succ.end());
        }
    }
    // Unreached blocks don't actually have any doms.
    for (auto id : unreached) {
        blocks[id]->dom.clear();
    }

    // Compute immediate dom for each block.
    for (auto& [blockId, block] : blocks) {
        block->idom = NO_BLOCK_ID;

        // Try each block in the block's dom.
        for (auto idomCandidate : block->dom) {
            bool valid = idomCandidate != blockId;

            // Can't be the idom if it's dominated by another dom.
            for (auto otherDom : block->dom) {
                if (otherDom != idomCandidate &&
                        otherDom != blockId &&
                        blocks[otherDom]->isDominatedBy(idomCandidate)) {

                    valid = false;
                    break;
                }
            }

            if (valid) {
                block->idom = idomCandidate;

                // Add ourselves to parent's children.
                blocks.at(idomCandidate)->idomChildren.push_back(block);
                break;
            }
        }
    }

    // Check that all blocks were assigned properly.
    for (auto& [blockId, block] : blocks) {
        // It's okay to have no idom as long as you're the first block in the function.
        if ((block->idom == NO_BLOCK_ID) != block->pred.empty()) {
            std::cout << "----\n"
                << blockId << "\n"
                << block->idom << "\n"
                << block->pred.size() << "\n";
        }
        assert((block->idom == NO_BLOCK_ID) == block->pred.empty());
    }

    if (verbose) {
        dumpBlockInfo();
        dumpGraph(unreached);
        dumpInstructions("after dom tree");
    }
}

void Function::dumpBlockInfo() const {
    std::cout << "----------------------- Block info\n";
    for (auto& [blockId, block] : blocks) {
        std::cout << "Block " << blockId << ":\n";
        std::cout << "    Pred:";
        for (auto blockId : block->pred) {
            std::cout << " " << blockId;
        }
        std::cout << "\n";
        std::cout << "    Succ:";
        for (auto blockId : block->succ) {
            std::cout << " " << blockId;
        }
        std::cout << "\n";
        std::cout << "    Dom:";
        for (auto blockId : block->dom) {
            std::cout << " " << blockId;
        }
        std::cout << "\n";
        if (block->idom != NO_BLOCK_ID) {
            std::cout << "    Immediate Dom: " << block->idom << "\n";
        }
    }
    std::cout << "-----------------------\n";
}

void Function::dumpGraph(const std::set<uint32_t> &unreached) const {
    // http://www.webgraphviz.com/
    std::cout << "digraph CFG {\n  rankdir=TB;\n";
    for (auto& [blockId, block] : blocks) {
        if (unreached.find(blockId) == unreached.end()) {
            for (auto pred : block->pred) {
                // XXX this is laid out much better if the function's start
                // block is mentioned first.
                if (unreached.find(pred) == unreached.end()) {
                    std::cout << "  \"" << pred << "\" -> \"" << blockId << "\"";
                    std::cout << ";\n";
                }
            }
            if (block->idom != NO_BLOCK_ID) {
                std::cout << "  \"" << blockId << "\" -> \"" << block->idom << "\"";
                std::cout << " [color=\"0.000, 0.999, 0.999\"]";
                std::cout << ";\n";
            }
        }
    }
    std::cout << "}\n";
    std::cout << "-----------------------\n";
}

void Function::dumpInstructions(const std::string &description) const {
    std::cout << "----------------------- Instruction info (" << description << ")\n";
    std::cout << cleanName << ":\n";
    for (auto& [blockId, block] : blocks) {
        std::cout << "Block " << blockId << ":\n";

        for (auto instruction = block->instructions.head; instruction;
                instruction = instruction->next) {
            std::ios oldState(nullptr);
            oldState.copyfmt(std::cout);

            uint32_t blockId = instruction->blockId();
            if (blockId == NO_BLOCK_ID) {
                std::cout << "  ---";
            } else {
                std::cout << std::setw(5) << blockId;
            }
            if (instruction->resIdSet.empty()) {
                std::cout << "         ";
            } else {
                for (uint32_t resId : instruction->resIdList) {
                    std::cout << std::setw(6) << resId;
                }
                std::cout << " <-";
            }

            std::cout << std::setw(0) << " " << instruction->name();

            for (auto argId : instruction->argIdList) {
                std::cout << " " << argId;
            }

            std::cout << " (pred";
            for (auto pred : instruction->pred()) {
                std::cout << " " << pred->pos();
            }
            std::cout << ", succ";
            for (auto succ : instruction->succ()) {
                std::cout << " " << succ->pos();
            }
            std::cout << ", livein";
            for (auto &[label,liveinSet] : instruction->livein) {
                std::cout << " " << label << "(";
                for (auto regId : liveinSet) {
                    std::cout << " " << regId;
                }
                std::cout << ")";
            }
            std::cout << ", liveout";
            for (auto regId : instruction->liveout) {
                std::cout << " " << regId;
            }

            std::cout << ")\n";

            std::cout.copyfmt(oldState);
        }
    }
    std::cout << "-----------------------\n";
}

// From "The Design and Implementation of a SSA-based Register Allocator" by
// Pereira (2007), section 4.2.
void Function::phiLifting() {
    // For each block that has a phi instruction, break up its parameters.
    for (auto &[_, block] : blocks) {
        Instruction *instruction = block->instructions.head.get();
        if (instruction->opcode() == RiscVOpPhi) {
            phiLiftingForBlock(block.get(), dynamic_cast<RiscVPhi *>(instruction));
        }
    }

    dumpInstructions("after phi lifting");
}

void Function::phiLiftingForBlock(Block *block, RiscVPhi *phi) {
    // For each result.
    for (int res = 0; res < phi->resultIds.size(); res++) {
        // For each label.
        for (int blockIndex = 0; blockIndex < phi->labelIds.size(); blockIndex++) {
            uint32_t operandId = phi->operandIds[res][blockIndex];
            uint32_t blockId = phi->labelIds.at(blockIndex);

            // Generate a new ID.
            uint32_t newId = program->nextReg++;
            uint32_t type = program->typeIdOf(operandId);
            program->resultTypes[newId] = type;

            // Replace the ID in the phi, in-place.
            phi->operandIds[res][blockIndex] = newId;

            // Insert a move instruction at the end of the source block, just before the
            // terminating instruction.
            Block *sourceBlock = blocks.at(blockId).get();
            LineInfo lineInfo;
            sourceBlock->instructions.insert(std::make_shared<InsnCopyObject>(
                        lineInfo, type, newId, operandId), sourceBlock->instructions.tail);
        }
    }

    // We've changed the operandIds array in-place, so recompute the args.
    phi->recomputeArgs();
}

// Swap the two instruction lists.
void swap(InstructionList &a, InstructionList &b) {
    a.swap(b);
}
