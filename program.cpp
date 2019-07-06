#include <iomanip>

#include "program.h"
#include "risc-v.h"


std::map<uint32_t, std::string> OpcodeToString = {
#include "opcode_to_string.h"
};

// Convenience function to get the number of items in a variable.
// If it's a vector, returns its count. Otherwise returns 1.
static int vectorCount(const TypeVector *typeVector) {
    return typeVector == nullptr ? 1 : typeVector->count;
}

Program::Program(bool throwOnUnimplemented_, bool verbose_) :
    throwOnUnimplemented(throwOnUnimplemented_),
    hasUnimplemented(false),
    verbose(verbose_),
    mainFunctionId(NO_FUNCTION)
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
uint32_t Program::scalarize(uint32_t vreg, int i, uint32_t subtype, uint32_t scalarReg,
        bool mustExist) {

    auto regIndex = RegIndex{vreg, i};
    auto itr = vec2scalar.find(regIndex);
    if (itr == vec2scalar.end()) {
        assert(!mustExist);

        // See if this was a constant vector.
        auto itr = constants.find(vreg);
        if (itr != constants.end()) {
            // Just refer back to the original component.
            // XXX This won't handle matrices, maybe?
            scalarReg = itr->second.subelements.at(i);
        } else {
            // Not a constant, must be a variable.
            if (scalarReg == 0) {
                scalarReg = nextReg++;
                assert(this->resultTypes.find(scalarReg) == this->resultTypes.end());
                this->resultTypes[scalarReg] = subtype;
            }

            vec2scalar[regIndex] = scalarReg;
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

uint32_t Program::scalarize(uint32_t vreg, int i, const TypeVector *typeVector) {
    return typeVector == nullptr ? vreg : scalarize(vreg, i, typeVector->type);
}

// Post-parsing work.
void Program::postParse() {
    // Find the main function.
    mainFunctionId = NO_FUNCTION;
    for(auto& e: entryPoints) {
        if(e.second.name == "main") {
            mainFunctionId = e.first;
        }
    }

    // Allocated variables 
    for(auto& [id, var]: variables) {
        switch (var.storageClass) {
            case SpvStorageClassUniformConstant: {
                uint32_t binding = decorations.at(id).at(SpvDecorationBinding)[0];
                var.address = memoryRegions[SpvStorageClassUniformConstant].base + binding * 16; // XXX magic number
                storeNamedVariableInfo(names[id], var.type, var.address);
                break;
            }

            case SpvStorageClassUniform: {
                uint32_t binding = decorations.at(id).at(SpvDecorationBinding)[0];
                var.address = memoryRegions[SpvStorageClassUniform].base + binding * 256; // XXX magic number
                storeNamedVariableInfo(names[id], var.type, var.address);
                break;
            }

            case SpvStorageClassOutput: {
                uint32_t location;
                if(decorations.at(id).find(SpvDecorationLocation) == decorations.at(id).end()) {
                    throw std::runtime_error("no Location decoration available for output " + std::to_string(id));
                }

                location = decorations.at(id).at(SpvDecorationLocation)[0];
                var.address = memoryRegions[SpvStorageClassOutput].base + location * 256; // XXX magic number
                storeNamedVariableInfo(names[id], var.type, var.address);
                break;
            }

            case SpvStorageClassInput: {
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
                break;
            }

            default: {
                var.address = allocate(var.storageClass, var.type);
                break;
            }
        }
    }
    if(verbose) {
        std::cout << "----------------------- Variable binding and location info\n";
        for(auto& [name, info]: namedVariables) {
            std::cout << "variable " << name << " is at " << info.address << '\n';
        }
    }
}

void Program::prepareForCompile() {
    // Replace phis with ours.
    replacePhi();

    // Compute successor and predecessor blocks.
    for (auto& [functionId, function] : functions) {
        for (auto& [_, block] : function->blocks) {
            Instruction *instruction = block->instructions.tail.get();
            assert(instruction->isTermination());
            block->succ = instruction->targetLabelIds;
            for (uint32_t blockId : block->succ) {
                function->blocks[blockId]->pred.insert(block->blockId);
            }
        }
    }

    // Break loops by renaming variables in phi instructions.
    for (auto &[_, function] : functions) {
        function->phiLifting();
    }

    // Convert vector instructions to scalar instructions.
    expandVectors();

    // Compute livein and liveout registers for each line.
    Timer timer;
    std::set<Instruction *> inst_worklist; // Instructions left to work on.
    for (auto &[_, function] : functions) {
        for (auto &[_, block] : function->blocks) {
            for (auto inst = block->instructions.head; inst; inst = inst->next) {
                inst_worklist.insert(inst.get());
            }
        }
    }
    while (!inst_worklist.empty()) {
        // Pick any PC to start with. Starting at the end is more efficient
        // since our predecessor depends on us.
        Instruction *instruction = *inst_worklist.rbegin();
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
                        variables.find(argId) == variables.end()) {

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

            // Compute union of livein.
            instruction->liveinAll.clear();
            for (auto &[label,liveinSet] : instruction->livein) {
                instruction->liveinAll.insert(liveinSet.begin(), liveinSet.end());
            }
        }
    }
    if (PRINT_TIMER_RESULTS) {
        std::cerr << "Livein and liveout took " << timer.elapsed() << " seconds.\n";
    }

    // Compute the dominance tree for blocks. Use a worklist.
    for (auto &[_, function] : functions) {
        function->computeDomTree(verbose);
    }
}

void Program::replacePhi() {
    for (auto &[_, function] : functions) {
        replacePhiInFunction(function.get());
    }
}

void Program::replacePhiInFunction(Function *function) {
    for (auto &[_, block] : function->blocks) {
        replacePhiInBlock(block.get());
    }
}

void Program::replacePhiInBlock(Block *block) {
    InstructionList newList(block);

    std::shared_ptr<Instruction> nextInst;
    for (auto inst = block->instructions.head; inst; inst = nextInst) {
        // Save this because if we move the instruction to another list, its next
        // pointer will be wrong.
        nextInst = inst->next;

        Instruction *instruction = inst.get();
        bool replaced;

        switch (instruction->opcode()) {
            case SpvOpPhi: {
                // Here we collapse all the consecutive phi instructions into our own.
                std::shared_ptr<RiscVPhi> newPhi = std::make_shared<RiscVPhi>(
                        instruction->lineInfo);
                newList.push_back(newPhi);

                // Eat up all consecutive phis.
                bool first = true;
                while (inst && inst->opcode() == SpvOpPhi) {
                    InsnPhi *oldPhi = dynamic_cast<InsnPhi *>(inst.get());

                    uint32_t resultId = oldPhi->resultId();
                    newPhi->resultIds.push_back(resultId);
                    newPhi->addResult(resultId);

                    std::vector<uint32_t> operandIds;
                    for (int i = 0; i < oldPhi->operandIdCount(); i++) {
                        // Deal with label.
                        uint32_t labelId = oldPhi->labelId[i];
                        if (first) {
                            newPhi->labelIds.push_back(labelId);
                        } else {
                            // Make sure labels are consistent across phi instructions.
                            assert(newPhi->labelIds[i] == labelId);
                        }

                        // Deal with operand.
                        uint32_t operandId = oldPhi->operandId(i);
                        operandIds.push_back(operandId);
                        newPhi->addParameter(operandId);
                    }
                    newPhi->operandIds.push_back(operandIds);

                    first = false;
                    inst = inst->next;
                }
                nextInst = inst;

                replaced = true;
                break;
            }

            default:
                // Leave as-is.
                replaced = false;
                break;
        }

        if (!replaced) {
            newList.push_back(inst);
        }
    }

    newList.swap(block->instructions);
}

void Program::expandVectors() {
    for (auto &[_, function] : functions) {
        expandVectorsInFunction(function.get());
    }
}

void Program::expandVectorsInFunction(Function *function) {
    for (auto &[_, block] : function->blocks) {
        expandVectorsInBlock(block.get());
    }
}

void Program::expandVectorsInBlock(Block *block) {
    InstructionList newList(block);

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

    std::shared_ptr<Instruction> nextInst;
    for (auto inst = block->instructions.head; inst; inst = nextInst) {
        // Save this because if we move the instruction to another list, its next
        // pointer will be wrong.
        nextInst = inst->next;

        Instruction *instruction = inst.get();
        bool replaced = false;

        switch (instruction->opcode()) {
            case SpvOpLoad: {
                InsnLoad *insn = dynamic_cast<InsnLoad *>(instruction);
                const TypeVector *typeVector = getTypeAsVector(
                        resultTypes.at(insn->resultId()));
                if (typeVector != nullptr) {
                    for (int i = 0; i < typeVector->count; i++) {
                        auto [subtype, offset] = getConstituentInfo(insn->type, i);
                        newList.push_back(std::make_shared<RiscVLoad>(insn->lineInfo,
                                    subtype,
                                    scalarize(insn->resultId(), i, subtype),
                                    insn->pointerId(),
                                    insn->memoryAccess,
                                    i*typeSizes.at(subtype)));
                    }
                } else {
                    newList.push_back(std::make_shared<RiscVLoad>(insn->lineInfo,
                                insn->type,
                                insn->resultId(),
                                insn->pointerId(),
                                insn->memoryAccess,
                                0));
                }
                replaced = true;
                break;
            }

            case SpvOpStore: {
                InsnStore *insn = dynamic_cast<InsnStore *>(instruction);
                // Look for it as a variable.
                uint32_t type = 0;
                auto itr = resultTypes.find(insn->objectId());
                if (itr != resultTypes.end()) {
                    type = itr->second;
                } else {
                    // Look for it as a constant.
                    auto itr2 = constants.find(insn->objectId());
                    if (itr2 != constants.end()) {
                        type = itr2->second.type;
                    }
                }
                const TypeVector *typeVector = type == 0 ? nullptr : getTypeAsVector(type);
                if (typeVector != nullptr) {
                    for (int i = 0; i < typeVector->count; i++) {
                        auto [subtype, offset] = getConstituentInfo(type, i);
                        newList.push_back(std::make_shared<RiscVStore>(insn->lineInfo,
                                    insn->pointerId(),
                                    scalarize(insn->objectId(), i, subtype),
                                    insn->memoryAccess,
                                    i*typeSizes.at(subtype)));
                    }
                } else {
                    newList.push_back(std::make_shared<RiscVStore>(insn->lineInfo,
                                insn->pointerId(),
                                insn->objectId(),
                                insn->memoryAccess,
                                0));
                }
                replaced = true;
                break;
            }

            case SpvOpCompositeConstruct: {
                InsnCompositeConstruct *insn =
                    dynamic_cast<InsnCompositeConstruct *>(instruction);
                const TypeMatrix *typeMatrix = getTypeAsMatrix(insn->type);
                if (typeMatrix != nullptr) {
                    const TypeVector *typeVector = getTypeAsVector(typeMatrix->columnType);
                    assert(typeVector != nullptr);

                    for (int col = 0; col < typeMatrix->columnCount; col++) {
                        uint32_t vectorId = insn->constituentsId(col);

                        for (int row = 0; row < typeVector->count; row++) {
                            int index = typeMatrix->getIndex(row, col, typeVector);

                            // Re-use existing SSA register.
                            uint32_t id = scalarize(vectorId, row, typeVector->type, 0, true);
                            scalarize(insn->resultId(), index, typeVector->type, id);
                        }
                    }
                } else {
                    const TypeVector *typeVector = getTypeAsVector(insn->type);
                    assert(typeVector != nullptr);
                    for (int i = 0; i < typeVector->count; i++) {
                        // Re-use existing SSA register.
                        scalarize(insn->resultId(), i, typeVector->type, insn->constituentsId(i));
                    }
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

                uint32_t oldId = scalarize(insn->compositeId(), index, insn->type);

                // XXX We could avoid this move by renaming the result register throughout
                // to "oldId".
                newList.push_back(std::make_shared<InsnCopyObject>(insn->lineInfo,
                            insn->type, insn->resultId(), oldId));

                replaced = true;
                break;
            }

            case SpvOpCopyObject: {
                expandVectorsUniOp<InsnCopyObject>(instruction, newList, replaced);
                break;
            }

            case SpvOpVectorTimesMatrix: {
                InsnVectorTimesMatrix *insn = dynamic_cast<InsnVectorTimesMatrix *>(instruction);

                // Vectors are rows.
                const TypeMatrix *typeMatrix = getTypeAsMatrix(resultTypes.at(insn->matrixId()));
                assert(typeMatrix != nullptr);
                const TypeVector *matVector = getTypeAsVector(typeMatrix->columnType);
                assert(matVector != nullptr);
                assert(typeMatrix->columnType == resultTypes.at(insn->vectorId()));
                const TypeVector *resVector = getTypeAsVector(insn->type);
                assert(resVector != nullptr);
                assert(resVector->count == typeMatrix->columnCount);

                // Dot the source vector with each column vector of the matrix.
                for (int col = 0; col < typeMatrix->columnCount; col++) {
                    std::vector<uint32_t> vector1Ids;
                    std::vector<uint32_t> vector2Ids;
                    for (int row = 0; row < matVector->count; row++) {
                        int index = typeMatrix->getIndex(row, col, matVector);
                        vector1Ids.push_back(scalarize(insn->vectorId(), row, matVector->type));
                        vector2Ids.push_back(scalarize(insn->matrixId(), index, matVector->type));
                    }
                    newList.push_back(std::make_shared<RiscVDot>(insn->lineInfo,
                                resVector->type,
                                scalarize(insn->resultId(), col, resVector->type),
                                vector1Ids,
                                vector2Ids));
                }

                replaced = true;
                break;
            }

            case SpvOpMatrixTimesVector: {
                InsnMatrixTimesVector *insn = dynamic_cast<InsnMatrixTimesVector *>(instruction);

                // Vectors are columns.
                const TypeMatrix *typeMatrix = getTypeAsMatrix(resultTypes.at(insn->matrixId()));
                assert(typeMatrix != nullptr);
                const TypeVector *matVector = getTypeAsVector(typeMatrix->columnType);
                assert(matVector != nullptr);
                const TypeVector *opVector = getTypeAsVector(resultTypes.at(insn->vectorId()));
                assert(opVector != nullptr);
                assert(opVector->count == typeMatrix->columnCount);
                const TypeVector *resVector = getTypeAsVector(insn->type);
                assert(resVector != nullptr);
                assert(matVector == resVector);

                // Dot the source vector with each row vector of the matrix.
                for (int row = 0; row < matVector->count; row++) {
                    std::vector<uint32_t> vector1Ids;
                    std::vector<uint32_t> vector2Ids;
                    for (int col = 0; col < typeMatrix->columnCount; col++) {
                        int index = typeMatrix->getIndex(row, col, matVector);
                        vector1Ids.push_back(scalarize(insn->vectorId(), col, matVector->type));
                        vector2Ids.push_back(scalarize(insn->matrixId(), index, matVector->type));
                    }
                    newList.push_back(std::make_shared<RiscVDot>(insn->lineInfo,
                                resVector->type,
                                scalarize(insn->resultId(), row, resVector->type),
                                vector1Ids,
                                vector2Ids));
                }

                replaced = true;
                break;
            }

            case SpvOpCompositeInsert: { // XXX not tested.
                InsnCompositeInsert *insn = dynamic_cast<InsnCompositeInsert *>(instruction);

                // Only handle the simple case.
                if (getTypeOp(insn->type) == SpvOpTypeMatrix) {
                    std::cerr << "SpvOpCompositeInsert doesn't handle matrices.\n";
                    exit(1);
                }
                const TypeVector *typeVector = getTypeAsVector(insn->type);
                assert(typeVector != nullptr);
                assert(insn->indexesId.size() == 1);
                uint32_t index = insn->indexesId[0];

                for (int i = 0; i < typeVector->count; i++) {
                    uint32_t newId;

                    if (i == index) {
                        // Use new value.
                        newId = insn->objectId();
                    } else {
                        // Use old value.
                        newId = scalarize(insn->compositeId(), i, typeVector->type);
                    }
                    scalarize(insn->resultId(), i, typeVector->type, newId);
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
                                scalarize(insn->resultId(), i, typeVector->type),
                                scalarize(insn->vectorId(), i, typeVector->type),
                                insn->scalarId()));
                }

                replaced = true;
                break;
            }

            case SpvOpVectorShuffle: {
                InsnVectorShuffle *insn = dynamic_cast<InsnVectorShuffle *>(instruction);

                const TypeVector *typeVector = getTypeAsVector(insn->type);
                assert(typeVector != nullptr);

                const TypeVector *typeVector1 = getTypeAsVector(typeIdOf(insn->vector1Id()));
                assert(typeVector1 != nullptr);
                size_t n1 = typeVector1->count;

                for (int i = 0; i < insn->componentsId.size(); i++) {
                    uint32_t component = insn->componentsId[i];
                    uint32_t vectorId;

                    if (component < n1) {
                        vectorId = insn->vector1Id();
                    } else {
                        vectorId = insn->vector2Id();
                        component -= n1;
                    }

                    // Use same register for both.
                    uint32_t newId = scalarize(vectorId, component, typeVector->type);
                    scalarize(insn->resultId(), i, typeVector->type, newId);
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

            case SpvOpFOrdGreaterThanEqual:
                 expandVectorsBinOp<InsnFOrdGreaterThanEqual>(instruction, newList, replaced);
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

            case SpvOpLogicalAnd:
                 expandVectorsBinOp<InsnLogicalAnd>(instruction, newList, replaced);
                 break;

            case SpvOpLogicalOr:
                 expandVectorsBinOp<InsnLogicalOr>(instruction, newList, replaced);
                 break;

            case SpvOpDot: {
                InsnDot *insn = dynamic_cast<InsnDot *>(instruction);

                const TypeVector *typeVector = getTypeAsVector(resultTypes.at(insn->vector1Id()));

                std::vector<uint32_t> vector1Ids;
                std::vector<uint32_t> vector2Ids;
                if (typeVector != nullptr) {
                    // Operand is vector.
                    for (int i = 0; i < typeVector->count; i++) {
                        vector1Ids.push_back(scalarize(insn->vector1Id(), i, typeVector->type));
                        vector2Ids.push_back(scalarize(insn->vector2Id(), i, typeVector->type));
                    }
                } else {
                    // Operand is scalar.
                    vector1Ids.push_back(insn->vector1Id());
                    vector2Ids.push_back(insn->vector2Id());
                }
                newList.push_back(std::make_shared<RiscVDot>(insn->lineInfo,
                            insn->type,
                            insn->resultId(),
                            vector1Ids,
                            vector2Ids));

                replaced = true;
                break;
            }

            case SpvOpAll: {
                InsnAll *insn = dynamic_cast<InsnAll *>(instruction);

                const TypeVector *typeVector = getTypeAsVector(resultTypes.at(insn->vectorId()));
                assert(typeVector != nullptr);

                std::vector<uint32_t> operandIds;
                for (int i = 0; i < typeVector->count; i++) {
                    operandIds.push_back(scalarize(insn->vectorId(), i, typeVector->type));
                }
                newList.push_back(std::make_shared<RiscVAll>(insn->lineInfo,
                            insn->type,
                            insn->resultId(),
                            operandIds));

                replaced = true;
                break;
            }

            case SpvOpAny: {
                InsnAny *insn = dynamic_cast<InsnAny *>(instruction);

                const TypeVector *typeVector = getTypeAsVector(resultTypes.at(insn->vectorId()));
                assert(typeVector != nullptr);

                std::vector<uint32_t> operandIds;
                for (int i = 0; i < typeVector->count; i++) {
                    operandIds.push_back(scalarize(insn->vectorId(), i, typeVector->type));
                }
                newList.push_back(std::make_shared<RiscVAny>(insn->lineInfo,
                            insn->type,
                            insn->resultId(),
                            operandIds));

                replaced = true;
                break;
            }

            case SpvOpPhi: {
                // Should have been replaced.
                assert(false);
            }

            case RiscVOpPhi: {
                RiscVPhi *oldPhi = dynamic_cast<RiscVPhi *>(instruction);
                std::shared_ptr<RiscVPhi> newPhi = std::make_shared<RiscVPhi>(oldPhi->lineInfo);
                newList.push_back(newPhi);

                // Can just copy these, they won't change.
                newPhi->labelIds = oldPhi->labelIds;

                for (int resIndex = 0; resIndex < oldPhi->resultIds.size(); resIndex++) {
                    // See if we should expand a vector.
                    const TypeVector *typeVector = getTypeAsVector(typeIdOf(
                                oldPhi->resultIds[resIndex]));

                    // Expand vectors.
                    for (int elIndex = 0; elIndex < vectorCount(typeVector); elIndex++) {
                        uint32_t regId = scalarize(oldPhi->resultIds[resIndex],
                                elIndex, typeVector);
                        newPhi->resultIds.push_back(regId);
                        newPhi->addResult(regId);

                        // Source labels.
                        std::vector<uint32_t> operandIds;
                        for (int labelIndex = 0; labelIndex < oldPhi->labelIds.size();
                                labelIndex++) {

                            uint32_t operandId = oldPhi->operandIds[resIndex][labelIndex];

                            uint32_t regId = scalarize(operandId, elIndex, typeVector);
                            operandIds.push_back(regId);
                            newPhi->addParameter(regId);
                        }
                        newPhi->operandIds.push_back(operandIds);
                    }
                }

                replaced = true;
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

            case 0x10000 | GLSLstd450Exp2:
                expandVectorsUniOp<InsnGLSLstd450Exp2>(instruction, newList, replaced);
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

            case 0x10000 | GLSLstd450SmoothStep:
                 expandVectorsTerOp<InsnGLSLstd450SmoothStep>(instruction, newList, replaced);
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

                const TypeVector *typeVector = getTypeAsVector(resultTypes.at(insn->xId()));

                std::vector<uint32_t> operandIds;
                if (typeVector != nullptr) {
                    // Operand is vector.
                    for (int i = 0; i < typeVector->count; i++) {
                        operandIds.push_back(scalarize(insn->xId(), i, typeVector->type));
                    }
                } else {
                    // Operand is scalar.
                    operandIds.push_back(insn->xId());
                }
                newList.push_back(std::make_shared<RiscVLength>(insn->lineInfo,
                            insn->type,
                            insn->resultId(),
                            operandIds));

                replaced = true;
                break;
            }

            case 0x10000 | GLSLstd450Normalize: {
                InsnGLSLstd450Normalize *insn =
                    dynamic_cast<InsnGLSLstd450Normalize *>(instruction);

                const TypeVector *typeVector = getTypeAsVector(resultTypes.at(insn->xId()));

                std::vector<uint32_t> resultIds;
                std::vector<uint32_t> operandIds;
                if (typeVector != nullptr) {
                    // Operand is vector.
                    for (int i = 0; i < typeVector->count; i++) {
                        resultIds.push_back(scalarize(insn->resultId(), i, typeVector->type));
                        operandIds.push_back(scalarize(insn->xId(), i, typeVector->type));
                    }
                } else {
                    // Operand is scalar.
                    resultIds.push_back(insn->resultId());
                    operandIds.push_back(insn->xId());
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
                            scalarize(insn->resultId(), 0, subtype),
                            scalarize(insn->resultId(), 1, subtype),
                            scalarize(insn->resultId(), 2, subtype),
                            scalarize(insn->xId(), 0, subtype),
                            scalarize(insn->xId(), 1, subtype),
                            scalarize(insn->xId(), 2, subtype),
                            scalarize(insn->yId(), 0, subtype),
                            scalarize(insn->yId(), 1, subtype),
                            scalarize(insn->yId(), 2, subtype)));
                replaced = true;
                break;
            }

            case 0x10000 | GLSLstd450Distance: {
                InsnGLSLstd450Distance *insn = dynamic_cast<InsnGLSLstd450Distance *>(instruction);

                const TypeVector *typeVector = getTypeAsVector(resultTypes.at(insn->p0Id()));

                std::vector<uint32_t> p0Id;
                std::vector<uint32_t> p1Id;
                if (typeVector != nullptr) {
                    // Operand is vector.
                    for (int i = 0; i < typeVector->count; i++) {
                        p0Id.push_back(scalarize(insn->p0Id(), i, typeVector->type));
                        p1Id.push_back(scalarize(insn->p1Id(), i, typeVector->type));
                    }
                } else {
                    // Operand is scalar.
                    p0Id.push_back(insn->p0Id());
                    p1Id.push_back(insn->p1Id());
                }
                newList.push_back(std::make_shared<RiscVDistance>(insn->lineInfo,
                            insn->type,
                            insn->resultId(),
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
            newList.push_back(inst);
        }
    }

    newList.swap(block->instructions);
}

