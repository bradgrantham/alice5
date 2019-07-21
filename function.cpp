
#include <iomanip>

#include "function.h"
#include "program.h"

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

    /// dumpInstructions("after phi lifting");
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

void Function::spill() {
    Timer timer;
    std::set<Instruction *> inst_worklist; // Instructions left to work on.

    // Insert all instructions of all blocks.
    for (auto &[_, block] : blocks) {
        for (auto inst = block->instructions.head; inst; inst = inst->next) {
            inst_worklist.insert(inst.get());
        }
    }

    int maxIntLiveness = 0;
    int maxFloatLiveness = 0;
    while (!inst_worklist.empty()) {
        // Pick any PC to start with.
        Instruction *instruction = *inst_worklist.begin();
        inst_worklist.erase(instruction);

        // Keep track of old livein/liveout for comparison.
        std::map<uint32_t,std::set<uint32_t>> oldLivein = instruction->livein;
        std::set<uint32_t> oldLiveout = instruction->liveout;

        // Compute liveout as union of next instructions' liveins.
        instruction->liveout.clear();
        for (Instruction *succInstruction : instruction->succ()) {
            // Add livein from any branch (0).
            instruction->liveout.insert(
                    succInstruction->livein[0].begin(),
                    succInstruction->livein[0].end());

            // Add livein specifically from this branch (if phi).
            auto itr = succInstruction->livein.find(instruction->blockId());
            if (itr != succInstruction->livein.end()) {
                instruction->liveout.insert(itr->second.begin(), itr->second.end());
            }
        }

        // Livein is liveout minus what we generate ...
        instruction->livein[0] = instruction->liveout;
        for (uint32_t resId : instruction->resIdSet) {
            instruction->livein[0].erase(resId);
        }
        // ... plus what we need.
        assert(instruction->opcode() != SpvOpPhi); // Should have been replaced.
        if (instruction->opcode() == RiscVOpPhi) {
            // Special handling of Phi instruction, because we must specify
            // which branch our livein is from. Otherwise all branches will
            // think they need all inputs, which is incorrect.
            RiscVPhi *phi = dynamic_cast<RiscVPhi *>(instruction);
            assert(phi->operandIds.size() == phi->resultIds.size());
            for (int i = 0; i < phi->operandIds.size(); i++) {
                // Operands for a specific result.
                const std::vector<uint32_t> &operandIds = phi->operandIds[i];
                assert(phi->labelIds.size() == operandIds.size());

                for (int j = 0; j < operandIds.size(); j++) {
                    instruction->livein[phi->labelIds[j]].insert(operandIds[j]);
                }
            }
        } else {
            // Non-phi instruction, all parameters are livein.
            for (uint32_t argId : instruction->argIdSet) {
                // Don't consider /*constants or*/ variables, they're never in registers.
                if (/*constants.find(argId) == constants.end() &&*/
                        program->variables.find(argId) == program->variables.end()) {

                    instruction->livein[0].insert(argId);
                }
            }
        }

        // If we changed, then our predecessors need to update since they
        // depend on us.
        if (oldLivein != instruction->livein || oldLiveout != instruction->liveout) {
            // Our predecessors depend on us.
            for (Instruction *predInstruction : instruction->pred()) {
                inst_worklist.insert(predInstruction);
            }

            int intLiveness = 0;
            int floatLiveness = 0;
            for (uint32_t reg : instruction->livein.at(0)) {
                uint32_t typeId = program->typeIdOf(reg);
                if (typeId == 0) {
                    std::cerr << "Error: Can't find type ID of virtual register " << reg << ".\n";
                    exit(1);
                }
                uint32_t typeOp = program->getTypeOp(typeId);
                switch (typeOp) {
                    case SpvOpTypeInt:
                    case SpvOpTypePointer:
                    case SpvOpTypeBool:
                        intLiveness += 1;
                        break;

                    case SpvOpTypeFloat:
                        floatLiveness += 1;
                        break;

                    default:
                        std::cerr << "Error: Can't have type " << typeId
                            << " of type op " << typeOp << " when computing liveness.\n";
                        exit(1);
                }
            }
            if (intLiveness > maxIntLiveness) {
                maxIntLiveness = intLiveness;
            }
            if (floatLiveness > maxFloatLiveness) {
                maxFloatLiveness = floatLiveness;
            }
        }
    }
    if (PRINT_TIMER_RESULTS) {
        std::cerr << "Livein and liveout took " << timer.elapsed() << " seconds.\n";
    }
    std::cerr << "Max int liveness is " << maxIntLiveness << "\n";
    std::cerr << "Max float liveness is " << maxFloatLiveness << "\n";
}
