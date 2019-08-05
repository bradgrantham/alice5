
#include <iomanip>

#include "function.h"
#include "program.h"

std::string Function::cleanUpName(std::string name) {
    // Replace "mainImage(vf4;vf2;" with "mainImage$v4f$vf2$"
    for (size_t i = 0; i < name.length(); i++) {
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

            instruction->dump(std::cout);
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
    for (size_t res = 0; res < phi->resultIds.size(); res++) {
        // For each label.
        for (size_t blockIndex = 0; blockIndex < phi->labelIds.size(); blockIndex++) {
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

void Function::ensureMaxRegisters() {
    // Set of registers that have already been spilled, so we don't spill them
    // again.
    std::set<uint32_t> alreadySpilled;
    uint32_t spilledRegId;

    do {
        computeLiveness();
        dumpInstructions("After liveness");
        spilledRegId = spillIfNecessary(alreadySpilled);
        if (spilledRegId != NO_REGISTER) {
            alreadySpilled.insert(spilledRegId);
        }
        // Keep going as long as we're spilling.
    } while (spilledRegId != NO_REGISTER);
}

int ___count = 0; // XXX

void Function::computeLiveness() {
    Timer timer;
    std::set<Instruction *> inst_worklist; // Instructions left to work on.

    // Insert all instructions of all blocks.
    for (auto &[_, block] : blocks) {
        for (auto inst = block->instructions.head; inst; inst = inst->next) {
            inst_worklist.insert(inst.get());
            inst->livein.clear();
            inst->liveout.clear();
        }
    }

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
            for (size_t i = 0; i < phi->operandIds.size(); i++) {
                // Operands for a specific result.
                const std::vector<uint32_t> &operandIds = phi->operandIds[i];
                assert(phi->labelIds.size() == operandIds.size());

                for (size_t j = 0; j < operandIds.size(); j++) {
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
        }
    }
    if (PRINT_TIMER_RESULTS || true) {
        std::cout << "Livein and liveout took " << timer.elapsed() << " seconds.\n";
    }
}

uint32_t Function::spillIfNecessary(const std::set<uint32_t> &alreadySpilled) {
    Timer timer;
    int maxFloatLiveness = 0;
    Instruction *heaviestInstruction = nullptr;

    for (auto &[_, block] : blocks) {
        for (auto inst = block->instructions.head; inst; inst = inst->next) {
            std::set<uint32_t> liveInts;
            std::set<uint32_t> liveFloats;
            computeLiveSets(inst.get(), liveInts, liveFloats);
            int floatLiveness = liveFloats.size();
            if (floatLiveness > maxFloatLiveness) {
                maxFloatLiveness = floatLiveness;
                heaviestInstruction = inst.get();
            }
            // We don't currently have a problem with too many ints, so ignore
            // them for now.
        }
    }

    std::cout << "Max float liveness is " << maxFloatLiveness << "\n";

    uint32_t spilledRegId = NO_REGISTER;
    if (heaviestInstruction != nullptr) {
        // Recompute these.
        std::set<uint32_t> liveInts;
        std::set<uint32_t> liveFloats;
        computeLiveSets(heaviestInstruction, liveInts, liveFloats);
        ___count++; // XXX remove.
        // XXX this is 31 because my swap routine assumes f31 is free.
        if (liveFloats.size() > 31 && ___count < 200) {
            spilledRegId = spillVariable(heaviestInstruction, liveFloats, alreadySpilled);
        }
    }

    if (PRINT_TIMER_RESULTS) {
        std::cerr << "Spilling took " << timer.elapsed() << " seconds.\n";
    }

    return spilledRegId;
}

void Function::computeLiveSets(Instruction *instruction,
        std::set<uint32_t> &liveInts,
        std::set<uint32_t> &liveFloats) {

    for (uint32_t regId : instruction->livein.at(0)) {
        uint32_t typeId = program->typeIdOf(regId);
        if (typeId == 0) {
            std::cerr << "Error: Can't find type ID of virtual register " << regId << ".\n";
            exit(1);
        }

        uint32_t typeOp = program->getTypeOp(typeId);
        switch (typeOp) {
            case SpvOpTypeInt:
            case SpvOpTypePointer:
            case SpvOpTypeBool:
                liveInts.insert(regId);
                break;

            case SpvOpTypeFloat:
                liveFloats.insert(regId);
                break;

            default:
                std::cerr << "Error: Can't have type " << typeId
                    << " of type op " << typeOp << " when computing liveness.\n";
                exit(1);
        }
    }
}

uint32_t Function::spillVariable(Instruction *instruction, const std::set<uint32_t> &liveFloats,
        const std::set<uint32_t> &alreadySpilled) {

    // Pick one randomly. Can later use ComputeWeight or similar to pick a good one.
    uint32_t regId = NO_REGISTER;
    for (uint32_t liveRegId : liveFloats) {
        if (alreadySpilled.find(liveRegId) == alreadySpilled.end()
                && (regId == NO_REGISTER || liveRegId < regId)) {

            regId = liveRegId;
        }
    }
    if (regId == NO_REGISTER) {
        std::cerr << "Have already spilled everything.\n";
        std::cerr << "Live registers:";
        for (uint32_t regId : liveFloats) {
            std::cerr << " " << regId;
        }
        std::cerr << "\n";
        std::cerr << "Already spilled registers:";
        for (uint32_t regId : alreadySpilled) {
            std::cerr << " " << regId;
        }
        std::cerr << "\n";
        instruction->dump(std::cerr);
    }
    assert(regId != NO_REGISTER);
    uint32_t typeId = program->typeIdOf(regId);
    bool isConstant = program->isConstant(regId);
    std::cout << "Spilling " << (isConstant ? "constant" : "variable")
        << " " << regId << " of type " << typeId << "\n";

    // We may not have a type for the "pointer to variable" that we need, so create one.
    // It's probably okay if one already exists, I don't think we ever compare types
    // by ID.
    uint32_t pointerTypeId = program->nextReg++;
    program->types[pointerTypeId] = std::make_shared<TypePointer>(program->types[typeId],
            typeId, SpvStorageClassFunction);
    program->typeSizes[pointerTypeId] = sizeof(uint32_t);

    // Create a new local (function) variable.
    uint32_t varId = program->nextReg++;
    program->variables[varId] = {pointerTypeId, SpvStorageClassFunction,
        NO_INITIALIZER, 0xFFFFFFFF};

    // Find every use.
    bool found = false;
    for (auto &[_, block] : blocks) {
        for (auto inst = block->instructions.head; inst; inst = inst->next) {
            if (inst->usesRegister(regId)) {
                found = true;

                // Find a new register name for the use.
                uint32_t newRegId = program->nextReg++;
                program->resultTypes[newRegId] = typeId;

                // Add a load instruction before the use.
                LineInfo lineInfo;
                std::shared_ptr<Instruction> loadInstruction = std::make_shared<RiscVLoad>(
                        lineInfo, typeId, newRegId, varId, NO_MEMORY_ACCESS_SEMANTIC, 0);

                // Insert before this instruction.
                block->instructions.insert(loadInstruction, inst);

                // Rename the use.
                inst->changeArg(regId, newRegId);
                // XXX do we need to call recomputeArgs() if this is a RiscVPhi?
            }
        }
    }
    assert(found);

    // Create the store instruction.
    LineInfo lineInfo;
    std::shared_ptr<Instruction> saveInstruction = std::make_shared<RiscVStore>(
            lineInfo, varId, regId, NO_MEMORY_ACCESS_SEMANTIC, 0);

    if (isConstant) {
        // Insert store at the very beginning of the function.
        // XXX this is wasteful. We could just load from the constant directly
        // each time, but we don't have an effective way to represent that load
        // above.
        Block *block = blocks.at(startBlockId).get();
        block->instructions.insert(saveInstruction, block->instructions.head);
    } else {
        // Find the definition.
        found = false;
        for (auto &[_, block] : blocks) {
            for (auto inst = block->instructions.head; inst; inst = inst->next) {
                if (inst->affectsRegister(regId)) {
                    // There can only be one definition.
                    assert(!found);
                    found = true;

                    // Insert before next instruction. This won't be the last instruction,
                    // because the last instruction of a block is always a terminating
                    // instructions, and they don't affect variables.
                    assert(inst->next);
                    block->instructions.insert(saveInstruction, inst->next);
                }
            }
        }
        assert(found);
    }

    return regId;
}
