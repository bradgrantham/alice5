
#include <iomanip>

#include "basic_types.h"

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

        std::cout << "----------------------- Instruction info\n";
        std::cout << cleanName << ":\n";
        for (auto& [blockId, block] : blocks) {
            std::cout << "Block " << blockId << ":\n";

            for (auto instruction = block->instructions.head; instruction;
                    instruction = instruction->next) {
                std::ios oldState(nullptr);
                oldState.copyfmt(std::cout);

                if (instruction->blockId == NO_BLOCK_ID) {
                    std::cout << "  ---";
                } else {
                    std::cout << std::setw(5) << instruction->blockId;
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
                for (auto line : instruction->pred()) {
                    std::cout << " " << line;
                }
                std::cout << ", succ";
                for (auto line : instruction->succ()) {
                    std::cout << " " << line;
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
}

