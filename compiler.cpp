
#include <iomanip>

#include "compiler.h"
#include "risc-v.h"

void Compiler::compile() {
    // Transform SPIR-V instructions to RISC-V instructions.
    transformInstructions(pgm->instructions, instructions);

    // Translate out of SSA by eliminating phi instructions.
    translateOutOfSsa();

    // Perform physical register assignment.
    assignRegisters();

    // Emit our header.
    emit("jal ra, main", "");
    emit("ebreak", "");

    // Emit instructions.
    for(int pc = 0; pc < instructions.size(); pc++) {
        // Label the function if we're at its start.
        // XXX Make a map of functions by start instruction index, don't loop.
        for(auto &function : pgm->functions) {
            if(pc == function.second.start) {
                std::string name = pgm->cleanUpFunctionName(function.first);
                outFile << "; ---------------------------- function \"" << name << "\"\n";
                emitLabel(name);

                // Emit instructions to fill constants.
                for (auto regId : pgm->instructions.at(function.second.start)->liveinAll) {
                    auto r = registers.find(regId);
                    assert(r != registers.end());
                    assert(r->second.phy.size() == 1);
                    std::ostringstream ss;
                    if (isRegFloat(regId)) {
                        ss << "flw f" << (r->second.phy.at(0) - 32);
                    } else {
                        ss << "lw x" << r->second.phy.at(0);
                    }
                    ss << ", .C" << regId << "(x0)";

                    // Build comment with constant value.
                    std::ostringstream ssc;
                    ssc << "Load constant (";
                    Register const &pr = pgm->constants.at(regId);
                    uint32_t typeOp = pgm->getTypeOp(r->second.type);
                    switch (typeOp) {
                        case SpvOpTypeInt:
                            ssc << *reinterpret_cast<uint32_t *>(pr.data);
                            break;

                        case SpvOpTypeFloat:
                            ssc << *reinterpret_cast<float *>(pr.data);
                            break;

                        case SpvOpTypePointer:
                            // Do we get constant pointers?
                            ssc << "pointer";
                            break;

                        case SpvOpTypeBool:
                            ssc << (*reinterpret_cast<uint32_t *>(pr.data) ? "true" : "false");
                            break;

                        default:
                            ssc << "unknown type " << r->second.type << ", op " << typeOp;
                    }
                    ssc << ")";

                    emit(ss.str(), ssc.str());
                }
            }
        }

        for(auto &label : pgm->labels) {
            if(pc == label.second) {
                std::ostringstream ss;
                ss << "label" << label.first;
                emitLabel(ss.str());
            }
        }

        instructions.at(pc)->emit(this);
    }

    // Emit variables.
    for (auto &[id, var] : pgm->variables) {
        auto name = pgm->names.find(id);
        if (name != pgm->names.end()) {
            // XXX Check storage class? (var.storageClass)
            emitLabel(name->second);
            size_t size = pgm->typeSizes.at(var.type);
            for (size_t i = 0; i < size/4; i++) {
                emit(".word 0", "");
            }
            for (size_t i = 0; i < size%4; i++) {
                emit(".byte 0", "");
            }
        } else {
            std::cerr << "Warning: Name of variable " << id << " not defined.\n";
        }
    }

    // Emit constants.
    for (auto &[id, reg] : pgm->constants) {
        std::string name;
        auto nameItr = pgm->names.find(id);
        if (nameItr == pgm->names.end()) {
            std::ostringstream ss;
            ss << ".C" << id;
            name = ss.str();
        } else {
            name = nameItr->second;
        }
        emitLabel(name);
        emitConstant(id, reg.type, reg.data);
    }

    // Emit library.
    outFile << readFileContents("library.s");
}

void Compiler::emitConstant(uint32_t id, uint32_t typeId, unsigned char *data) {
    const Type *type = pgm->types.at(typeId).get();

    switch (type->op()) {
        case SpvOpTypeInt: {
            uint32_t value = *reinterpret_cast<uint32_t *>(data);
            std::ostringstream ss;
            ss << ".word " << value;
            emit(ss.str(), "");
            break;
        }

        case SpvOpTypeFloat: {
            uint32_t intValue = *reinterpret_cast<uint32_t *>(data);
            float floatValue = *reinterpret_cast<float *>(data);
            std::ostringstream ss1;
            ss1 << ".word 0x" << std::hex << intValue;
            std::ostringstream ss2;
            ss2 << "Float " << floatValue;
            emit(ss1.str(), ss2.str());
            break;
        }

        case SpvOpTypeVector: {
            const TypeVector *typeVector = pgm->getTypeAsVector(typeId);
            for (int i = 0; i < typeVector->count; i++) {
                auto [subtype, offset] = pgm->getConstituentInfo(typeId, i);
                emitConstant(id, subtype, data + offset);
            }
            break;
        }

        default:
            std::cerr << "Error: Unhandled type for constant " << id << ".\n";
            exit(EXIT_FAILURE);
    }
}

