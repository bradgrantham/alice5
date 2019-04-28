#include <iostream>
#include <vector>
#include <functional>
#include <set>
#include <cstdio>
#include <fstream>
#include <chrono>
#include <thread>
#include <algorithm>
#include <atomic>
#include <sstream>
#include <iomanip>
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <StandAlone/ResourceLimits.h>
#include <glslang/MachineIndependent/localintermediate.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/Include/intermediate.h>
#include <SPIRV/GlslangToSpv.h>
#include <SPIRV/disassemble.h>
#include <spirv-tools/libspirv.h>
#include <spirv-tools/optimizer.hpp>
#include "spirv.h"
#include "GLSL.std.450.h"

#include "basic_types.h"
#include "risc-v.h"
#include "image.h"
#include "interpreter.h"
#include "program.h"
#include "interpreter_tmpl.h"
#include "shadertoy.h"
#include "timer.h"

#define DEFAULT_WIDTH (640/2)
#define DEFAULT_HEIGHT (360/2)

// Enable this to check if our virtual RAM is being initialized properly.
#define CHECK_MEMORY_ACCESS
// Enable this to check if our virtual registers are being initialized properly.
#define CHECK_REGISTER_ACCESS

// -----------------------------------------------------------------------------------

// Virtual register used by the compiler.
struct CompilerRegister {
    // Type of the data.
    uint32_t type;

    // Number of words of data in this register.
    int count;

    // Physical registers.
    std::vector<uint32_t> phy;

    CompilerRegister()
        : type(NO_REGISTER)
    {
        // Nothing.
    }

    CompilerRegister(uint32_t type, int count)
        : type(type), count(count)
    {
        // Nothing.
    }
};

// Compiles a Program to our ISA.
struct Compiler
{
    const Program *pgm;
    std::map<uint32_t, CompilerRegister> registers;
    uint32_t localLabelCounter;
    InstructionList instructions;

    Compiler(const Program *pgm)
        : pgm(pgm), localLabelCounter(1)
    {
        // Nothing.
    }

