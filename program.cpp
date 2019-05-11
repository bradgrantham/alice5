#include <iomanip>

#include "program.h"
#include "risc-v.h"


std::map<uint32_t, std::string> OpcodeToString = {
#include "opcode_to_string.h"
};

Program::Program(bool throwOnUnimplemented_, bool verbose_) :
    throwOnUnimplemented(throwOnUnimplemented_),
    hasUnimplemented(false),
    verbose(verbose_),
    currentFunction(nullptr)
{
    memorySize = 0;
    auto anotherRegion = [this](size_t size){MemoryRegion r(memorySize, size); memorySize += size; return r;};
    memoryRegions[SpvStorageClassUniformConstant] = anotherRegion(1024);
    memoryRegions[SpvStorageClassInput] = anotherRegion(1024);
    memoryRegions[SpvStorageClassUniform] = anotherRegion(4096);
    memoryRegions[SpvStorageClassOutput] = anotherRegion(1024);
    memoryRegions[SpvStorageClassWorkgroup] = anotherRegion(0);
    memoryRegions[SpvStorageClassCrossWorkgroup] = anotherRegion(0);
    memoryRegions[SpvStorageClassPrivate] = anotherRegion(16384); // XXX still needed?
    memoryRegions[SpvStorageClassFunction] = anotherRegion(16384);
    memoryRegions[SpvStorageClassGeneric] = anotherRegion(0);
    memoryRegions[SpvStorageClassPushConstant] = anotherRegion(0);
    memoryRegions[SpvStorageClassAtomicCounter] = anotherRegion(0);
    memoryRegions[SpvStorageClassImage] = anotherRegion(0);
    memoryRegions[SpvStorageClassStorageBuffer] = anotherRegion(0);
}

spv_result_t Program::handleHeader(void* user_data, spv_endianness_t endian,
                               uint32_t /* magic */, uint32_t version,
                               uint32_t generator, uint32_t id_bound,
                               uint32_t schema)
{
    // auto pgm = static_cast<Program*>(user_data);
    return SPV_SUCCESS;
}

// Return the scalar for the vector register's index.
uint32_t Program::scalarize(uint32_t vreg, int i, uint32_t subtype, uint32_t scalarReg) {
    auto regIndex = RegIndex{vreg, i};
    auto itr = vec2scalar.find(regIndex);
    if (itr == vec2scalar.end()) {
        bool scalarRegWasSpecified = scalarReg > 0;
        if (!scalarRegWasSpecified) {
            scalarReg = nextReg++;
        }
        vec2scalar[regIndex] = scalarReg;

        // See if this was a constant vector.
        auto itr = constants.find(vreg);
        if (itr != constants.end()) {
            // Extract new constant for this component.
            Register &r = allocConstantObject(scalarReg, subtype);
            auto [subtype2, offset] = getConstituentInfo(itr->second.type, i);
            uint32_t size = typeSizes[subtype];
            assert(subtype == subtype2);
            std::copy(itr->second.data + offset, itr->second.data + offset + size, r.data);
        } else {
            // Not a constant, must be a variable.
            if (!scalarRegWasSpecified) {
                assert(this->resultTypes.find(scalarReg) == this->resultTypes.end());
                this->resultTypes[scalarReg] = subtype;
            }
        }
    } else {
        if (scalarReg != 0) {
            assert(scalarReg == itr->second);
        } else {
            scalarReg = itr->second;
        }
    }

    return scalarReg;
}