bool Compiler::asIntegerConstant(uint32_t id, uint32_t &value) const {
    auto r = pgm->constants.find(id);
    if (r != pgm->constants.end()) {
        const Type *type = pgm->types.at(r->second.type).get();
        if (type->op() == SpvOpTypeInt) {
            value = *reinterpret_cast<uint32_t *>(r->second.data);
            return true;
        }
    }

    return false;
}

bool Compiler::isRegFloat(uint32_t id) const {
    auto r = registers.find(id);
    assert(r != registers.end());

    return pgm->isTypeFloat(r->second.type);
}

std::string Compiler::makeLocalLabel() {
    std::ostringstream ss;
    ss << "local" << localLabelCounter;
    localLabelCounter++;
    return ss.str();
}

void Compiler::transformInstructions(const InstructionList &inList, InstructionList &outList) {
    InstructionList newList;

    for (uint32_t pc = 0; pc < inList.size(); pc++) {
        bool replaced = false;
        const std::shared_ptr<Instruction> &instructionPtr = inList[pc];
        Instruction *instruction = instructionPtr.get();
        if (instruction->opcode() == SpvOpIAdd) {
            InsnIAdd *insnIAdd = dynamic_cast<InsnIAdd *>(instruction);

            uint32_t imm;
            if (asIntegerConstant(insnIAdd->operand1Id(), imm)) {
                // XXX Verify that immediate fits in 12 bits.
                newList.push_back(std::make_shared<RiscVAddi>(insnIAdd->lineInfo,
                            insnIAdd->type, insnIAdd->resultId(), insnIAdd->operand2Id(), imm));
                replaced = true;
            } else if (asIntegerConstant(insnIAdd->operand2Id(), imm)) {
                // XXX Verify that immediate fits in 12 bits.
                newList.push_back(std::make_shared<RiscVAddi>(insnIAdd->lineInfo,
                            insnIAdd->type, insnIAdd->resultId(), insnIAdd->operand1Id(), imm));
                replaced = true;
            }
        }

        if (!replaced) {
            newList.push_back(instructionPtr);
        }
    }

    std::swap(newList, outList);
}

void Compiler::translateOutOfSsa() {
    // Compute the phi equivalent classes.
    // Disable this. It's efficient because it maps registers together, but
    // we need to make sure they don't interfere. See end of that function.
    /// computePhiClassMap();
}

void Compiler::computePhiClassMap() {
    phiClassMap.clear();

    // Go through all instructions look for phi.
    for (int pc = 0; pc < instructions.size(); pc++) {
        Instruction *insn = instructions.at(pc).get();
        // XXX This is still using the old phi instruction.
        if (insn->opcode() == SpvOpPhi) {
            std::shared_ptr<PhiClass> phiClass = std::make_shared<PhiClass>();
            for (uint32_t regId : insn->resIdSet) {
                processPhiRegister(regId, phiClass);
            }
            for (uint32_t regId : insn->argIdSet) {
                processPhiRegister(regId, phiClass);
            }
        }
    }

    if (pgm->verbose) {
        std::cout << "----------------------- Phi class info\n";
        for (auto itr : phiClassMap) {
            std::cout << itr.first << ":";
            for (uint32_t regId : itr.second->registers) {
                std::cout << " " << regId;
            }
            std::cout << " (" << itr.second->getId() << ")\n";
        }
        std::cout << "-----------------------\n";
    }

    // We've created all the phi classes. We must make sure that
    // no pair of registers in a class is ever live at the same time.
    // XXX to do!
}

void Compiler::processPhiRegister(uint32_t regId, std::shared_ptr<PhiClass> &phiClass) {
    auto itr = phiClassMap.find(regId);
    if (itr != phiClassMap.end()) {
        // This register is already in another class. Merge what
        // we have so far of this class into the other one.
        std::shared_ptr<PhiClass> otherPhiClass = itr->second;
        for (uint32_t prevRegId : phiClass->registers) {
            otherPhiClass->registers.insert(prevRegId);
            phiClassMap[prevRegId] = otherPhiClass;
        }

        // Continue adding to other existing class.
        phiClass = otherPhiClass;
    }

    // Record this register as part of this class.
    phiClass->registers.insert(regId);
    phiClassMap[regId] = phiClass;
}