    void compile()
    {
        // Transform SPIR-V instructions to RISC-V instructions.
        transformInstructions(pgm->instructions, instructions);

        // Perform physical register assignment.
        assignRegisters();

        // Emit our header.
        emit("jal ra, main", "");
        emit("ebreak", "");

        // Emit instructions.
        for(int pc = 0; pc < instructions.size(); pc++) {
            for(auto &function : pgm->functions) {
                if(pc == function.second.start) {
                    std::string name = pgm->cleanUpFunctionName(function.first);
                    std::cout << "; ---------------------------- function \"" << name << "\"\n";
                    emitLabel(name);

                    // Emit instructions to fill constants.
                    for (auto regId : pgm->instructions.at(function.second.start)->livein) {
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
                        emit(ss.str(), "Load constant");
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
    }

    // Emit constant value for the specified constant ID.
    void emitConstant(uint32_t id, uint32_t typeId, unsigned char *data) {
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

    // If the virtual register "id" points to an integer constant, returns it
    // in "value" and returns true. Otherwise returns false and leaves value
    // untouched.
    bool asIntegerConstant(uint32_t id, uint32_t &value) const {
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

    // Returns true if the register is a float, false if it's an integer,
    // otherwise asserts.
    bool isRegFloat(uint32_t id) const {
        auto r = registers.find(id);
        assert(r != registers.end());

        return pgm->isTypeFloat(r->second.type);
    }

    // Make a new label that can be used for local jumps.
    std::string makeLocalLabel() {
        std::ostringstream ss;
        ss << "local" << localLabelCounter;
        localLabelCounter++;
        return ss.str();
    }

    // Transform SPIR-V instructions to RISC-V instructions. Creates a new
    // "instructions" array that's local to the compiler and is closer to
    // machine instructions, but still using SSA. inList and outList
    // may be the same list.
    void transformInstructions(const InstructionList &inList, InstructionList &outList) {
        InstructionList newList;

        for (uint32_t pc = 0; pc < inList.size(); pc++) {
            bool replaced = false;
            const std::shared_ptr<Instruction> &instructionPtr = inList[pc];
            Instruction *instruction = instructionPtr.get();
            if (instruction->opcode() == SpvOpIAdd) {
                InsnIAdd *insnIAdd = dynamic_cast<InsnIAdd *>(instruction);

                uint32_t imm;
                if (asIntegerConstant(insnIAdd->operand1Id, imm)) {
                    // XXX Verify that immediate fits in 12 bits.
                    newList.push_back(std::make_shared<RiscVAddi>(insnIAdd->lineInfo,
                                insnIAdd->type, insnIAdd->resultId, insnIAdd->operand2Id, imm));
                    replaced = true;
                } else if (asIntegerConstant(insnIAdd->operand2Id, imm)) {
                    // XXX Verify that immediate fits in 12 bits.
                    newList.push_back(std::make_shared<RiscVAddi>(insnIAdd->lineInfo,
                                insnIAdd->type, insnIAdd->resultId, insnIAdd->operand1Id, imm));
                    replaced = true;
                }
            }

            if (!replaced) {
                newList.push_back(instructionPtr);
            }
        }

        std::swap(newList, outList);
    }

    void assignRegisters() {
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
            for (auto regId : instructions.at(block->begin)->livein) {
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

    // Assigns registers for this block.
    void assignRegisters(Block *block,
            const std::set<uint32_t> &allIntPhy,
            const std::set<uint32_t> &allFloatPhy) {

        if (pgm->verbose) {
            std::cout << "assignRegisters(" << block->labelId << ")\n";
        }

        // Assigned physical registers.
        std::set<uint32_t> assigned;

        // Registers that are live going into this block have already been
        // assigned.
        for (auto regId : instructions.at(block->begin)->livein) {
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

    // Determine whether these two virtual register IDs are currently assigned
    // to the same physical register. Returns false if one or both aren't
    // assigned at all. Index is the index into the vector, if it is one.
    bool isSamePhysicalRegister(uint32_t id1, uint32_t id2, int index) const {
        auto r1 = registers.find(id1);
        if (r1 != registers.end() && index < r1->second.phy.size()) {
            auto r2 = registers.find(id2);
            if (r2 != registers.end() && index < r2->second.phy.size()) {
                return r1->second.phy[index] == r2->second.phy[index];
            }
        }

        return false;
    }

    // Returns the SSA ID as a compiler register, or null if it's not stored
    // in a compiler register.
    const CompilerRegister *asRegister(uint32_t id) const {
        auto r = registers.find(id);

        return r == registers.end() ? nullptr : &r->second;
    }

    // String for a virtual register ("r12" or more).
    std::string reg(uint32_t id, int index = 0) const {
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

    void emitNotImplemented(const std::string &op)
    {
        std::ostringstream ss;

        ss << op << " not implemented";

        emit("nop", ss.str());
    }

    void emitUnaryOp(const std::string &opName, int result, int op)
    {
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

    void emitBinaryOp(const std::string &opName, int result, int op1, int op2)
    {
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

    void emitBinaryImmOp(const std::string &opName, int result, int op, uint32_t imm)
    {
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

    void emitLabel(const std::string &label)
    {
        std::cout << notEmptyLabel(label) << ":\n";
    }

    // Return the label passed in, unless it's an empty string, in which case
    // it returns an appropriate non-empty string.
    std::string notEmptyLabel(const std::string &label) const {
        return label.empty() ? ".anonymous" : label;
    }

    void emit(const std::string &op, const std::string &comment)
    {
        std::ios oldState(nullptr);
        oldState.copyfmt(std::cout);

        std::cout
            << "        "
            << std::left
            << std::setw(30) << op
            << std::setw(0);
        if(!comment.empty()) {
            std::cout << "; " << comment;
        }
        std::cout << "\n";

        std::cout.copyfmt(oldState);
    }

    // Just before a Branch or BranchConditional instruction, copy any
    // registers that a target OpPhi instruction might need.
    void emitPhiCopy(Instruction *instruction, uint32_t labelId) {
        Block *block = pgm->blocks.at(labelId).get();
        for (int pc = block->begin; pc < block->end; pc++) {
            Instruction *nextInstruction = instructions[pc].get();
            if (nextInstruction->opcode() != SpvOpPhi) {
                // No more phi instructions for this block.
                break;
            }

            InsnPhi *phi = dynamic_cast<InsnPhi *>(nextInstruction);

            // Find the block corresponding to ours.
            bool found = false;
            for (int i = 0; !found && i < phi->operandId.size(); i += 2) {
                uint32_t srcId = phi->operandId[i];
                uint32_t parentId = phi->operandId[i + 1];

                if (parentId == instruction->blockId) {
                    found = true;

                    // XXX need to iterate over all registers of a vector.
                    std::ostringstream ss;
                    if (isSamePhysicalRegister(phi->resultId, srcId, 0)) {
                        ss << "; ";
                    }
                    ss << "mov " << reg(phi->resultId)
                        << ", " << reg(srcId);
                    emit(ss.str(), "phi elimination");
                }
            }
            if (!found) {
                std::cerr << "Error: Can't find source block " << instruction->blockId
                    << " in phi assigning to " << phi->resultId << "\n";
            }
        }
    }

    // Assert that the block at the label ID does not start with a Phi instruction.
    void assertNoPhi(uint32_t labelId) {
        Block *block = pgm->blocks.at(labelId).get();
        assert(instructions[block->begin]->opcode() != SpvOpPhi);
    }
};

// -----------------------------------------------------------------------------------

void RiscVAddi::emit(Compiler *compiler)
{
    compiler->emitBinaryImmOp("addi", resultId, rs1, imm);
}

void RiscVLoad::emit(Compiler *compiler)
{
    std::ostringstream ss1;
    if (compiler->isRegFloat(resultId)) {
        ss1 << "flw ";
    } else {
        ss1 << "lw ";
    }
    ss1 << compiler->reg(resultId) << ", ";
    auto r = compiler->asRegister(pointerId);
    if (r == nullptr) {
        // It's a variable reference.
        ss1 << compiler->reg(pointerId);
        if (offset != 0) {
            ss1 << "+" << offset;
        }
        ss1 << "(x0)";
    } else {
        // It's a register reference.
        ss1 << offset << "(" << compiler->reg(pointerId) << ")";
    }
    std::ostringstream ss2;
    ss2 << "r" << resultId << " = (r" << pointerId << ")";
    compiler->emit(ss1.str(), ss2.str());
}

void RiscVStore::emit(Compiler *compiler)
{
    std::ostringstream ss1;
    if (compiler->isRegFloat(objectId)) {
        ss1 << "fsw ";
    } else {
        ss1 << "sw ";
    }
    ss1 << compiler->reg(objectId) << ", " << compiler->reg(pointerId);
    if (offset != 0) {
        ss1 << "+" << offset;
    }
    ss1 << "(x0)";
    std::ostringstream ss2;
    ss2 << "(r" << pointerId << ") = r" << objectId;
    compiler->emit(ss1.str(), ss2.str());
}

void RiscVCross::emit(Compiler *compiler)
{
    compiler->emit("addi sp, sp, -28", "Make room on stack");
    compiler->emit("sw ra, 24(sp)", "Save return address");

    for (int i = argIdList.size() - 1; i >= 0; i--) {
        std::ostringstream ss;
        ss << "fsw " << compiler->reg(argIdList[i]) << ", " << (i*4) << "(sp)";
        compiler->emit(ss.str(), "Push parameter");
    }

    compiler->emit("jal ra, .cross", "Call routine");

    for (int i = 0; i < resIdList.size(); i++) {
        std::ostringstream ss;
        ss << "flw " << compiler->reg(resIdList[i]) << ", " << (i*4) << "(sp)";
        compiler->emit(ss.str(), "Pop result");
    }

    compiler->emit("lw ra, 12(sp)", "Restore return address");
    compiler->emit("addi sp, sp, 16", "Restore stack");
}

void RiscVMove::emit(Compiler *compiler)
{
    const CompilerRegister *r1 = compiler->asRegister(resultId);
    const CompilerRegister *r2 = compiler->asRegister(rs);
    assert(r1 != nullptr);
    assert(r2 != nullptr);

    std::ostringstream ss1;
    if (r1->phy != r2->phy) {
        if (compiler->isRegFloat(resultId)) {
            ss1 << "fsgnj.s ";
        } else {
            ss1 << "and ";
        }
        ss1 << compiler->reg(resultId) << ", "
            << compiler->reg(rs) << ", "
            << compiler->reg(rs);
    }
    std::ostringstream ss2;
    ss2 << "r" << resultId << " = r" << rs;
    compiler->emit(ss1.str(), ss2.str());
}

// -----------------------------------------------------------------------------------

Instruction::Instruction(const LineInfo& lineInfo)
    : lineInfo(lineInfo),
      blockId(NO_BLOCK_ID)
{
    // Nothing.
}

void Instruction::emit(Compiler *compiler)
{
    compiler->emitNotImplemented(name());
}

void InsnFAdd::emit(Compiler *compiler)
{
    compiler->emitBinaryOp("fadd.s", resultId, operand1Id, operand2Id);
}

void InsnFMul::emit(Compiler *compiler)
{
    compiler->emitBinaryOp("fmul.s", resultId, operand1Id, operand2Id);
}

void InsnFDiv::emit(Compiler *compiler)
{
    compiler->emitBinaryOp("fdiv.s", resultId, operand1Id, operand2Id);
}

void InsnIAdd::emit(Compiler *compiler)
{
    uint32_t intValue;
    if (compiler->asIntegerConstant(operand1Id, intValue)) {
        // XXX Verify that immediate fits in 12 bits.
        compiler->emitBinaryImmOp("addi", resultId, operand2Id, intValue);
    } else if (compiler->asIntegerConstant(operand2Id, intValue)) {
        // XXX Verify that immediate fits in 12 bits.
        compiler->emitBinaryImmOp("addi", resultId, operand1Id, intValue);
    } else {
        compiler->emitBinaryOp("add", resultId, operand1Id, operand2Id);
    }
}

void InsnFunctionCall::emit(Compiler *compiler)
{
    compiler->emit("push pc", "");
    for(int i = operandId.size() - 1; i >= 0; i--) {
        compiler->emit(std::string("push ") + compiler->reg(operandId[i]), "");
    }
    compiler->emit(std::string("call ") + compiler->pgm->cleanUpFunctionName(functionId), "");
    compiler->emit(std::string("pop ") + compiler->reg(resultId), "");
}

void InsnFunctionParameter::emit(Compiler *compiler)
{
    compiler->emit(std::string("pop ") + compiler->reg(resultId), "");
}

void InsnLoad::emit(Compiler *compiler)
{
    // We expand these to RiscVLoad.
    assert(false);
}

void InsnStore::emit(Compiler *compiler)
{
    // We expand these to RiscVStore.
    assert(false);
}

void InsnBranch::emit(Compiler *compiler)
{
    // See if we need to emit any copies for Phis at our target.
    compiler->emitPhiCopy(this, targetLabelId);

    std::ostringstream ss;
    ss << "j label" << targetLabelId;
    compiler->emit(ss.str(), "");
}

void InsnReturn::emit(Compiler *compiler)
{
    compiler->emit("jalr x0, ra, 0", "");
}

void InsnReturnValue::emit(Compiler *compiler)
{
    std::ostringstream ss1;
    ss1 << "mov x10, " << compiler->reg(valueId);
    std::ostringstream ss2;
    ss2 << "return " << valueId;
    compiler->emit(ss1.str(), ss2.str());
    compiler->emit("jalr x0, ra, 0", "");
}

void InsnPhi::emit(Compiler *compiler)
{
    compiler->emit("", "phi instruction, nothing to do.");
}

void InsnFOrdLessThanEqual::emit(Compiler *compiler)
{
    compiler->emitBinaryOp("lte", resultId, operand1Id, operand2Id);
}

void InsnBranchConditional::emit(Compiler *compiler)
{
    std::string localLabel = compiler->makeLocalLabel();

    std::ostringstream ss1;
    ss1 << "beq " << compiler->reg(conditionId)
        << ", x0, " << localLabel;
    std::ostringstream ssid;
    ssid << "r" << conditionId;
    compiler->emit(ss1.str(), ssid.str());
    // True path.
    compiler->emitPhiCopy(this, trueLabelId);
    std::ostringstream ss2;
    ss2 << "j label" << trueLabelId;
    compiler->emit(ss2.str(), "");
    // False path.
    compiler->emitLabel(localLabel);
    compiler->emitPhiCopy(this, falseLabelId);
    std::ostringstream ss3;
    ss3 << "j label" << falseLabelId;
    compiler->emit(ss3.str(), "");
}

void InsnAccessChain::emit(Compiler *compiler)
{
    uint32_t offset = 0;

    const Variable &variable = compiler->pgm->variables.at(baseId);
    uint32_t type = variable.type;
    for (auto &id : indexesId) {
        uint32_t intValue;
        if (compiler->asIntegerConstant(id, intValue)) {
            auto [subtype, subOffset] = compiler->pgm->getConstituentInfo(type, intValue);
            type = subtype;
            offset += subOffset;
        } else {
            std::cerr << "Error: Don't yet handle non-constant offsets in AccessChain ("
                << id << ").\n";
            exit(EXIT_FAILURE);
        }
    }

    auto name = compiler->pgm->names.find(baseId);
    if (name == compiler->pgm->names.end()) {
        std::cerr << "Error: Don't yet handle pointer reference in AccessChain ("
            << baseId << ").\n";
        exit(EXIT_FAILURE);
    }

    std::ostringstream ss;
    ss << "addi " << compiler->reg(resultId) << ", x0, "
        << compiler->notEmptyLabel(name->second) << "+" << offset;
    compiler->emit(ss.str(), "");
}

void InsnGLSLstd450Sin::emit(Compiler *compiler)
{
    compiler->emit("addi sp, sp, -8", "Make room on stack");
    compiler->emit("sw ra, 4(sp)", "Save return address");

    {
        std::ostringstream ss;
        ss << "fsw " << compiler->reg(xId) << ", 0(sp)";
        compiler->emit(ss.str(), "Push parameter");
    }

    compiler->emit("jal ra, .sin", "Call routine");

    {
        std::ostringstream ss;
        ss << "flw " << compiler->reg(resultId) << ", 0(sp)";
        compiler->emit(ss.str(), "Pop result");
    }

    compiler->emit("lw ra, 4(sp)", "Restore return address");
    compiler->emit("addi sp, sp, 8", "Restore stack");
}

// -----------------------------------------------------------------------------------

// Returns whether successful.
bool compileProgram(const Program &pgm)
{
    Compiler compiler(&pgm);

    compiler.compile();

    return true;
}

// -----------------------------------------------------------------------------------

void eval(Interpreter &interpreter, float x, float y, v4float& color)
{
    interpreter.clearPrivateVariables();
    interpreter.set(SpvStorageClassInput, 0, v4float {x, y}); // gl_FragCoord is always #0 
    interpreter.set(SpvStorageClassOutput, 0, color); // color is out #0 in preamble
    interpreter.run();
    interpreter.get(SpvStorageClassOutput, 0, color); // color is out #0 in preamble
}


std::string readFileContents(std::string shaderFileName)
{
    std::ifstream shaderFile(shaderFileName.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
    if(!shaderFile.good()) {
        throw std::runtime_error("couldn't open file " + shaderFileName + " for reading");
    }
    std::ifstream::pos_type size = shaderFile.tellg();
    shaderFile.seekg(0, std::ios::beg);

    std::string text(size, '\0');
    shaderFile.read(&text[0], size);

    return text;
}

std::string readStdin()
{
    std::istreambuf_iterator<char> begin(std::cin), end;
    return std::string(begin, end);
}

void usage(const char* progname)
{
    printf("usage: %s [options] shader.frag\n", progname);
    printf("provide \"-\" as a filename to read from stdin\n");
    printf("options:\n");
    printf("\t-f S E    Render frames S through and including E [0 0]\n");
    printf("\t-d W H    Render frame at size W by H [%d %d]\n",
            int(DEFAULT_WIDTH), int(DEFAULT_HEIGHT));
    printf("\t-j N      Use N threads [%d]\n",
            int(std::thread::hardware_concurrency()));
    printf("\t-v        Print opcodes as they are parsed\n");
    printf("\t-g        Generate debugging information\n");
    printf("\t-O        Run optimizing passes\n");
    printf("\t-t        Throw an exception on first unimplemented opcode\n");
    printf("\t-n        Compile and load shader, but do not shade an image\n");
    printf("\t-S        show the disassembly of the SPIR-V code\n");
    printf("\t-c        compile to our own ISA\n");
    printf("\t-s        convert vector operations to scalar operations\n");
    printf("\t--json    input file is a ShaderToy JSON file\n");
    printf("\t--term    draw output image on terminal (in addition to file)\n");
}

const std::string shaderPreambleFilename = "preamble.frag";
const std::string shaderEpilogueFilename = "epilogue.frag";

// Number of rows still left to shade (for progress report).
static std::atomic_int rowsLeft;

// Render rows starting at "startRow" every "skip".
void render(ShaderToyRenderPass* pass, int startRow, int skip, int frameNumber, float when)
{
    Interpreter interpreter(&pass->pgm);
    ImagePtr output = pass->outputs[0].sampledImage.image;

    interpreter.set("iResolution", v2float {static_cast<float>(output->width), static_cast<float>(output->height)});

    interpreter.set("iFrame", frameNumber);

    interpreter.set("iTime", when);

    interpreter.set("iMouse", v4float {0, 0, 0, 0});

    for(int i = 0; i < pass->inputs.size(); i++) {
        auto& input = pass->inputs[i];
        interpreter.set("iChannel" + std::to_string(input.channelNumber), i);
        ImagePtr image = input.sampledImage.image;
        float w = static_cast<float>(image->width);
        float h = static_cast<float>(image->height);
        interpreter.set("iChannelResolution[" + std::to_string(input.channelNumber) + "]", v3float{w, h, 0});
    }

    // This loop acts like a rasterizer fixed function block.  Maybe it should
    // set inputs and read outputs also.
    for(int y = startRow; y < output->height; y += skip) {
        for(int x = 0; x < output->width; x++) {
            v4float color;
            output->get(x, output->height - 1 - y, color);
            eval(interpreter, x + 0.5f, y + 0.5f, color);
            output->set(x, output->height - 1 - y, color);
        }

        rowsLeft--;
    }
}

// Thread to show progress to the user.
void showProgress(int totalRows, std::chrono::time_point<std::chrono::steady_clock> startTime)
{
    while(true) {
        int left = rowsLeft;
        if (left == 0) {
            break;
        }

        std::cout << left << " rows left of " << totalRows;

        // Estimate time left.
        if (left != totalRows) {
            auto now = std::chrono::steady_clock::now();
            auto elapsedTime = now - startTime;
            auto elapsedSeconds = double(elapsedTime.count())*
                std::chrono::steady_clock::period::num/
                std::chrono::steady_clock::period::den;
            auto secondsLeft = elapsedSeconds*left/(totalRows - left);

            std::cout << " (" << int(secondsLeft) << " seconds left)   ";
        }
        std::cout << "\r";
        std::cout.flush();

        // Wait one second while polling.
        for (int i = 0; i < 100 && rowsLeft > 0; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    // Clear the line.
    std::cout << "                                                             \r";
}

void earwigMessageConsumer(spv_message_level_t level, const char *source,
        const spv_position_t& position, const char *message)
{
    std::cout << source << ": " << message << "\n";
}

bool createSPIRVFromSources(const std::vector<ShaderSource>& sources, bool debug, bool optimize, std::vector<uint32_t>& spirv)
{
    glslang::TShader *shader = new glslang::TShader(EShLangFragment);

    glslang::TProgram& glslang_program = *new glslang::TProgram;

    std::vector<const char *> strings;
    std::vector<const char *> names;
    for(auto& source: sources) {
        strings.push_back(source.code.c_str());
        names.push_back(source.filename.c_str());
    }
    shader->setStringsWithLengthsAndNames(strings.data(), NULL, names.data(), sources.size());

    shader->setEnvInput(glslang::EShSourceGlsl, EShLangFragment, glslang::EShClientVulkan, glslang::EShTargetVulkan_1_0);

    shader->setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_1);

    shader->setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_3);

    EShMessages messages = (EShMessages)(EShMsgDefault | EShMsgSpvRules | EShMsgVulkanRules | EShMsgDebugInfo);

    glslang::TShader::ForbidIncluder includer;
    TBuiltInResource resources;

    resources = glslang::DefaultTBuiltInResource;

    if (!shader->parse(&resources, 110, false, messages, includer)) {
        std::cerr << "compile failed\n";
        std::cerr << shader->getInfoLog();
        return false;
    }

    glslang_program.addShader(shader);

    if(!glslang_program.link(messages)) {
        std::cerr << "link failed\n";
        std::cerr << glslang_program.getInfoLog();
        return false;
    }

    std::string warningsErrors;
    spv::SpvBuildLogger logger;
    glslang::SpvOptions options;
    options.generateDebugInfo = debug;
    options.disableOptimizer = !optimize;
    options.optimizeSize = false;
    glslang::TIntermediate *shaderInterm = glslang_program.getIntermediate(EShLangFragment);
    glslang::GlslangToSpv(*shaderInterm, spirv, &logger, &options);

    return true;
}

void optimizeSPIRV(spv_target_env targetEnv, std::vector<uint32_t>& spirv)
{
    Timer timer;
    spvtools::Optimizer optimizer(targetEnv);
    optimizer.SetMessageConsumer(earwigMessageConsumer);
    optimizer.RegisterPerformancePasses();
    // optimizer.SetPrintAll(&std::cerr);
    spvtools::OptimizerOptions optimizerOptions;
    bool success = optimizer.Run(spirv.data(), spirv.size(), &spirv, optimizerOptions);
    if (!success) {
        std::cout << "Warning: Optimizer failed.\n";
    }
    std::cerr << "Optimizing took " << timer.elapsed() << " seconds.\n";
}

bool createProgram(const std::vector<ShaderSource>& sources, bool debug, bool optimize, bool disassemble, bool scalarize, Program& program)
{
    std::vector<uint32_t> spirv;

    bool result = createSPIRVFromSources(sources, debug, optimize, spirv);
    if(!result) {
        return result;
    }

    spv_target_env targetEnv = SPV_ENV_UNIVERSAL_1_3;

    if (optimize) {
        if(disassemble) {
            spv::Disassemble(std::cout, spirv);
        }

        optimizeSPIRV(targetEnv, spirv);
    }

    if(disassemble) {
        spv::Disassemble(std::cout, spirv);
    }

    if(false) {
        FILE *fp = fopen("spirv", "wb");
        fwrite(spirv.data(), 1, spirv.size() * 4, fp);
        fclose(fp);
    }

    spv_context context = spvContextCreate(targetEnv);
    spvBinaryParse(context, &program, spirv.data(), spirv.size(), Program::handleHeader, Program::handleInstruction, nullptr);

    if (program.hasUnimplemented) {
        return false;
    }

    {
        Timer timer;
        program.postParse(scalarize);
        std::cerr << "Post-parse took " << timer.elapsed() << " seconds.\n";
    }

    return true;
}

// https://stackoverflow.com/a/34571089/211234
static const char *BASE64_ALPHABET = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
std::string base64Encode(const std::string &in) {
    std::string out;
    int val = 0;
    int valb = -6;

    for (uint8_t c : in) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(BASE64_ALPHABET[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) {
        out.push_back(BASE64_ALPHABET[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    while (out.size() % 4 != 0) {
        out.push_back('=');
    }

    return out;
}

int main(int argc, char **argv)
{
    bool debug = false;
    bool disassemble = false;
    bool optimize = false;
    bool doNotShade = false;
    bool inputIsJSON = false;
    bool imageToTerminal = false;
    bool compile = false;
    bool scalarize = false;
    int threadCount = std::thread::hardware_concurrency();
    int frameStart = 0, frameEnd = 0;
    CommandLineParameters params;

    params.outputWidth = DEFAULT_WIDTH;
    params.outputHeight = DEFAULT_HEIGHT;
    params.beVerbose = false;
    params.throwOnUnimplemented = false;

    ShInitialize();

    char *progname = argv[0];
    argv++; argc--;

    while(argc > 0 && argv[0][0] == '-') {
        if(strcmp(argv[0], "-g") == 0) {

            debug = true;
            argv++; argc--;

        } else if(strcmp(argv[0], "--json") == 0) {

            inputIsJSON = true;
            argv++; argc--;

        } else if(strcmp(argv[0], "--term") == 0) {

            imageToTerminal = true;
            argv++; argc--;

        } else if(strcmp(argv[0], "-S") == 0) {

            disassemble = true;
            argv++; argc--;

        } else if(strcmp(argv[0], "-d") == 0) {

            if(argc < 3) {
                usage(progname);
                exit(EXIT_FAILURE);
            }
            params.outputWidth = atoi(argv[1]);
            params.outputHeight = atoi(argv[2]);
            argv += 3; argc -= 3;

        } else if(strcmp(argv[0], "-f") == 0) {

            if(argc < 3) {
                usage(progname);
                exit(EXIT_FAILURE);
            }
            frameStart = atoi(argv[1]);
            frameEnd = atoi(argv[2]);
            argv += 3; argc -= 3;

        } else if(strcmp(argv[0], "-j") == 0) {

            if(argc < 2) {
                usage(progname);
                exit(EXIT_FAILURE);
            }
            threadCount = atoi(argv[1]);
            argv += 2; argc -= 2;

        } else if(strcmp(argv[0], "-v") == 0) {

            params.beVerbose = true;
            argv++; argc--;

        } else if(strcmp(argv[0], "-t") == 0) {

            params.throwOnUnimplemented = true;
            argv++; argc--;

        } else if(strcmp(argv[0], "-O") == 0) {

            optimize = true;
            argv++; argc--;

        } else if(strcmp(argv[0], "-n") == 0) {

            doNotShade = true;
            argv++; argc--;

        } else if(strcmp(argv[0], "-c") == 0) {

            compile = true;
            argv++; argc--;

        } else if(strcmp(argv[0], "-s") == 0) {

            scalarize = true;
            argv++; argc--;

        } else if(strcmp(argv[0], "-h") == 0) {

            usage(progname);
            exit(EXIT_SUCCESS);

        } else if(strcmp(argv[0], "-") == 0) {

            // Read from stdin.
            break;

        } else {

            usage(progname);
            exit(EXIT_FAILURE);

        }
    }

    if(argc < 1) {
        usage(progname);
        exit(EXIT_FAILURE);
    }

    std::vector<ShaderToyRenderPassPtr> renderPasses;

    std::string filename = argv[0];
    std::string shader_code;

    if(!inputIsJSON) {

        shader_code = readFileContents(filename);

        ImagePtr image(new Image(Image::FORMAT_R8G8B8_UNORM, Image::DIM_2D, params.outputWidth, params.outputHeight));
        ShaderToyImage output {0, 0, {image, Sampler {}}};

        std::vector<ShaderSource> sources = { {"", ""}, {shader_code, filename}};
        ShaderToyRenderPassPtr pass(new ShaderToyRenderPass("Image", {}, {output}, sources, params));

        renderPasses.push_back(pass);

    } else {

        getOrderedRenderPassesFromJSON(filename, renderPasses, params);

    }

    ShaderSource preamble { readFileContents(shaderPreambleFilename), shaderPreambleFilename };
    ShaderSource epilogue { readFileContents(shaderEpilogueFilename), shaderEpilogueFilename };

    // Do passes

    for(auto& pass: renderPasses) {

        ShaderToyImage output = pass->outputs[0];

        std::vector<ShaderSource> sources;

        sources.push_back(preamble);
        for(auto& ss: pass->sources) {
            sources.push_back(ss);
        }
        sources.push_back(epilogue);

        bool success = createProgram(sources, debug, optimize, disassemble, scalarize, pass->pgm);
        if(!success) {
            exit(EXIT_FAILURE);
        }

        if (compile) {
            bool success = compileProgram(pass->pgm);
            exit(success ? EXIT_SUCCESS : EXIT_FAILURE);
        }

        if (doNotShade) {
            exit(EXIT_SUCCESS);
        }

        for(int i = 0; i < pass->inputs.size(); i++) {
            auto& toyImage = pass->inputs[i];
            pass->pgm.sampledImages[i] = toyImage.sampledImage;
        }
    }

    std::cout << "Using " << threadCount << " threads.\n";

    for(int frameNumber = frameStart; frameNumber <= frameEnd; frameNumber++) {
        for(auto& pass: renderPasses) {

            Timer timer;

            ShaderToyImage output = pass->outputs[0];
            ImagePtr image = output.sampledImage.image;

            // Workers decrement rowsLeft at the end of each row.
            rowsLeft = image->height;

            std::vector<std::thread *> thread;

            // Generate the rows on multiple threads.
            for (int t = 0; t < threadCount; t++) {
                thread.push_back(new std::thread(render, pass.get(), t, threadCount, frameNumber, frameNumber / 60.0));
                // std::this_thread::sleep_for(std::chrono::milliseconds(100)); XXX delete
            }

            // Progress information.
            thread.push_back(new std::thread(showProgress, image->height, timer.startTime()));

            // Wait for worker threads to quit.
            for (int t = 0; t < thread.size(); t++) {
                std::thread* td = thread.back();
                thread.pop_back();
                td->join();
            }

            double elapsedSeconds = timer.elapsed();
            std::cerr << "Shading pass " << pass->name << " took " << elapsedSeconds << " seconds ("
                << long(image->width*image->height/elapsedSeconds) << " pixels per second)\n";
        }

        if(false) {
            ShaderToyImage output = renderPasses[0]->outputs[0];
            ImagePtr image = output.sampledImage.image;

            std::ostringstream ss;
            ss << "pass0" << std::setfill('0') << std::setw(4) << frameNumber << std::setw(0) << ".ppm";
            std::ofstream imageFile(ss.str(), std::ios::out | std::ios::binary);
            image->writePpm(imageFile);
            imageFile.close();
        }

        ShaderToyImage output = renderPasses.back()->outputs[0];
        ImagePtr image = output.sampledImage.image;

        std::ostringstream ss;
        ss << "image" << std::setfill('0') << std::setw(4) << frameNumber << std::setw(0) << ".ppm";
        std::ofstream imageFile(ss.str(), std::ios::out | std::ios::binary);
        image->writePpm(imageFile);
        imageFile.close();

        if (imageToTerminal) {
            // https://www.iterm2.com/documentation-images.html
            std::ostringstream ss;
            image->writePpm(ss);
            std::cout << "\033]1337;File=width="
                << image->width << "px;height="
                << image->height << "px;inline=1:"
                << base64Encode(ss.str()) << "\007\n";
        }
    }

    exit(EXIT_SUCCESS);
}