// Post-parsing work.
void Program::postParse(bool scalarize) {
    if (scalarize) {
        // Convert vector instructions to scalar instructions.
        expandVectors();
    }

    // Find the main function.
    mainFunction = nullptr;
    for(auto& e: entryPoints) {
        if(e.second.name == "main") {
            mainFunction = &functions[e.first];
        }
    }

    // Allocated variables 
    for(auto& [id, var]: variables) {
        if(var.storageClass == SpvStorageClassUniformConstant) {
            uint32_t binding = decorations.at(id).at(SpvDecorationBinding)[0];
            var.address = memoryRegions[SpvStorageClassUniformConstant].base + binding * 16; // XXX magic number
            storeNamedVariableInfo(names[id], var.type, var.address);

        } else if(var.storageClass == SpvStorageClassUniform) {
            uint32_t binding = decorations.at(id).at(SpvDecorationBinding)[0];
            var.address = memoryRegions[SpvStorageClassUniform].base + binding * 256; // XXX magic number
            storeNamedVariableInfo(names[id], var.type, var.address);
        } else if(var.storageClass == SpvStorageClassOutput) {
            uint32_t location;
            if(decorations.at(id).find(SpvDecorationLocation) == decorations.at(id).end()) {
                throw std::runtime_error("no Location decoration available for output " + std::to_string(id));
            }

            location = decorations.at(id).at(SpvDecorationLocation)[0];
            var.address = memoryRegions[SpvStorageClassOutput].base + location * 256; // XXX magic number
            storeNamedVariableInfo(names[id], var.type, var.address);
        } else if(var.storageClass == SpvStorageClassInput) {
            uint32_t location;
            if(names[id] == "gl_FragCoord") {
                location = 0;
            } else if(names[id].find("gl_") == 0) {
                throw std::runtime_error("builtin input variable " + names[id] + " specified with no location available");
            } else {
                if(decorations.find(id) == decorations.end()) {
                    throw std::runtime_error("no decorations available for input " + std::to_string(id) + ", name \"" + names[id] + "\"");
                }
                if(decorations.at(id).find(SpvDecorationLocation) == decorations.at(id).end()) {
                    throw std::runtime_error("no Location decoration available for input " + std::to_string(id));
                }

                location = decorations.at(id).at(SpvDecorationLocation)[0];
            }
            var.address = memoryRegions[SpvStorageClassInput].base + location * 256; // XXX magic number
            storeNamedVariableInfo(names[id], var.type, var.address);
        } else {
            var.address = allocate(var.storageClass, var.type);
        }
    }
    if(verbose) {
        std::cout << "----------------------- Variable binding and location info\n";
        for(auto& [name, info]: namedVariables) {
            std::cout << "variable " << name << " is at " << info.address << '\n';
        }
    }

    // Figure out our basic blocks. These start on an OpLabel and end on
    // a terminating instruction.
    for (auto [labelId, codeIndex] : labels) {
        bool found = false;
        for (int i = codeIndex; i < instructions.size(); i++) {
            if (instructions[i]->isTermination()) {
                blocks[labelId] = std::make_unique<Block>(labelId, codeIndex, uint32_t(i + 1));
                found = true;
                break;
            }
        }
        if (!found) {
            std::cout << "Error: Terminating instruction for label "
                << labelId << " not found.\n";
            exit(EXIT_FAILURE);
        }
    }

    // Compute successor and predecessor blocks.
    for (auto& [labelId, block] : blocks) {
        Instruction *instruction = instructions[block->end - 1].get();
        assert(instruction->isTermination());
        block->succ = instruction->targetLabelIds;
        for (uint32_t labelId : block->succ) {
            blocks[labelId]->pred.insert(block->labelId);
        }
    }

    // Record the block ID for each instruction. Note a problem
    // here is that the OpFunctionParameter instruction gets put into the
    // block at the end of the previous function. I don't think this
    // matters in practice because there's never a Phi at the top of a
    // function.
    for (auto& [labelId, block] : blocks) {
        for (int pc = block->begin; pc < block->end; pc++) {
            instructions[pc]->blockId = block->labelId;
        }
    }

    // Compute predecessor and successor instructions for each instruction.
    // For most instructions this is just the previous or next line, except
    // for the ones at the beginning or end of each block.
    for (auto& [labelId, block] : blocks) {
        for (auto predBlockId : block->pred) {
            instructions[block->begin]->pred.insert(blocks[predBlockId]->end - 1);
        }
        if (block->begin != block->end - 1) {
            instructions[block->begin]->succ.insert(block->begin + 1);
        }
        for (int pc = block->begin + 1; pc < block->end - 1; pc++) {
            instructions[pc]->pred.insert(pc - 1);
            instructions[pc]->succ.insert(pc + 1);
        }
        if (block->begin != block->end - 1) {
            instructions[block->end - 1]->pred.insert(block->end - 2);
        }
        for (auto succBlockId : block->succ) {
            instructions[block->end - 1]->succ.insert(blocks[succBlockId]->begin);
        }
    }

    // Compute livein and liveout registers for each line.
    Timer timer;
    std::set<uint32_t> pc_worklist; // PCs left to work on.
    for (int pc = 0; pc < instructions.size(); pc++) {
        pc_worklist.insert(pc);
    }
    while (!pc_worklist.empty()) {
        // Pick any PC to start with. Starting at the end is a bit more efficient
        // since our predecessor depends on us.
        auto itr = pc_worklist.rbegin();
        uint32_t pc = *itr;
        pc_worklist.erase(*itr);

        // Keep track of old livein/liveout for comparison.
        Instruction *instruction = instructions[pc].get();
        std::map<uint32_t,std::set<uint32_t>> oldLivein = instruction->livein;
        std::set<uint32_t> oldLiveout = instruction->liveout;

        // Compute liveout as union of next instructions' liveins.
        instruction->liveout.clear();
        for (uint32_t succPc : instruction->succ) {
            Instruction *succInstruction = instructions[succPc].get();

            // Add livein from any branch (0).
            instruction->liveout.insert(
                    succInstruction->livein[0].begin(),
                    succInstruction->livein[0].end());

            // Add livein specifically from this branch.
            auto itr = succInstruction->livein.find(instruction->blockId);
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
        if (instruction->opcode() == SpvOpPhi) {
            // Special handling of Phi instruction, because we must specify
            // which branch our livein is from. Otherwise all branches will
            // think they need all inputs, which is incorrect.
            InsnPhi *phi = dynamic_cast<InsnPhi *>(instruction);
            for (int i = 0; i < phi->operandId.size(); i += 2) {
                uint32_t data = phi->operandId[i];
                uint32_t label = phi->operandId[i + 1];
                instruction->livein[label].insert(data);
            }
        } else {
            // Non-phi instruction, all parameters are livein.
            for (uint32_t argId : instruction->argIdSet) {
                // Don't consider /*constants or*/ variables, they're never in registers.
                if (/*constants.find(argId) == constants.end() &&*/
                        variables.find(argId) == variables.end()) {

                    instruction->livein[0].insert(argId);
                }
            }
        }

        // If we changed, then our predecessors need to update since they
        // depend on us.
        if (oldLivein != instruction->livein || oldLiveout != instruction->liveout) {
            // Our predecessors depend on us.
            for (uint32_t predPc : instruction->pred) {
                pc_worklist.insert(predPc);
            }

            // Compute union of livein.
            instruction->liveinAll.clear();
            for (auto &[label,liveinSet] : instruction->livein) {
                instruction->liveinAll.insert(liveinSet.begin(), liveinSet.end());
            }
        }
    }
    std::cerr << "Livein and liveout took " << timer.elapsed() << " seconds.\n";

    // Compute the dominance tree for blocks. Use a worklist. Do all blocks
    // simultaneously (across functions).
    timer.reset();
    std::vector<uint32_t> worklist; // block IDs.
    // Make set of all label IDs.
    std::set<uint32_t> allLabelIds;
    for (auto& [labelId, block] : blocks) {
        allLabelIds.insert(labelId);
    }
    std::set<uint32_t> unreached = allLabelIds;
    // Prepare every block.
    for (auto& [labelId, block] : blocks) {
        block->dom = allLabelIds;
    }
    // Insert every function's start block.
    for (auto& [_, function] : functions) {
        assert(function.labelId != NO_BLOCK_ID);
        worklist.push_back(function.labelId);
    }
    while (!worklist.empty()) {
        // Take any item from worklist.
        uint32_t labelId = worklist.back();
        worklist.pop_back();

        // We can reach this from the start node.
        unreached.erase(labelId);

        Block *block = blocks.at(labelId).get();

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
        dom.insert(labelId);

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
    std::cerr << "Dominance tree took " << timer.elapsed() << " seconds.\n";

    // Compute immediate dom for each block.
    for (auto& [labelId, block] : blocks) {
        block->idom = NO_BLOCK_ID;

        // Try each block in the block's dom.
        for (auto idomCandidate : block->dom) {
            bool valid = idomCandidate != labelId;

            // Can't be the idom if it's dominated by another dom.
            for (auto otherDom : block->dom) {
                if (otherDom != idomCandidate &&
                        otherDom != labelId &&
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

    // Dump block info.
    if (verbose) {
        std::cout << "----------------------- Block info\n";
        for (auto& [labelId, block] : blocks) {
            std::cout << "Block " << labelId << ", instructions "
                << block->begin << "-" << block->end << ":\n";
            std::cout << "    Pred:";
            for (auto labelId : block->pred) {
                std::cout << " " << labelId;
            }
            std::cout << "\n";
            std::cout << "    Succ:";
            for (auto labelId : block->succ) {
                std::cout << " " << labelId;
            }
            std::cout << "\n";
            std::cout << "    Dom:";
            for (auto labelId : block->dom) {
                std::cout << " " << labelId;
            }
            std::cout << "\n";
            if (block->idom != NO_BLOCK_ID) {
                std::cout << "    Immediate Dom: " << block->idom << "\n";
            }
        }
        std::cout << "-----------------------\n";
        // http://www.webgraphviz.com/
        std::cout << "digraph CFG {\n  rankdir=TB;\n";
        for (auto& [labelId, block] : blocks) {
            if (unreached.find(labelId) == unreached.end()) {
                for (auto pred : block->pred) {
                    // XXX this is laid out much better if the function's start
                    // block is mentioned first.
                    if (unreached.find(pred) == unreached.end()) {
                        std::cout << "  \"" << pred << "\" -> \"" << labelId << "\"";
                        std::cout << ";\n";
                    }
                }
                if (block->idom != NO_BLOCK_ID) {
                    std::cout << "  \"" << labelId << "\" -> \"" << block->idom << "\"";
                    std::cout << " [color=\"0.000, 0.999, 0.999\"]";
                    std::cout << ";\n";
                }
            }
        }
        std::cout << "}\n";
        std::cout << "-----------------------\n";
    }

    // Dump instruction info.
    if (verbose) {
        std::cout << "----------------------- Instruction info\n";
        for (int pc = 0; pc < instructions.size(); pc++) {
            Instruction *instruction = instructions[pc].get();

            for(auto &function : functions) {
                if(pc == function.second.start) {
                    std::string name = cleanUpFunctionName(function.first);
                    std::cout << name << ":\n";
                }
            }

            for(auto &label : labels) {
                if(pc == label.second) {
                    std::cout << label.first << ":\n";
                }
            }

            std::ios oldState(nullptr);
            oldState.copyfmt(std::cout);

            std::cout
                << std::setw(5) << pc;
            if (instruction->blockId == NO_BLOCK_ID) {
                std::cout << "  ---";
            } else {
                std::cout << std::setw(5) << instruction->blockId;
            }
            if (instruction->resIdSet.empty()) {
                std::cout << "        ";
            } else {
                for (uint32_t resId : instruction->resIdList) {
                    std::cout << std::setw(5) << resId;
                }
                std::cout << " <-";
            }

            std::cout << std::setw(0) << " " << instruction->name();

            for (auto argId : instruction->argIdList) {
                std::cout << " " << argId;
            }

            std::cout << " (pred";
            for (auto line : instruction->pred) {
                std::cout << " " << line;
            }
            std::cout << ", succ";
            for (auto line : instruction->succ) {
                std::cout << " " << line;
            }
            std::cout << ", live";
            for (auto &[label,liveinSet] : instruction->livein) {
                std::cout << " " << label << "(";
                for (auto regId : liveinSet) {
                    std::cout << " " << regId;
                }
                std::cout << ")";
            }

            std::cout << ")\n";

            std::cout.copyfmt(oldState);
        }
        std::cout << "-----------------------\n";
    }

    // Check that all blocks were assigned properly.
    for (auto& [labelId, block] : blocks) {
        // It's okay to have no idom as long as you're the first block in the function.
        if ((block->idom == NO_BLOCK_ID) != block->pred.empty()) {
            std::cout << "----\n"
                << labelId << "\n"
                << block->idom << "\n"
                << block->pred.size() << "\n";
        }
        assert((block->idom == NO_BLOCK_ID) == block->pred.empty());
    }
}

void Program::expandVectors() {
    // Map from instruction index before and after, for remapping other data structures.
    std::vector<uint32_t> lineMap;
    InstructionList newList;

    /* XXX delete
    // Expand variables.
    std::map<uint32_t, Variable> newVariables;
    for (auto &[id, var] : variables) {
        const TypeVector *typeVector = getTypeAsVector(var.type);
        if (typeVector != nullptr) {
            assert(var.initializer == NO_INITIALIZER);
            for (int i = 0; i < typeVector->count; i++) {
                auto [subtype, offset] = getConstituentInfo(var.type, i);
                uint32_t scalarReg = scalarize(id, i, subtype);
                newVariables[scalarReg] = {
                    subtype, var.storageClass, NO_INITIALIZER, 0xFFFFFFFF
                };
                auto itr = names.find(id);
                if (itr != names.end()) {
                    names[scalarReg] = itr->second;
                }
            }
        }
    }
    for (auto &[id, var] : newVariables) {
        variables[id] = var;
    }
    */

    for (uint32_t pc = 0; pc < instructions.size(); pc++) {
        bool replaced = false;
        const std::shared_ptr<Instruction> &instructionPtr = instructions[pc];
        Instruction *instruction = instructionPtr.get();

        // Keep track of where this instructions ends up.
        lineMap.push_back(newList.size());

        switch (instruction->opcode()) {
            case SpvOpLoad: {
                InsnLoad *insn = dynamic_cast<InsnLoad *>(instruction);
                const TypeVector *typeVector = getTypeAsVector(
                        resultTypes.at(insn->resultId));
                if (typeVector != nullptr) {
                    for (int i = 0; i < typeVector->count; i++) {
                        auto [subtype, offset] = getConstituentInfo(insn->type, i);
                        newList.push_back(std::make_shared<RiscVLoad>(insn->lineInfo,
                                    subtype,
                                    scalarize(insn->resultId, i, subtype),
                                    insn->pointerId,
                                    insn->memoryAccess,
                                    i*typeSizes.at(subtype)));
                    }
                } else {
                    newList.push_back(std::make_shared<RiscVLoad>(insn->lineInfo,
                                insn->type,
                                insn->resultId,
                                insn->pointerId,
                                insn->memoryAccess,
                                0));
                }
                replaced = true;
                break;
            }

            case SpvOpStore: {
                InsnStore *insn = dynamic_cast<InsnStore *>(instruction);
                const TypeVector *typeVector = nullptr;
                // XXX handle case of objectId being a constant.
                auto itr = resultTypes.find(insn->objectId);
                if (itr != resultTypes.end()) {
                    typeVector = getTypeAsVector(itr->second);
                }
                if (typeVector != nullptr) {
                    for (int i = 0; i < typeVector->count; i++) {
                        auto [subtype, offset] = getConstituentInfo(
                                resultTypes.at(insn->objectId), i);
                        newList.push_back(std::make_shared<RiscVStore>(insn->lineInfo,
                                    insn->pointerId,
                                    scalarize(insn->objectId, i, subtype),
                                    insn->memoryAccess,
                                    i*typeSizes.at(subtype)));
                    }
                } else {
                    newList.push_back(std::make_shared<RiscVStore>(insn->lineInfo,
                                insn->pointerId,
                                insn->objectId,
                                insn->memoryAccess,
                                0));
                }
                replaced = true;
                break;
            }

            case SpvOpCompositeConstruct: {
                InsnCompositeConstruct *insn =
                    dynamic_cast<InsnCompositeConstruct *>(instruction);
                const TypeVector *typeVector = getTypeAsVector(insn->type);
                assert(typeVector != nullptr);
                for (int i = 0; i < typeVector->count; i++) {
                    auto [subtype, offset] = getConstituentInfo(insn->type, i);
                    // Re-use existing SSA register.
                    scalarize(insn->resultId, i, subtype, insn->constituentsId[i]);
                }
                replaced = true;
                break;
            }

            case SpvOpCompositeExtract: {
                InsnCompositeExtract *insn = dynamic_cast<InsnCompositeExtract *>(instruction);

                // Only handle the simple case.
                assert(isTypeFloat(insn->type));
                assert(insn->indexesId.size() == 1);
                uint32_t index = insn->indexesId[0];

                uint32_t oldId = scalarize(insn->compositeId, index, insn->type);

                // XXX We could avoid this move by renaming the result register throughout
                // to "oldId".
                newList.push_back(std::make_shared<RiscVMove>(insn->lineInfo,
                            insn->type, insn->resultId, oldId));

                replaced = true;
                break;
            }

            case SpvOpCompositeInsert: { // XXX not tested.
                InsnCompositeInsert *insn = dynamic_cast<InsnCompositeInsert *>(instruction);

                // Only handle the simple case.
                const TypeVector *typeVector = getTypeAsVector(insn->type);
                assert(typeVector != nullptr);
                assert(insn->indexesId.size() == 1);
                uint32_t index = insn->indexesId[0];

                for (int i = 0; i < typeVector->count; i++) {
                    uint32_t newId;

                    if (i == index) {
                        // Use new value.
                        newId = insn->objectId;
                    } else {
                        // Use old value.
                        newId = scalarize(insn->compositeId, i, typeVector->type);
                    }
                    scalarize(insn->resultId, i, typeVector->type, newId);
                }

                replaced = true;
                break;
            }

            case SpvOpVectorTimesScalar: {
                InsnVectorTimesScalar *insn = dynamic_cast<InsnVectorTimesScalar *>(instruction);

                const TypeVector *typeVector = getTypeAsVector(insn->type);
                assert(typeVector != nullptr);

                // Break into individual floating point multiplies.
                for (int i = 0; i < typeVector->count; i++) {
                    newList.push_back(std::make_shared<InsnFMul>(insn->lineInfo,
                                typeVector->type,
                                scalarize(insn->resultId, i, typeVector->type),
                                scalarize(insn->vectorId, i, typeVector->type),
                                insn->scalarId));
                }

                replaced = true;
                break;
            }

            case SpvOpVectorShuffle: {
                InsnVectorShuffle *insn = dynamic_cast<InsnVectorShuffle *>(instruction);

                const TypeVector *typeVector = getTypeAsVector(insn->type);
                assert(typeVector != nullptr);

                const TypeVector *typeVector1 = getTypeAsVector(typeIdOf(insn->vector1Id));
                assert(typeVector1 != nullptr);
                size_t n1 = typeVector1->count;

                for (int i = 0; i < insn->componentsId.size(); i++) {
                    uint32_t component = insn->componentsId[i];
                    uint32_t vectorId;

                    if (component < n1) {
                        vectorId = insn->vector1Id;
                    } else {
                        vectorId = insn->vector2Id;
                        component -= n1;
                    }

                    // Use same register for both.
                    uint32_t newId = scalarize(vectorId, component, typeVector->type);
                    scalarize(insn->resultId, i, typeVector->type, newId);
                }

                replaced = true;
                break;
            }

            case SpvOpFOrdEqual:
                 expandVectorsBinOp<InsnFOrdEqual>(instruction, newList, replaced);
                 break;

            case SpvOpFOrdLessThan:
                 expandVectorsBinOp<InsnFOrdLessThan>(instruction, newList, replaced);
                 break;

            case SpvOpFOrdGreaterThan:
                 expandVectorsBinOp<InsnFOrdGreaterThan>(instruction, newList, replaced);
                 break;

            case SpvOpIEqual:
                 expandVectorsBinOp<InsnIEqual>(instruction, newList, replaced);
                 break;

            case SpvOpSLessThan:
                 expandVectorsBinOp<InsnSLessThan>(instruction, newList, replaced);
                 break;

            case SpvOpIAdd:
                 expandVectorsBinOp<InsnIAdd>(instruction, newList, replaced);
                 break;

            case SpvOpFAdd:
                 expandVectorsBinOp<InsnFAdd>(instruction, newList, replaced);
                 break;

            case SpvOpFSub:
                 expandVectorsBinOp<InsnFSub>(instruction, newList, replaced);
                 break;

            case SpvOpFMul:
                 expandVectorsBinOp<InsnFMul>(instruction, newList, replaced);
                 break;

            case SpvOpFDiv:
                 expandVectorsBinOp<InsnFDiv>(instruction, newList, replaced);
                 break;

            case SpvOpFMod:
                 expandVectorsBinOp<InsnFMod>(instruction, newList, replaced);
                 break;

            case SpvOpFNegate:
                 expandVectorsUniOp<InsnFNegate>(instruction, newList, replaced);
                 break;

            case SpvOpConvertSToF:
                 expandVectorsUniOp<InsnConvertSToF>(instruction, newList, replaced);
                 break;

            case SpvOpConvertFToS:
                 expandVectorsUniOp<InsnConvertFToS>(instruction, newList, replaced);
                 break;

            case SpvOpLogicalNot:
                 expandVectorsUniOp<InsnLogicalNot>(instruction, newList, replaced);
                 break;

            case SpvOpDot: {
                InsnDot *insn = dynamic_cast<InsnDot *>(instruction);

                const TypeVector *typeVector = getTypeAsVector(resultTypes.at(insn->vector1Id));

                std::vector<uint32_t> vector1Ids;
                std::vector<uint32_t> vector2Ids;
                if (typeVector != nullptr) {
                    // Operand is vector.
                    for (int i = 0; i < typeVector->count; i++) {
                        vector1Ids.push_back(scalarize(insn->vector1Id, i, typeVector->type));
                        vector2Ids.push_back(scalarize(insn->vector2Id, i, typeVector->type));
                    }
                } else {
                    // Operand is scalar.
                    vector1Ids.push_back(insn->vector1Id);
                    vector2Ids.push_back(insn->vector2Id);
                }
                newList.push_back(std::make_shared<RiscVDot>(insn->lineInfo,
                            insn->type,
                            insn->resultId,
                            vector1Ids,
                            vector2Ids));

                replaced = true;
                break;
            }

            case SpvOpAll: {
                InsnAll *insn = dynamic_cast<InsnAll *>(instruction);

                const TypeVector *typeVector = getTypeAsVector(resultTypes.at(insn->vectorId));
                assert(typeVector != nullptr);

                std::vector<uint32_t> operandIds;
                for (int i = 0; i < typeVector->count; i++) {
                    operandIds.push_back(scalarize(insn->vectorId, i, typeVector->type));
                }
                newList.push_back(std::make_shared<RiscVAll>(insn->lineInfo,
                            insn->type,
                            insn->resultId,
                            operandIds));

                replaced = true;
                break;
            }

            case SpvOpAny: {
                InsnAny *insn = dynamic_cast<InsnAny *>(instruction);

                const TypeVector *typeVector = getTypeAsVector(resultTypes.at(insn->vectorId));
                assert(typeVector != nullptr);

                std::vector<uint32_t> operandIds;
                for (int i = 0; i < typeVector->count; i++) {
                    operandIds.push_back(scalarize(insn->vectorId, i, typeVector->type));
                }
                newList.push_back(std::make_shared<RiscVAny>(insn->lineInfo,
                            insn->type,
                            insn->resultId,
                            operandIds));

                replaced = true;
                break;
            }

            case SpvOpPhi: {
                InsnPhi *insn = dynamic_cast<InsnPhi *>(instruction);
                const TypeVector *typeVector = getTypeAsVector(
                        resultTypes.at(insn->resultId));
                if (typeVector != nullptr) {
                    // Operands are <data, label> pairs.
                    for (int i = 0; i < typeVector->count; i++) {
                        std::vector<uint32_t> operandIds;
                        for (int j = 0; j < insn->operandId.size(); j += 2) {
                            operandIds.push_back(scalarize(insn->operandId[j], i, typeVector->type));
                            operandIds.push_back(insn->operandId[j + 1]);
                        }
                        newList.push_back(std::make_shared<InsnPhi>(insn->lineInfo,
                                    insn->type,
                                    scalarize(insn->resultId, i, typeVector->type),
                                    operandIds));
                    }
                    replaced = true;
                }
                break;
            }

            case SpvOpSelect:
                expandVectorsTerOp<InsnSelect>(instruction, newList, replaced);
                break;

            case 0x10000 | GLSLstd450Sin:
                 expandVectorsUniOp<InsnGLSLstd450Sin>(instruction, newList, replaced);
                 break;

            case 0x10000 | GLSLstd450Cos:
                 expandVectorsUniOp<InsnGLSLstd450Cos>(instruction, newList, replaced);
                 break;

            case 0x10000 | GLSLstd450Atan2:
                expandVectorsBinOp<InsnGLSLstd450Atan2>(instruction, newList, replaced);
                break;

            case 0x10000 | GLSLstd450FAbs:
                expandVectorsUniOp<InsnGLSLstd450FAbs>(instruction, newList, replaced);
                break;

            case 0x10000 | GLSLstd450Fract:
                expandVectorsUniOp<InsnGLSLstd450Fract>(instruction, newList, replaced);
                break;

            case 0x10000 | GLSLstd450Floor:
                expandVectorsUniOp<InsnGLSLstd450Floor>(instruction, newList, replaced);
                break;

            case 0x10000 | GLSLstd450FClamp:
                 expandVectorsTerOp<InsnGLSLstd450FClamp>(instruction, newList, replaced);
                 break;

            case 0x10000 | GLSLstd450FMix:
                 expandVectorsTerOp<InsnGLSLstd450FMix>(instruction, newList, replaced);
                 break;

            case 0x10000 | GLSLstd450FMin:
                 expandVectorsBinOp<InsnGLSLstd450FMin>(instruction, newList, replaced);
                 break;

            case 0x10000 | GLSLstd450FMax:
                 expandVectorsBinOp<InsnGLSLstd450FMax>(instruction, newList, replaced);
                 break;

            case 0x10000 | GLSLstd450Step:
                 expandVectorsBinOp<InsnGLSLstd450Step>(instruction, newList, replaced);
                 break;

            case 0x10000 | GLSLstd450Pow:
                 expandVectorsBinOp<InsnGLSLstd450Pow>(instruction, newList, replaced);
                 break;

            case 0x10000 | GLSLstd450Exp:
                 expandVectorsUniOp<InsnGLSLstd450Exp>(instruction, newList, replaced);
                 break;

            case 0x10000 | GLSLstd450Length: {
                InsnGLSLstd450Length *insn = dynamic_cast<InsnGLSLstd450Length *>(instruction);

                const TypeVector *typeVector = getTypeAsVector(resultTypes.at(insn->xId));

                std::vector<uint32_t> operandIds;
                if (typeVector != nullptr) {
                    // Operand is vector.
                    for (int i = 0; i < typeVector->count; i++) {
                        operandIds.push_back(scalarize(insn->xId, i, typeVector->type));
                    }
                } else {
                    // Operand is scalar.
                    operandIds.push_back(insn->xId);
                }
                newList.push_back(std::make_shared<RiscVLength>(insn->lineInfo,
                            insn->type,
                            insn->resultId,
                            operandIds));

                replaced = true;
                break;
            }

            case 0x10000 | GLSLstd450Normalize: {
                InsnGLSLstd450Normalize *insn =
                    dynamic_cast<InsnGLSLstd450Normalize *>(instruction);

                const TypeVector *typeVector = getTypeAsVector(resultTypes.at(insn->xId));

                std::vector<uint32_t> resultIds;
                std::vector<uint32_t> operandIds;
                if (typeVector != nullptr) {
                    // Operand is vector.
                    for (int i = 0; i < typeVector->count; i++) {
                        resultIds.push_back(scalarize(insn->resultId, i, typeVector->type));
                        operandIds.push_back(scalarize(insn->xId, i, typeVector->type));
                    }
                } else {
                    // Operand is scalar.
                    resultIds.push_back(insn->resultId);
                    operandIds.push_back(insn->xId);
                }
                newList.push_back(std::make_shared<RiscVNormalize>(insn->lineInfo,
                            insn->type,
                            resultIds,
                            operandIds));

                replaced = true;
                break;
            }

            case 0x10000 | GLSLstd450Cross: {
                InsnGLSLstd450Cross *insn =
                    dynamic_cast<InsnGLSLstd450Cross *>(instruction);
                auto [subtype, offset] = getConstituentInfo(insn->type, 0);
                newList.push_back(std::make_shared<RiscVCross>(insn->lineInfo,
                            scalarize(insn->resultId, 0, subtype),
                            scalarize(insn->resultId, 1, subtype),
                            scalarize(insn->resultId, 2, subtype),
                            scalarize(insn->xId, 0, subtype),
                            scalarize(insn->xId, 1, subtype),
                            scalarize(insn->xId, 2, subtype),
                            scalarize(insn->yId, 0, subtype),
                            scalarize(insn->yId, 1, subtype),
                            scalarize(insn->yId, 2, subtype)));
                replaced = true;
                break;
            }

            case 0x10000 | GLSLstd450Distance: {
                InsnGLSLstd450Distance *insn = dynamic_cast<InsnGLSLstd450Distance *>(instruction);

                const TypeVector *typeVector = getTypeAsVector(resultTypes.at(insn->p0Id));

                std::vector<uint32_t> p0Id;
                std::vector<uint32_t> p1Id;
                if (typeVector != nullptr) {
                    // Operand is vector.
                    for (int i = 0; i < typeVector->count; i++) {
                        p0Id.push_back(scalarize(insn->p0Id, i, typeVector->type));
                        p1Id.push_back(scalarize(insn->p1Id, i, typeVector->type));
                    }
                } else {
                    // Operand is scalar.
                    p0Id.push_back(insn->p0Id);
                    p1Id.push_back(insn->p1Id);
                }
                newList.push_back(std::make_shared<RiscVDistance>(insn->lineInfo,
                            insn->type,
                            insn->resultId,
                            p0Id,
                            p1Id));

                replaced = true;
                break;
            }

            case SpvOpReturn:
            case SpvOpBranch:
            case SpvOpBranchConditional:
            case SpvOpAccessChain: {
                // Nothing to do for these.
                break;
            }

            default: {
                std::cerr << "Warning: Unhandled opcode "
                    << OpcodeToString.at(instruction->opcode())
                    << " when expanding vectors.\n";
                break;
            }
        }

        if (!replaced) {
            newList.push_back(instructionPtr);
        }
    }

    // Remap other data structures.
    for (auto &itr : labels) {
        itr.second = lineMap[itr.second];
    }

    std::swap(newList, instructions);
}