void Compiler::assignRegisters() {
    // Set up all the virtual registers we'll deal with.
    for(auto [id, type]: pgm->resultTypes) {
        const TypeVector *typeVector = pgm->getTypeAsVector(type);
        int count = typeVector == nullptr ? 1 : typeVector->count;
        registers[id] = CompilerRegister {type, count};
    }

    // 32 registers; x0 is always zero; x1 is ra; x2 is sp.
    std::set<uint32_t> PHY_INT_REGS;
    for (int i = 3; i < 32; i++) {
        PHY_INT_REGS.insert(i);
    }

    // 32 float registers.
    std::set<uint32_t> PHY_FLOAT_REGS;
    for (int i = 0; i < 32; i++) {
        PHY_FLOAT_REGS.insert(i + 32);
    }

    // Start with blocks at the start of functions.
    for (auto& [id, function] : pgm->functions) {
        Block *block = pgm->blocks.at(function.labelId).get();

        // Assign registers for constants.
        std::set<uint32_t> constIntRegs = PHY_INT_REGS;
        std::set<uint32_t> constFloatRegs = PHY_FLOAT_REGS;
        for (auto regId : instructions.at(block->begin)->liveinAll) {
            if (registers.find(regId) != registers.end()) {
                std::cerr << "Error: Constant "
                    << regId << " already assigned a register at head of function.\n";
                exit(EXIT_FAILURE);
            }
            auto &c = pgm->constants.at(regId);
            auto r = CompilerRegister {c.type, 1}; // XXX get real count.
            uint32_t phy;
            if (pgm->isTypeFloat(c.type)) {
                assert(!constFloatRegs.empty());
                phy = *constFloatRegs.begin();
                constFloatRegs.erase(phy);
            } else {
                assert(!constIntRegs.empty());
                phy = *constIntRegs.begin();
                constIntRegs.erase(phy);
            }
            r.phy.push_back(phy);
            registers[regId] = r;
        }

        assignRegisters(block, PHY_INT_REGS, PHY_FLOAT_REGS);
    }
}

void Compiler::assignRegisters(Block *block,
        const std::set<uint32_t> &allIntPhy,
        const std::set<uint32_t> &allFloatPhy) {

    if (pgm->verbose) {
        std::cout << "assignRegisters(" << block->labelId << ")\n";
    }

    // Assigned physical registers.
    std::set<uint32_t> assigned;

    // Registers that are live going into this block have already been
    // assigned. Note that we use livein[0] here, not liveinAll, because
    // we want to ignore parameters to phi instructions. They're not
    // considered live here, we'll add copy instructions on the edge
    // between the block and here.
    for (auto regId : instructions.at(block->begin)->livein.at(0)) {
        auto r = registers.find(regId);
        if (r == registers.end()) {
            std::cerr << "Warning: Initial virtual register "
                << regId << " not found in block " << block->labelId << ".\n";
        } else if (r->second.phy.empty()) {
            std::cerr << "Warning: Expected initial physical register for "
                << regId << " in block " << block->labelId << ".\n";
        } else {
            for (auto phy : r->second.phy) {
                assigned.insert(phy);
            }
        }
    }

    for (int pc = block->begin; pc < block->end; pc++) {
        Instruction *instruction = instructions.at(pc).get();

        // Free up now-unused physical registers.
        for (auto argId : instruction->argIdSet) {
            // If this virtual register doesn't survive past this line, then we
            // can use its physical register again.
            if (instruction->liveout.find(argId) == instruction->liveout.end()) {
                auto r = registers.find(argId);
                if (r != registers.end()) {
                    for (auto phy : r->second.phy) {
                        assigned.erase(phy);
                    }
                }
            }
        }

        // Assign result registers to physical registers.
        for (uint32_t resId : instruction->resIdSet) {
            auto r = registers.find(resId);
            if (r == registers.end()) {
                std::cout << "Error: Virtual register "
                    << resId << " not found in block " << block->labelId << ".\n";
                exit(EXIT_FAILURE);
            }

            // Register set to pick from for this register.
            const std::set<uint32_t> &allPhy =
                pgm->isTypeFloat(r->second.type) ? allFloatPhy : allIntPhy;

            // For each element in this virtual register...
            for (int i = 0; i < r->second.count; i++) {
                // Find an available physical register for this element.
                bool found = false;
                for (uint32_t phy : allPhy) {
                    if (assigned.find(phy) == assigned.end()) {
                        r->second.phy.push_back(phy);
                        // If the result lives past this instruction, consider its
                        // register assigned.
                        if (instruction->liveout.find(resId) != instruction->liveout.end()) {
                            assigned.insert(phy);
                        }
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    std::cout << "Error: No physical register available for "
                        << resId << "[" << i << "] on line " << pc << ".\n";
                    exit(EXIT_FAILURE);
                }
            }
        }
    }

    // Go down dominance tree.
    for (auto& [labelId, subBlock] : pgm->blocks) {
        if (subBlock->idom == block->labelId) {
            assignRegisters(subBlock.get(), allIntPhy, allFloatPhy);
        }
    }
}

bool Compiler::isSamePhysicalRegister(uint32_t id1, uint32_t id2, int index) const {
    auto r1 = registers.find(id1);
    if (r1 != registers.end() && index < r1->second.phy.size()) {
        auto r2 = registers.find(id2);
        if (r2 != registers.end() && index < r2->second.phy.size()) {
            return r1->second.phy[index] == r2->second.phy[index];
        }
    }

    return false;
}

const CompilerRegister *Compiler::asRegister(uint32_t id) const {
    auto r = registers.find(id);

    return r == registers.end() ? nullptr : &r->second;
}

std::string Compiler::reg(uint32_t id, int index) const {
    std::ostringstream ss;

    auto r = asRegister(id);
    if (r != nullptr) {
        uint32_t phy = r->phy[index];
        if (phy < 32) {
            ss << "x" << phy;
        } else {
            ss << "f" << (phy - 32);
        }
    } else {
        auto constant = pgm->constants.find(id);
        if (constant != pgm->constants.end() &&
                pgm->types.at(constant->second.type)->op() == SpvOpTypeFloat) {

            const float *f = reinterpret_cast<float *>(constant->second.data);
            ss << *f;
        } else {
            auto name = pgm->names.find(id);
            if (name != pgm->names.end()) {
                ss << name->second;
            } else {
                ss << "r" << id;
            }
        }
    }

    return ss.str();
}

void Compiler::emitNotImplemented(const std::string &op) {
    std::ostringstream ss;

    ss << op << " not implemented in compiler";

    emit("#error#", ss.str());

    std::cerr << "Warning: " << ss.str() << ".\n";
}

void Compiler::emitUnaryOp(const std::string &opName, int result, int op) {
    auto r = registers.find(result);
    int count = r == registers.end() ? 1 : r->second.count;

    for (int i = 0; i < count; i++) {
        std::ostringstream ss1;
        ss1 << opName << " " << reg(result, i) << ", " << reg(op, i);
        std::ostringstream ss2;
        ss2 << "r" << result << " = " << opName << " r" << op;
        emit(ss1.str(), ss2.str());
    }
}

void Compiler::emitBinaryOp(const std::string &opName, int result, int op1, int op2) {
    auto r = registers.find(result);
    int count = r == registers.end() ? 1 : r->second.count;

    for (int i = 0; i < count; i++) {
        std::ostringstream ss1;
        ss1 << opName << " " << reg(result, i) << ", " << reg(op1, i) << ", " << reg(op2, i);
        std::ostringstream ss2;
        ss2 << "r" << result << " = " << opName << " r" << op1 << " r" << op2;
        emit(ss1.str(), ss2.str());
    }
}

void Compiler::emitBinaryImmOp(const std::string &opName, int result, int op, uint32_t imm) {
    auto r = registers.find(result);
    int count = r == registers.end() ? 1 : r->second.count;

    for (int i = 0; i < count; i++) {
        std::ostringstream ss1;
        ss1 << opName << " " << reg(result, i) << ", " << reg(op, i) << ", " << imm;
        std::ostringstream ss2;
        ss2 << "r" << result << " = " << opName << " r" << op << " " << imm;
        emit(ss1.str(), ss2.str());
    }
}

void Compiler::emitLabel(const std::string &label) {
    outFile << notEmptyLabel(label) << ":\n";
}

std::string Compiler::notEmptyLabel(const std::string &label) const {
    return label.empty() ? ".anonymous" : label;
}

void Compiler::emitCall(const std::string &functionName,
        const std::vector<uint32_t> resultIds,
        const std::vector<uint32_t> operandIds) {

    // Push parameters.
    {
        std::ostringstream ss;
        ss << "addi sp, sp, -" << 4*(operandIds.size() + 1);
        emit(ss.str(), "Make room on stack");
    }
    {
        std::ostringstream ss;
        ss << "sw ra, " << 4*operandIds.size() << "(sp)";
        emit(ss.str(), "Save return address");
    }

    for (int i = operandIds.size() - 1; i >= 0; i--) {
        std::ostringstream ss;
        ss << "fsw " << reg(operandIds[i]) << ", " << i*4 << "(sp)";
        emit(ss.str(), "Push parameter");
    }

    // Call routines.
    {
        std::ostringstream ss;
        ss << "jal ra, " << functionName;
        emit(ss.str(), "Call routine");
    }

    // Pop parameters.
    for (int i = 0; i < resultIds.size(); i++) {
        std::ostringstream ss;
        ss << "flw " << reg(resultIds[i]) << ", " << i*4 << "(sp)";
        emit(ss.str(), "Pop result");
    }

    {
        std::ostringstream ss;
        ss << "lw ra, " << 4*resultIds.size() << "(sp)";
        emit(ss.str(), "Restore return address");
    }
    {
        std::ostringstream ss;
        ss << "addi sp, sp, " << 4*(resultIds.size() + 1);
        emit(ss.str(), "Restore stack");
    }
}

void Compiler::emitUniCall(const std::string &functionName, uint32_t resultId, uint32_t operandId) {
    std::vector<uint32_t> operandIds;
    std::vector<uint32_t> resultIds;

    operandIds.push_back(operandId);
    resultIds.push_back(resultId);

    emitCall(functionName, resultIds, operandIds);
}

void Compiler::emitBinCall(const std::string &functionName, uint32_t resultId,
        uint32_t operand1Id, uint32_t operand2Id) {

    std::vector<uint32_t> operandIds;
    std::vector<uint32_t> resultIds;

    operandIds.push_back(operand1Id);
    operandIds.push_back(operand2Id);
    resultIds.push_back(resultId);

    emitCall(functionName, resultIds, operandIds);
}

void Compiler::emit(const std::string &op, const std::string &comment) {
    std::ios oldState(nullptr);
    oldState.copyfmt(outFile);

    outFile
        << "        "
        << std::left
        << std::setw(30) << op
        << std::setw(0);
    if(!comment.empty()) {
        outFile << "; " << comment;
    }
    outFile << "\n";

    outFile.copyfmt(oldState);
}

void Compiler::emitPhiCopy(Instruction *instruction, uint32_t labelId) {
    // Where we're coming from.
    uint32_t sourceBlockId = instruction->blockId;

    // Find block with phi.
    Block *block = pgm->blocks.at(labelId).get();
    int pc = block->begin;
    if (pc >= block->end) {
        // No instructions in block.
        return;
    }

    // Find the phi instruction.
    Instruction *firstInstruction = instructions[pc].get();
    if (firstInstruction->opcode() != RiscVOpPhi) {
        // Block doesn't start with a phi.
        return;
    }
    RiscVPhi *phi = dynamic_cast<RiscVPhi *>(firstInstruction);

    // Find the block corresponding to ours.
    bool found = false;
    for (int labelIndex = 0; !found && labelIndex < phi->labelIds.size(); labelIndex++) {
        if (phi->labelIds[labelIndex] == sourceBlockId) {
            found = true;

            // Copy each source to its result.
            // XXX Must do this in parallel.
            for (int resultIndex = 0; resultIndex < phi->resultIds.size(); resultIndex++) {
                uint32_t resultId = phi->resultIds[resultIndex];
                uint32_t sourceId = phi->operandIds[resultIndex][labelIndex];

                std::ostringstream ss;
                if (isSamePhysicalRegister(resultId, sourceId, 0)) {
                    ss << "; ";
                }
                ss << "mov " << reg(resultId) << ", " << reg(sourceId);
                emit(ss.str(), "phi elimination");
            }
        }
    }

    if (!found) {
        std::cerr << "Error: Can't find source block " << sourceBlockId
            << " in phi at start of block " << labelId << "\n";
    }
}

void Compiler::assertNoPhi(uint32_t labelId) {
    Block *block = pgm->blocks.at(labelId).get();
    uint32_t opcode = instructions[block->begin]->opcode();
    assert(opcode != SpvOpPhi);
    assert(opcode != RiscVOpPhi);
}
